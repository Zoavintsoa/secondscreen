package com.zoavintsoa.secondscreen

import android.media.MediaCodec
import android.media.MediaFormat
import android.os.Build
import android.view.SurfaceHolder

/**
 * Hardware-accelerated video decoder for SecondScreen.
 *
 * Calls are synchronized because Surface lifecycle callbacks can arrive while
 * the network worker is delivering access units. A failed decoder is discarded
 * so a later keyframe can start a clean decoding session.
 */
class VideoStreamDecoder(private val surface: SurfaceHolder) {
    private var decoder: MediaCodec? = null
    private var codecId = 0
    private var width = 0
    private var height = 0

    @Synchronized
    fun decode(
        codec: Int,
        w: Int,
        h: Int,
        accessUnit: ByteArray,
        timestampUs: Long,
        keyFrame: Boolean
    ) {
        if (!surface.surface.isValid) return
        if (w !in 320..3840 || h !in 240..2160) return
        if (accessUnit.isEmpty() || accessUnit.size > MAX_ACCESS_UNIT_BYTES) return

        try {
            if (decoder == null || codecId != codec || width != w || height != h) {
                configure(codec, w, h, accessUnit.size)
            }

            // Keep a local reference to the active codec. Reconfiguration below
            // can replace the decoder instance; output must be drained from the
            // replacement, not from a stale reference.
            var active = decoder ?: return
            var inputIndex = active.dequeueInputBuffer(INPUT_TIMEOUT_US)
            if (inputIndex >= 0) {
                var inputBuffer = active.getInputBuffer(inputIndex)
                if (inputBuffer == null || accessUnit.size > inputBuffer.capacity()) {
                    // The access unit can exceed a device codec's default input
                    // buffer size. Recreate with an explicit, bounded capacity.
                    configure(codec, w, h, accessUnit.size)
                    active = decoder ?: return
                    inputIndex = active.dequeueInputBuffer(INPUT_TIMEOUT_US)
                    if (inputIndex < 0) {
                        drainOutput(active)
                        return
                    }
                    inputBuffer = active.getInputBuffer(inputIndex)
                }

                if (inputBuffer == null || accessUnit.size > inputBuffer.capacity()) {
                    // Do not queue a partial access unit. Resetting lets the
                    // caller recover on its next keyframe instead of corrupting
                    // the decoder's bitstream state.
                    release()
                    return
                }

                inputBuffer.clear()
                inputBuffer.put(accessUnit)
                // KEY_FRAME is an output-side flag on MediaCodec. Decoder input
                // buffers are submitted without it; keyframe recovery is managed
                // by the stream/session layer.
                active.queueInputBuffer(
                    inputIndex,
                    0,
                    accessUnit.size,
                    timestampUs.coerceAtLeast(0L),
                    0
                )
            }

            drainOutput(active)
        } catch (_: IllegalStateException) {
            // MediaCodec can enter an invalid state after a surface or driver
            // change. Drop the instance and allow the stream to recover cleanly.
            release()
        } catch (_: IllegalArgumentException) {
            release()
        }
    }

    private fun configure(codec: Int, w: Int, h: Int, requiredInputBytes: Int) {
        release()

        val mime = when (codec) {
            1 -> "video/avc"
            2 -> "video/hevc"
            else -> throw IllegalArgumentException("Unsupported codec: $codec")
        }
        require(w in 320..3840 && h in 240..2160)
        require(requiredInputBytes in 1..MAX_ACCESS_UNIT_BYTES)
        check(surface.surface.isValid) { "Video surface is not valid" }

        val format = MediaFormat.createVideoFormat(mime, w, h).apply {
            setInteger(MediaFormat.KEY_PRIORITY, 0)
            setInteger(
                MediaFormat.KEY_MAX_INPUT_SIZE,
                maxOf(DEFAULT_INPUT_BUFFER_BYTES, requiredInputBytes)
            )
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                setInteger(MediaFormat.KEY_LOW_LATENCY, 1)
            }
        }

        val created = MediaCodec.createDecoderByType(mime)
        try {
            created.configure(format, surface.surface, null, 0)
            created.start()
        } catch (t: Throwable) {
            runCatching { created.stop() }
            created.release()
            throw t
        }

        decoder = created
        codecId = codec
        width = w
        height = h
    }

    private fun drainOutput(active: MediaCodec) {
        val info = MediaCodec.BufferInfo()
        while (true) {
            val outputIndex = active.dequeueOutputBuffer(info, 0)
            if (outputIndex < 0) return
            active.releaseOutputBuffer(outputIndex, true)
        }
    }

    @Synchronized
    fun release() {
        val old = decoder
        decoder = null
        codecId = 0
        width = 0
        height = 0
        if (old != null) {
            runCatching { old.stop() }
            runCatching { old.release() }
        }
    }

    private companion object {
        const val INPUT_TIMEOUT_US = 10_000L
        const val DEFAULT_INPUT_BUFFER_BYTES = 2 * 1024 * 1024
        const val MAX_ACCESS_UNIT_BYTES = 16 * 1024 * 1024
    }
}
