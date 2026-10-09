package com.zoavintsoa.secondscreen

import android.media.MediaCodec
import android.media.MediaFormat
import android.view.SurfaceHolder

class VideoStreamDecoder(private val surface: SurfaceHolder) {
    private var decoder: MediaCodec? = null
    private var codecId = 0
    private var width = 0
    private var height = 0

    fun decode(codec: Int, w: Int, h: Int, accessUnit: ByteArray, timestampUs: Long, keyFrame: Boolean) {
        if (decoder == null || codecId != codec || width != w || height != h) {
            runCatching { configure(codec, w, h) }.getOrElse { return }
        }
        val d = decoder ?: return
        val index = d.dequeueInputBuffer(10_000)
        if (index >= 0) {
            val buffer = d.getInputBuffer(index) ?: run {
                // Return the dequeued slot so the codec cannot remain starved if a
                // device temporarily fails to expose its input ByteBuffer.
                runCatching { d.queueInputBuffer(index, 0, 0, 0L, 0) }
                return
            }
            if (accessUnit.size > buffer.capacity()) {
                runCatching { configure(codec, w, h) }.getOrElse { return }
                val replacement = decoder ?: return
                val replacementIndex = replacement.dequeueInputBuffer(10_000)
                if (replacementIndex < 0) return
                val replacementBuffer = replacement.getInputBuffer(replacementIndex) ?: return
                if (accessUnit.size > replacementBuffer.capacity()) return
                replacementBuffer.clear()
                replacementBuffer.put(accessUnit)
                replacement.queueInputBuffer(
                    replacementIndex, 0, accessUnit.size, timestampUs.coerceAtLeast(0L),
                    if (keyFrame) MediaCodec.BUFFER_FLAG_KEY_FRAME else 0
                )
            } else {
                buffer.clear()
                buffer.put(accessUnit)
                d.queueInputBuffer(
                    index, 0, accessUnit.size, timestampUs.coerceAtLeast(0L),
                    if (keyFrame) MediaCodec.BUFFER_FLAG_KEY_FRAME else 0
                )
            }
        }
        val info = MediaCodec.BufferInfo()
        while (true) {
            val output = d.dequeueOutputBuffer(info, 0)
            if (output >= 0) d.releaseOutputBuffer(output, true) else break
        }
    }

    private fun configure(codec: Int, w: Int, h: Int) {
        release()
        val mime = when (codec) {
            1 -> "video/avc"
            2 -> "video/hevc"
            else -> error("Unsupported codec: $codec")
        }
        val format = MediaFormat.createVideoFormat(mime, w, h).apply {
            setInteger(MediaFormat.KEY_PRIORITY, 0)
            setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, 16 * 1024 * 1024)
        }
        val created = MediaCodec.createDecoderByType(mime)
        try {
            created.configure(format, surface.surface, null, 0)
            created.start()
        } catch (failure: Throwable) {
            runCatching { created.stop() }
            runCatching { created.release() }
            throw failure
        }
        decoder = created
        codecId = codec
        width = w
        height = h
    }

    fun release() {
        decoder?.runCatching { stop() }
        decoder?.runCatching { release() }
        decoder = null
        codecId = 0
        width = 0
        height = 0
    }
}
