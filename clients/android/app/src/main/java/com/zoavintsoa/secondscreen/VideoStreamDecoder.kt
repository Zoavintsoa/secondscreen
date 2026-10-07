package com.zoavintsoa.secondscreen

import android.media.MediaCodec
import android.media.MediaFormat
import android.view.SurfaceHolder

class VideoStreamDecoder(private val surface: SurfaceHolder) {
    private var decoder: MediaCodec? = null
    private var codecId = 0

    fun decode(codec: Int, width: Int, height: Int, accessUnit: ByteArray, timestampUs: Long, keyFrame: Boolean) {
        if (decoder == null || codecId != codec) configure(codec, width, height)
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

    private fun configure(codec: Int, width: Int, height: Int) {
        release()
        val mime = when (codec) {
            1 -> "video/avc"
            2 -> "video/hevc"
            else -> error("Unsupported codec: $codec")
        }
        val format = MediaFormat.createVideoFormat(mime, width, height)
        decoder = MediaCodec.createDecoderByType(mime).also {
            it.configure(format, surface.surface, null, 0)
            it.start()
        }
        codecId = codec
    }

    fun release() {
        decoder?.runCatching { stop() }
        decoder?.runCatching { release() }
        decoder = null
        codecId = 0
    }
}
