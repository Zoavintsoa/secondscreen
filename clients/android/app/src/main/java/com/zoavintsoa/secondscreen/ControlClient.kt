package com.zoavintsoa.secondscreen

import org.json.JSONObject
import java.io.DataInputStream
import java.io.DataOutputStream
import java.net.Socket
import java.nio.ByteBuffer
import java.nio.ByteOrder

data class ControlResult(
    val authenticated: Boolean,
    val sessionToken: String? = null,
    val streamConfig: StreamConfig? = null
)

class ControlClient(private val socket: Socket) {
    private val input = DataInputStream(socket.getInputStream())
    private val output = DataOutputStream(socket.getOutputStream())

    fun hello(deviceId: String, deviceName: String, capabilitiesJson: String = "{}"): Boolean {
        val capabilities = JSONObject(capabilitiesJson)
        val json = JSONObject()
            .put("deviceId", deviceId)
            .put("deviceName", deviceName)
            .put("capabilities", capabilities)
        send(TYPE_HELLO, json.toString())
        return receive()?.type == TYPE_CAPABILITIES
    }

    fun authenticate(token: String): ControlResult {
        if (!SESSION_TOKEN.matches(token)) return ControlResult(false)
        send(TYPE_AUTH, JSONObject().put("sessionToken", token).toString())
        val response = receive() ?: return ControlResult(false)

        // The host's AUTH success response is STREAM_CONFIG (0x05). Do not
        // treat unrelated messages such as PING as proof of authentication.
        if (response.type != TYPE_STREAM_CONFIG) return ControlResult(false)
        return ControlResult(true, token, StreamConfig.parse(response.json))
    }

    fun pair(code: String): ControlResult {
        if (!PAIRING_CODE.matches(code)) return ControlResult(false)
        send(TYPE_PAIR_REQUEST, JSONObject().put("code", code).toString())
        val response = receive() ?: return ControlResult(false)
        if (response.type != TYPE_PAIR_RESPONSE) return ControlResult(false)

        val body = runCatching { JSONObject(response.json) }.getOrNull()
            ?: return ControlResult(false)
        if (body.optString("status") != "paired") return ControlResult(false)

        val token = body.optString("sessionToken").takeIf { SESSION_TOKEN.matches(it) }
            ?: return ControlResult(false)
        return ControlResult(true, token, StreamConfig.parse(response.json))
    }

    fun requestKeyframe() {
        // Protocol v1: KEYFRAME_REQUEST = 0x0B.
        send(TYPE_KEYFRAME_REQUEST, "{}")
    }

    fun close() {
        runCatching { send(TYPE_CLOSE, "{}") }
        runCatching { socket.close() }
    }

    private fun send(type: Int, json: String) {
        require(type in 1..14) { "Unknown control message type: $type" }
        val payload = json.toByteArray(Charsets.UTF_8)
        require(payload.size <= MAX_PAYLOAD_BYTES) { "Control payload is too large" }

        val frame = ByteBuffer.allocate(HEADER_BYTES + payload.size).order(ByteOrder.BIG_ENDIAN)
        frame.put(MAGIC)
        frame.put(PROTOCOL_MAJOR.toByte())
        frame.put(type.toByte())
        frame.putShort(0)
        frame.putInt(payload.size)
        frame.put(payload)
        output.write(frame.array())
        output.flush()
    }

    private fun receive(): ControlResponse? {
        val magic = ByteArray(MAGIC.size)
        input.readFully(magic)
        require(magic.contentEquals(MAGIC)) { "Invalid control frame magic" }
        require(input.readUnsignedByte() == PROTOCOL_MAJOR) { "Unsupported control protocol version" }

        val type = input.readUnsignedByte()
        require(type in 1..14) { "Unknown control response type: $type" }
        input.readUnsignedShort() // flags reserved for future protocol versions
        val length = input.readInt()
        require(length in 0..MAX_PAYLOAD_BYTES) { "Invalid control payload length: $length" }

        val payload = ByteArray(length)
        input.readFully(payload)
        return ControlResponse(type, payload.toString(Charsets.UTF_8))
    }

    private data class ControlResponse(val type: Int, val json: String)

    private companion object {
        const val HEADER_BYTES = 12
        const val MAX_PAYLOAD_BYTES = 64 * 1024
        const val PROTOCOL_MAJOR = 1
        const val TYPE_HELLO = 0x01
        const val TYPE_PAIR_REQUEST = 0x02
        const val TYPE_PAIR_RESPONSE = 0x03
        const val TYPE_CAPABILITIES = 0x04
        const val TYPE_STREAM_CONFIG = 0x05
        const val TYPE_CLOSE = 0x08
        const val TYPE_AUTH = 0x0A
        const val TYPE_KEYFRAME_REQUEST = 0x0B
        val PAIRING_CODE = Regex("[0-9]{6}")
        val SESSION_TOKEN = Regex("[A-Fa-f0-9]{64}")
        val MAGIC = byteArrayOf('S'.code.toByte(), 'S'.code.toByte(), 'C'.code.toByte(), 'P'.code.toByte())
    }
}
