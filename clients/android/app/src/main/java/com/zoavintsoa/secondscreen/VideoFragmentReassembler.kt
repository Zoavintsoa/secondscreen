package com.zoavintsoa.secondscreen

import java.nio.ByteBuffer
import java.nio.ByteOrder

data class ReassembledFrame(
    val frameId: Long,
    val codec: Int,
    val flags: Int,
    val timestampUs: Long,
    val annexB: ByteArray
)

class VideoFragmentReassembler(
    private val maxFrameBytes: Int = 16 * 1024 * 1024,
    private val timeoutMs: Long = 250
) {
    private data class Pending(
        val frameId: Long,
        val codec: Int,
        val flags: Int,
        val timestampUs: Long,
        val count: Int,
        val parts: Array<ByteArray?>,
        var bytes: Int,
        var updatedAt: Long
    )

    private var pending: Pending? = null
    private var newestFrameId: Long = -1L

    fun accept(datagram: ByteArray, nowMs: Long = System.currentTimeMillis()): ReassembledFrame? {
        if (datagram.size < 24) return null
        val b = ByteBuffer.wrap(datagram).order(ByteOrder.BIG_ENDIAN)
        if (b.int != 0x53535647) return null
        if (b.get().toInt() != 1) return null

        val codec = b.get().toInt() and 0xff
        if (codec != 1 && codec != 2) return null

        val flags = b.get().toInt() and 0xff
        b.get()
        val frameId = b.int.toLong() and 0xffffffffL
        val index = b.short.toInt() and 0xffff
        val count = b.short.toInt() and 0xffff
        val timestampUs = b.long

        if (count == 0 || index >= count || count > 4096) return null
        if (newestFrameId >= 0 && !isNewerOrEqual(frameId, newestFrameId)) return null

        val payload = ByteArray(b.remaining()).also { b.get(it) }
        if (payload.isEmpty()) return null

        if (pending?.let { nowMs - it.updatedAt > timeoutMs } == true) pending = null

        val active = pending
        if (active == null ||
            active.frameId != frameId ||
            active.count != count ||
            active.codec != codec ||
            active.flags != flags ||
            active.timestampUs != timestampUs
        ) {
            pending = Pending(frameId, codec, flags, timestampUs, count, arrayOfNulls(count), 0, nowMs)
        }

        val p = pending ?: return null
        if (p.parts[index] == null) {
            if (p.bytes + payload.size > maxFrameBytes) {
                pending = null
                return null
            }
            p.parts[index] = payload
            p.bytes += payload.size
        }
        p.updatedAt = nowMs

        if (p.parts.any { it == null }) return null

        val output = ByteArray(p.bytes)
        var offset = 0
        for (part in p.parts) {
            val bytes = part ?: return null
            bytes.copyInto(output, offset)
            offset += bytes.size
        }

        val result = ReassembledFrame(p.frameId, p.codec, p.flags, p.timestampUs, output)
        newestFrameId = p.frameId
        pending = null
        return result
    }

    fun reset() {
        pending = null
        newestFrameId = -1L
    }

    private fun isNewerOrEqual(candidate: Long, reference: Long): Boolean {
        if (candidate == reference) return true
        val diff = (candidate - reference) and 0xffffffffL
        return diff in 1L..0x7fffffffL
    }
}
