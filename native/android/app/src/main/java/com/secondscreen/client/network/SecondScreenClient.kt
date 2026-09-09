package com.secondscreen.client.network

import android.util.Log
import kotlinx.coroutines.*
import java.io.DataInputStream
import java.io.DataOutputStream
import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.InetAddress
import java.net.Socket
import java.nio.ByteBuffer
import java.nio.ByteOrder

enum class ClientState {
    DISCONNECTED,
    DISCOVERING,
    PAIRING,
    CONNECTING,
    CONNECTED,
    STREAMING,
    RECONNECTING,
    ERROR
}

data class DiscoveredHost(
    val ip: String,
    val port: Int,
    val name: String
)

data class Telemetry(
    val fps: Float,
    val bitrateMbps: Float,
    val rttMs: Float,
    val totalLatencyMs: Float,
    val isMeasured: Boolean
)

class SecondScreenClient(
    private val onFrameReceived: (ByteArray, Long, Boolean) -> Unit,
    private val onStateChanged: (ClientState) -> Unit,
    private val onTelemetryReceived: (Telemetry) -> Unit
) {
    private val TAG = "SecondScreenClient"
    private var state = ClientState.DISCONNECTED
        set(value) {
            field = value
            onStateChanged(value)
        }

    private var controlSocket: Socket? = null
    private var dataInputStream: DataInputStream? = null
    private var dataOutputStream: DataOutputStream? = null
    private var clientScope = CoroutineScope(Dispatchers.IO + SupervisorJob())
    private var isRunning = false

    private var hostIp: String? = null
    private var hostPort: Int = 9876
    private var pin: String? = null

    fun discoverHosts(onHostDiscovered: (DiscoveredHost) -> Unit) {
        state = ClientState.DISCOVERING
        clientScope.launch {
            try {
                val socket = DatagramSocket()
                socket.broadcast = true
                socket.soTimeout = 3000

                val broadcastMessage = "{\"type\":\"DISCOVER\"}".toByteArray()
                val broadcastAddr = InetAddress.getByName("255.255.255.255")
                val packet = DatagramPacket(broadcastMessage, broadcastMessage.size, broadcastAddr, 9877)
                socket.send(packet)

                val rxBuf = ByteArray(1024)
                val rxPacket = DatagramPacket(rxBuf, rxBuf.size)
                socket.receive(rxPacket)

                val response = String(rxPacket.data, 0, rxPacket.length)
                Log.d(TAG, "Host discovered: $response from ${rxPacket.address.hostAddress}")

                onHostDiscovered(DiscoveredHost(rxPacket.address.hostAddress, 9876, "Windows Host"))
                socket.close()
            } catch (e: Exception) {
                Log.w(TAG, "Discovery timed out or failed: ${e.message}")
            }
        }
    }

    fun connect(ip: String, port: Int = 9876, pinCode: String) {
        this.hostIp = ip
        this.hostPort = port
        this.pin = pinCode
        isRunning = true
        state = ClientState.CONNECTING

        clientScope.launch {
            try {
                Log.i(TAG, "Connecting to host $ip:$port...")
                val sock = Socket(ip, port)
                sock.tcpNoDelay = true
                sock.soTimeout = 10000

                controlSocket = sock
                dataInputStream = DataInputStream(sock.getInputStream())
                dataOutputStream = DataOutputStream(sock.getOutputStream())

                // 1. Send PAIR_REQUEST with 6-digit PIN
                state = ClientState.PAIRING
                sendPairRequest(pinCode)

                // 2. Main Binary Packet Stream Loop
                readPacketStreamLoop()
            } catch (e: Exception) {
                Log.e(TAG, "Connection error: ${e.message}", e)
                state = ClientState.ERROR
                handleReconnect()
            }
        }
    }

    private fun sendPairRequest(pinCode: String) {
        val out = dataOutputStream ?: return
        val headerBuf = ByteBuffer.allocate(24).order(ByteOrder.LITTLE_ENDIAN)
        headerBuf.putInt(0x53325343)      // magic 'S2SC'
        headerBuf.putShort(1.toShort())   // version 1
        headerBuf.put(0x03.toByte())      // type PAIR_REQUEST
        headerBuf.put(0.toByte())         // flags
        headerBuf.putInt(1)               // sequence
        headerBuf.putLong(System.currentTimeMillis() * 1000) // timestamp
        headerBuf.putInt(pinCode.toByteArray().size) // payload size

        out.write(headerBuf.array())
        out.write(pinCode.toByteArray())
        out.flush()
    }

    private fun readPacketStreamLoop() {
        val input = dataInputStream ?: return
        val headerBytes = ByteArray(24)

        while (isRunning && state != ClientState.DISCONNECTED) {
            input.readFully(headerBytes)
            val headerBuf = ByteBuffer.wrap(headerBytes).order(ByteOrder.LITTLE_ENDIAN)
            val magic = headerBuf.int
            if (magic != 0x53325343) {
                Log.w(TAG, "Invalid packet magic: 0x${Integer.toHexString(magic)}")
                continue
            }

            val version = headerBuf.short
            val type = headerBuf.get().toInt()
            val flags = headerBuf.get().toInt()
            val sequence = headerBuf.int
            val timestamp = headerBuf.long
            val payloadSize = headerBuf.int

            if (version.toInt() != 1 || payloadSize < 0 || payloadSize > 16 * 1024 * 1024) {
                throw IllegalStateException("Unsupported protocol packet: version=$version payloadSize=$payloadSize")
            }

            val isKeyFrame = (flags and 0x01) != 0

            when (type) {
                0x04 -> { // PAIR_RESPONSE
                    if (payloadSize > 0) {
                        val sessionToken = ByteArray(payloadSize)
                        input.readFully(sessionToken)
                        Log.i(TAG, "Authenticated session established.")
                    }
                    Log.i(TAG, "Authenticated successfully. Streaming active.")
                    state = ClientState.STREAMING
                }
                0x08 -> { // FRAME (H.264 NALU payload)
                    if (payloadSize > 0) {
                        val payload = ByteArray(payloadSize)
                        input.readFully(payload)
                        onFrameReceived(payload, timestamp, isKeyFrame)
                    }
                }
                0x0B -> { // TELEMETRY
                    if (payloadSize >= 37) {
                        val telemBytes = ByteArray(payloadSize)
                        input.readFully(telemBytes)
                        val tBuf = ByteBuffer.wrap(telemBytes).order(ByteOrder.LITTLE_ENDIAN)
                        val fps = tBuf.float
                        val bitrate = tBuf.float
                        val rtt = tBuf.float
                        val decodeLatency = tBuf.float // decodeLatencyMs
                        val renderLatency = tBuf.float // renderLatencyMs
                        val totalLatency = tBuf.float  // totalLatencyMs
                        val packetLoss = tBuf.float    // packetLossPercent
                        val frameDrops = tBuf.int      // frameDrops
                        val jitter = tBuf.float        // jitterMs
                        val isMeasured = tBuf.get().toInt() == 1 // isMeasured at offset 36

                        onTelemetryReceived(Telemetry(fps, bitrate, rtt, totalLatency, isMeasured))
                    }
                }
                0x0A -> { // PONG
                    val rtt = (System.currentTimeMillis() * 1000 - timestamp) / 1000.0f
                    Log.v(TAG, "Measured round-trip time: $rtt ms")
                }
                else -> {
                    if (payloadSize > 0) {
                        input.skipBytes(payloadSize)
                    }
                }
            }
        }
    }

    fun sendInputEvent(actionType: Byte, normX: Float, normY: Float, pressure: Float, button: Byte) {
        clientScope.launch {
            try {
                val out = dataOutputStream ?: return@launch
                val buffer = ByteBuffer.allocate(24 + 22).order(ByteOrder.LITTLE_ENDIAN)
                // Header (24 bytes)
                buffer.putInt(0x53325343)
                buffer.putShort(1.toShort())
                buffer.put(0x0C.toByte()) // INPUT_EVENT
                buffer.put(0.toByte())
                buffer.putInt(0)
                buffer.putLong(System.currentTimeMillis() * 1000)
                buffer.putInt(22) // payload size is exactly 22 bytes

                // Input Payload (22 bytes)
                buffer.put(actionType)
                buffer.putFloat(normX)
                buffer.putFloat(normY)
                buffer.putFloat(pressure)
                buffer.put(button)
                buffer.putLong(System.currentTimeMillis())

                out.write(buffer.array())
                out.flush()
            } catch (e: Exception) {
                Log.w(TAG, "Failed to send input event: ${e.message}")
            }
        }
    }

    private fun handleReconnect() {
        if (!isRunning) return
        state = ClientState.RECONNECTING
        clientScope.launch {
            delay(2000)
            val ip = hostIp
            val pinCode = pin
            if (ip != null && pinCode != null && isRunning) {
                Log.i(TAG, "Attempting automatic reconnection to $ip...")
                connect(ip, hostPort, pinCode)
            }
        }
    }

    fun disconnect() {
        isRunning = false
        state = ClientState.DISCONNECTED
        try {
            controlSocket?.close()
            controlSocket = null
        } catch (e: Exception) {
            Log.w(TAG, "Error closing socket: ${e.message}")
        }
    }
}
