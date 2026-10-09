package com.zoavintsoa.secondscreen

import android.media.MediaCodec
import android.media.MediaFormat
import android.view.SurfaceHolder

class VideoStreamDecoder(private val surface: SurfaceHolder) {
    private var decoder: MediaCodec? = null
    private var codecId = 0
    private var width = 0
    private var height = 0

    fun decode(
        codec: Int,
        w: Int,
        h: Int,
        accessUnit: ByteArray,
        timestampUs: Long,
        keyFrame: Boolean
    ) {
        require(w in 1..8192 && h in 1..8192) {
            "Invalid video dimensions: ${w}x${h}"
        }
        require(accessUnit.isNotEmpty() && accessUnit.size <= MAX_ACCESS_UNIT_BYTES) {
            "Invalid video access-unit size: ${accessUnit.size}"
        }

        try {
            if (decoder == null || codecId != codec || width != w || height != h) {
                configure(codec, w, h)
            }

            val activeDecoder = decoder ?: error("Video decoder was not initialized")
            val inputIndex = activeDecoder.dequeueInputBuffer(INPUT_TIMEOUT_US)
            if (inputIndex >= 0) {
                val inputBuffer = activeDecoder.getInputBuffer(inputIndex)
                if (inputBuffer == null) {
                    // Do not leave an input slot dequeued when the codec returns
                    // an unexpected null buffer.
                    activeDecoder.queueInputBuffer(inputIndex, 0, 0, 0L, 0)
                    error("MediaCodec returned a null input buffer")
                }
                if (accessUnit.size > inputBuffer.capacity()) {
                    activeDecoder.queueInputBuffer(inputIndex, 0, 0, 0L, 0)
                    error(
                        "Video access unit (${accessUnit.size} bytes) exceeds decoder input buffer " +
                            "(${inputBuffer.capacity()} bytes)"
                    )
                }

                inputBuffer.clear()
                inputBuffer.put(accessUnit)
                // KEY_FRAME is an output flag. The decoder detects keyframes
                // from the compressed bitstream; input buffers use no such flag.
                activeDecoder.queueInputBuffer(
                    inputIndex,
                    0,
                    accessUnit.size,
                    timestampUs.coerceAtLeast(0L),
                    0
                )
            }

            drainOutput(activeDecoder)
        } catch (failure: Throwable) {
            // MediaCodec can remain in an unusable state after an input/output
            // error. Tear it down so the caller can request a fresh keyframe
            // and the next access unit can initialize a clean decoder.
            release()
            throw failure
        }
    }

    private fun drainOutput(activeDecoder: MediaCodec) {
        val info = MediaCodec.BufferInfo()
        while (true) {
            val outputIndex = activeDecoder.dequeueOutputBuffer(info, 0)
            when {
                outputIndex >= 0 -> activeDecoder.releaseOutputBuffer(outputIndex, true)
                outputIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED -> {
                    // The surface renderer follows the format selected by MediaCodec.
                }
                outputIndex == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED -> Unit
                else -> return
            }
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
            setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, MAX_ACCESS_UNIT_BYTES)
        }
        val created = MediaCodec.createDecoderByType(mime)
        try {
            created.configure(format, surface.surface, null, 0)
            created.start()
            decoder = created
            codecId = codec
            width = w
            height = h
        } catch (failure: Throwable) {
            runCatching { created.stop() }
            runCatching { created.release() }
            throw failure
        }
    }

    fun release() {
        decoder?.runCatching { stop() }
        decoder?.runCatching { release() }
        decoder = null
        codecId = 0
        width = 0
        height = 0
    }

    private companion object {
        const val INPUT_TIMEOUT_US = 10_000L
        const val MAX_ACCESS_UNIT_BYTES = 16 * 1024 * 1024
    }
}
