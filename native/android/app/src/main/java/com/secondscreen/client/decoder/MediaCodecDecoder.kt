package com.secondscreen.client.decoder

import android.media.MediaCodec
import android.media.MediaFormat
import android.util.Log
import android.view.Surface
import java.nio.ByteBuffer

class MediaCodecDecoder(private val renderSurface: Surface) {
    private val TAG = "MediaCodecDecoder"
    private var codec: MediaCodec? = null
    private var isRunning = false
    private val bufferInfo = MediaCodec.BufferInfo()

    fun initialize(width: Int = 1920, height: Int = 1080, mimeType: String = MediaFormat.MIMETYPE_VIDEO_AVC) {
        release()
        try {
            val format = MediaFormat.createVideoFormat(mimeType, width, height).apply {
                setInteger(MediaFormat.KEY_COLOR_FORMAT, android.media.MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface)
                setInteger(MediaFormat.KEY_LOW_LATENCY, 1)       // Low-latency decoding (Android 11+)
                setInteger(MediaFormat.KEY_PRIORITY, 0)           // Realtime priority
                setInteger(MediaFormat.KEY_OPERATING_RATE, 120)  // Fast decode clock
            }

            codec = MediaCodec.createDecoderByType(mimeType).apply {
                configure(format, renderSurface, null, 0)
                start()
            }
            isRunning = true
            Log.i(TAG, "Hardware MediaCodec decoder started for surface: ${width}x${height}")
        } catch (e: Exception) {
            Log.e(TAG, "Failed to initialize MediaCodec decoder: ${e.message}", e)
        }
    }

    fun feedEncodedNalu(naluData: ByteArray, presentationTimeUs: Long, isKeyFrame: Boolean) {
        val activeCodec = codec ?: return
        if (!isRunning) return

        try {
            val inputIndex = activeCodec.dequeueInputBuffer(1000) // 1ms timeout
            if (inputIndex >= 0) {
                val inputBuffer = activeCodec.getInputBuffer(inputIndex)
                inputBuffer?.clear()
                inputBuffer?.put(naluData)
                val flags = if (isKeyFrame) MediaCodec.BUFFER_FLAG_KEY_FRAME else 0
                activeCodec.queueInputBuffer(inputIndex, 0, naluData.size, presentationTimeUs, flags)
            }

            // Zero-copy render directly to hardware Surface
            var outputIndex = activeCodec.dequeueOutputBuffer(bufferInfo, 0)
            while (outputIndex >= 0) {
                activeCodec.releaseOutputBuffer(outputIndex, true) // true = render to Surface
                outputIndex = activeCodec.dequeueOutputBuffer(bufferInfo, 0)
            }
        } catch (e: Exception) {
            Log.w(TAG, "Decode frame error: ${e.message}")
        }
    }

    fun release() {
        isRunning = false
        try {
            codec?.stop()
            codec?.release()
        } catch (e: Exception) {
            Log.w(TAG, "Decoder release error: ${e.message}")
        }
        codec = null
    }
}
