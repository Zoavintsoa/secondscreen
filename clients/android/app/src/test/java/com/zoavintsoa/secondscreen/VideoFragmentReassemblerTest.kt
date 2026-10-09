package com.zoavintsoa.secondscreen

import java.nio.ByteBuffer
import java.nio.ByteOrder
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class VideoFragmentReassemblerTest {
    @Test
    fun reassemblesOutOfOrderFragments() {
        val reassembler = VideoFragmentReassembler()
        assertNull(reassembler.accept(fragment(frameId = 7, index = 1, count = 2, payload = byteArrayOf(3, 4))))
        val frame = reassembler.accept(fragment(frameId = 7, index = 0, count = 2, payload = byteArrayOf(1, 2)))

        requireNotNull(frame)
        assertEquals(7L, frame.frameId)
        assertEquals(1, frame.codec)
        assertEquals(1, frame.flags and 1)
        assertEquals(1234L, frame.timestampUs)
        assertArrayEquals(byteArrayOf(1, 2, 3, 4), frame.annexB)
    }

    @Test
    fun rejectsInvalidAndOversizedFragments() {
        val reassembler = VideoFragmentReassembler(maxFrameBytes = 3)
        assertNull(reassembler.accept(byteArrayOf(1, 2, 3)))
        assertNull(reassembler.accept(fragment(frameId = 1, index = 0, count = 1, payload = byteArrayOf(1, 2, 3, 4))))
    }

    @Test
    fun ignoresDuplicateFragmentWithoutDuplicatingPayload() {
        val reassembler = VideoFragmentReassembler()
        val part = fragment(frameId = 9, index = 0, count = 2, payload = byteArrayOf(5, 6))
        assertNull(reassembler.accept(part))
        assertNull(reassembler.accept(part))
        val frame = reassembler.accept(fragment(frameId = 9, index = 1, count = 2, payload = byteArrayOf(7)))
        requireNotNull(frame)
        assertArrayEquals(byteArrayOf(5, 6, 7), frame.annexB)
    }

    private fun fragment(
        frameId: Int,
        index: Int,
        count: Int,
        payload: ByteArray
    ): ByteArray {
        val buffer = ByteBuffer.allocate(24 + payload.size).order(ByteOrder.BIG_ENDIAN)
        buffer.putInt(0x53535647) // SSVG
        buffer.put(1)             // protocol version
        buffer.put(1)             // H.264
        buffer.put(1)             // keyframe flag
        buffer.put(0)             // reserved
        buffer.putInt(frameId)
        buffer.putShort(index.toShort())
        buffer.putShort(count.toShort())
        buffer.putLong(1234L)
        buffer.put(payload)
        return buffer.array()
    }
}
