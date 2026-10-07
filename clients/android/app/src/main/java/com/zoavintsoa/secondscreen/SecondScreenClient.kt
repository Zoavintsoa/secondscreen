package com.zoavintsoa.secondscreen

import android.media.MediaCodec
import android.media.MediaFormat
import android.view.SurfaceHolder
import java.io.BufferedInputStream
import java.io.DataInputStream
import java.net.InetSocketAddress
import java.net.Socket
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicBoolean

class SecondScreenClient(
    private val surface: SurfaceHolder,
    private val onStatus: (String) -> Unit
) : SurfaceHolder.Callback {

    private val executor = Executors.newSingleThreadExecutor()
    private val running = AtomicBoolean(false)
    private var socket: Socket? = null
    private var decoder: MediaCodec? = null

    private var discoveredHost: HostAdvertisement? = null
    private var streamWidth = 1920
    private var streamHeight = 1080

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
                onStatus("SecondScreen — connexion…")
                val host = discoverHost() ?: throw IllegalStateException("No SecondScreen host found")

                val s = Socket()
                s.tcpNoDelay = true
                s.connect(InetSocketAddress(host.address, host.videoPort), 1500)
                socket = s

                onStatus("SecondScreen — connecté à " + host.name)
                consumeVideo(DataInputStream(BufferedInputStream(s.getInputStream())))
            } catch (_: Throwable) {
                onStatus("SecondScreen — reconnexion…")
                if (running.get()) Thread.sleep(1000)
            } finally {
                socket?.close()
                socket = null
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
            require(magic.contentEquals(byteArrayOf('S'.code.toByte(), 'S'.code.toByte(), 'V'.code.toByte(), 'F'.code.toByte())))

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

            val d = decoder ?: continue
            val index = d.dequeueInputBuffer(10_000)
            if (index >= 0) {
                val buffer = d.getInputBuffer(index) ?: continue
                buffer.clear()
                buffer.put(accessUnit)
                d.queueInputBuffer(
                    index,
                    0,
                    accessUnit.size,
                    timestampUs.coerceAtLeast(0L),
                    if ((flags and 1) != 0) MediaCodec.BUFFER_FLAG_KEY_FRAME else 0
                )
            }

            val info = MediaCodec.BufferInfo()
            while (true) {
                val output = d.dequeueOutputBuffer(info, 0)
                if (output >= 0) {
                    d.releaseOutputBuffer(output, true)
                } else {
                    break
                }
            }
        }
    }

    private fun configureDecoder(codecId: Int, width: Int, height: Int) {
        releaseDecoder()

        val mime = when (codecId) {
            1 -> "video/avc"
            2 -> "video/hevc"
            else -> error("Unsupported codec: $codecId")
        }

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

    fun close() {
        stop()
        executor.shutdownNow()
    }

    private fun stop() {
        running.set(false)
        socket?.runCatching { close() }
        releaseDecoder()
    }
}
