package com.zoavintsoa.secondscreen

import android.media.MediaCodec
import android.media.MediaFormat
import android.view.Surface
import android.view.SurfaceHolder
import java.io.BufferedInputStream
import java.io.DataInputStream
import java.net.InetSocketAddress
import java.nio.ByteBuffer
import java.nio.ByteOrder
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

    // Current bring-up endpoint. Discovery/pairing replaces this with the
    // selected host once the control protocol is connected.
    private var discoveredHost: HostAdvertisement? = null
    private var streamWidth = 1920
    private var streamHeight = 1080

    override fun surfaceCreated(holder: SurfaceHolder) {
        start()
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) = Unit

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        stop()
    }

    private fun start() {
        if (!running.compareAndSet(false, true)) return
        executor.execute {
            connectLoop()
        }
    }

    private fun connectLoop() {
        while (running.get()) {
            try {
                onStatus("SecondScreen — connexion…")
                val s = Socket()
                s.tcpNoDelay = true
                val host = discoverHost() ?: throw IllegalStateException("No SecondScreen host found")
                s.connect(InetSocketAddress(host.address, host.controlPort), 1500)
                socket = s
                onStatus("SecondScreen — connecté")
                consumeVideo(DataInputStream(BufferedInputStream(s.getInputStream())))

            } catch (t: Throwable) {
                onStatus("SecondScreen — reconnexion…")
                Thread.sleep(1000)
            } finally {
                socket?.close()
                socket = null
                releaseDecoder()
            }
        }
    }

    private fun discoverHost(): HostAdvertisement? {
        discoveredHost?.let { return it }
        val found = DiscoveryClient().discover()
        discoveredHost = found
        return found
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
            input.readLong()
            val length = input.readInt()
            require(length in 1..16_777_216)

            val accessUnit = ByteArray(length)
            input.readFully(accessUnit)

            if ((flags and 2) != 0) {
                // Configuration access units are decoder-specific. Keep the decoder
                // alive and let MediaCodec consume SPS/PPS/VPS from the Annex-B stream.
            }
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
                d.queueInputBuffer(index, 0, accessUnit.size, 0, if ((flags and 1) != 0) MediaCodec.BUFFER_FLAG_KEY_FRAME else 0)
            }

            val info = MediaCodec.BufferInfo()
            while (true) {
                val output = d.dequeueOutputBuffer(info, 0)
                if (output >= 0) {
                    d.releaseOutputBuffer(output, true)
                } else if (output == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED || output == MediaCodec.INFO_TRY_AGAIN_LATER) {
                    break
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

        val format = MediaFormat.createVideoFormat(mime, width, height)
        format.setInteger(MediaFormat.KEY_PRIORITY, 0)
        format.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, 16 * 1024 * 1024)
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
