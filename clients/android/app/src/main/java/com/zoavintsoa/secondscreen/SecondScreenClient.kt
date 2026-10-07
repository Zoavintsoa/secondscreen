package com.zoavintsoa.secondscreen

import android.content.Context
import android.media.MediaCodec
import android.media.MediaFormat
import android.view.SurfaceHolder
import java.io.BufferedInputStream
import java.io.DataInputStream
import java.net.InetSocketAddress
import java.net.Socket
import java.util.UUID
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicBoolean

class SecondScreenClient(
    private val context: Context,
    private val surface: SurfaceHolder,
    private val onStatus: (String) -> Unit,
    private val onPairingRequired: (String) -> String?
) : SurfaceHolder.Callback {
    private val executor = Executors.newSingleThreadExecutor()
    private val running = AtomicBoolean(false)
    private var socket: Socket? = null
    private var controlSocket: Socket? = null
    private var controlClient: ControlClient? = null
    private var decoder: MediaCodec? = null
    private var discoveredHost: HostAdvertisement? = null
    private var streamWidth = 1920
    private var streamHeight = 1080
    private val preferences = context.getSharedPreferences("secondscreen", Context.MODE_PRIVATE)
    private val deviceId: String by lazy {
        preferences.getString("deviceId", null) ?: UUID.randomUUID().toString().also {
            preferences.edit().putString("deviceId", it).apply()
        }
    }

    override fun surfaceCreated(holder: SurfaceHolder) { start() }
    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) = Unit
    override fun surfaceDestroyed(holder: SurfaceHolder) { stop() }

    private fun start() {
        if (!running.compareAndSet(false, true)) return
        executor.execute { connectLoop() }
    }

    private fun connectLoop() {
        while (running.get()) {
            try {
                onStatus("SecondScreen — recherche d’un hôte…")
                val host = discoverHost() ?: throw IllegalStateException("No SecondScreen host found")

                val cs = Socket().apply { tcpNoDelay = true; connect(InetSocketAddress(host.address, host.controlPort), 1500) }
                controlSocket = cs
                val cc = ControlClient(cs)
                controlClient = cc
                if (!cc.hello(deviceId, "Android SecondScreen")) throw IllegalStateException("Control HELLO rejected")

                val storedToken = preferences.getString("sessionToken", null)
                var authenticated = storedToken != null && runCatching { cc.authenticate(storedToken) }.getOrDefault(false)

                if (!authenticated) {
                    onStatus("SecondScreen — appairage requis")
                    val code = onPairingRequired(host.name) ?: throw IllegalStateException("Pairing cancelled")
                    val result = cc.pair(code.trim())
                    if (!result.authenticated || result.sessionToken == null) throw IllegalStateException("Pairing rejected")
                    preferences.edit().putString("sessionToken", result.sessionToken).apply()
                    authenticated = true
                }

                onStatus("SecondScreen — sécurisé, connexion vidéo…")
                val s = Socket().apply { tcpNoDelay = true; connect(InetSocketAddress(host.address, host.videoPort), 1500) }
                socket = s
                consumeVideo(DataInputStream(BufferedInputStream(s.getInputStream())))
            } catch (_: Throwable) {
                onStatus("SecondScreen — reconnexion…")
                if (running.get()) Thread.sleep(1000)
            } finally {
                socket?.close(); socket = null
                controlClient?.close(); controlClient = null
                controlSocket = null
                releaseDecoder()
            }
        }
    }

    private fun discoverHost(): HostAdvertisement? {
        discoveredHost = DiscoveryClient().discover() ?: discoveredHost
        return discoveredHost
    }

    private fun consumeVideo(input: DataInputStream) {
        var configured = false
        while (running.get()) {
            val magic = ByteArray(4)
            input.readFully(magic)
            when (String(magic, Charsets.US_ASCII)) {
                "SSVF" -> {
                    val codec = input.readUnsignedByte()
                    val flags = input.readUnsignedByte()
                    input.readUnsignedShort()
                    val timestampUs = input.readLong()
                    val length = input.readInt()
                    require(length in 1..16_777_216)
                    val accessUnit = ByteArray(length)
                    input.readFully(accessUnit)
                    if (!configured) {
                        configureDecoder(codec, streamWidth, streamHeight)
                        configured = true
                    }
                    queueAccessUnit(codec, accessUnit, timestampUs, (flags and 1) != 0)
                }
                "SSVG" -> {
                    val header = ByteArray(20)
                    input.readFully(header)
                    val packet = ByteArray(24 + input.available().coerceAtMost(0))
                    // TCP bring-up is legacy SSVF; SSVG is reserved for QUIC datagrams.
                    throw IllegalStateException("SSVG received on TCP bring-up")
                }
                else -> throw IllegalStateException("Unknown video frame magic")
            }
        }
    }

    private fun queueAccessUnit(codec: Int, accessUnit: ByteArray, timestampUs: Long, keyFrame: Boolean) {
        if (decoder == null) configureDecoder(codec, streamWidth, streamHeight)
        val d = decoder ?: return
        val index = d.dequeueInputBuffer(10_000)
        if (index >= 0) {
            val buffer = d.getInputBuffer(index) ?: return
            buffer.clear()
            buffer.put(accessUnit)
            d.queueInputBuffer(index, 0, accessUnit.size, timestampUs.coerceAtLeast(0L),
                if (keyFrame) MediaCodec.BUFFER_FLAG_KEY_FRAME else 0)
        }
        val info = MediaCodec.BufferInfo()
        while (true) {
            val output = d.dequeueOutputBuffer(info, 0)
            if (output >= 0) d.releaseOutputBuffer(output, true) else break
        }
    }

    private fun configureDecoder(codecId: Int, width: Int, height: Int) {
        releaseDecoder()
        val mime = when (codecId) { 1 -> "video/avc"; 2 -> "video/hevc"; else -> error("Unsupported codec: $codecId") }
        val format = MediaFormat.createVideoFormat(mime, width, height).apply {
            setInteger(MediaFormat.KEY_PRIORITY, 0)
            setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, 16 * 1024 * 1024)
        }
        decoder = MediaCodec.createDecoderByType(mime).also {
            it.configure(format, surface.surface, null, 0)
            it.start()
        }
    }

    private fun releaseDecoder() {
        decoder?.runCatching { stop() }
        decoder?.runCatching { release() }
        decoder = null
    }

    fun close() { stop(); executor.shutdownNow() }
    private fun stop() {
        running.set(false)
        socket?.runCatching { close() }
        controlSocket?.runCatching { close() }
        releaseDecoder()
    }
}
