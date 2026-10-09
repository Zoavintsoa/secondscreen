package com.zoavintsoa.secondscreen

import org.junit.Assert.assertEquals
import org.junit.Test

class StreamConfigTest {
    @Test
    fun parseReadsH264Configuration() {
        val config = StreamConfig.parse(
            """{"width":1920,"height":1080,"fps":60,"bitrateKbps":12000,"profile":"Creator","codec":"h264"}"""
        )

        assertEquals(1920, config.width)
        assertEquals(1080, config.height)
        assertEquals(60, config.fps)
        assertEquals(12000, config.bitrateKbps)
        assertEquals("Creator", config.profile)
        assertEquals(1, config.codec)
    }

    @Test
    fun normalizeClampsValuesToSupportedSafetyBounds() {
        val config = StreamConfig(
            width = 10000,
            height = 9000,
            fps = 999,
            bitrateKbps = 999999,
            codec = 99
        ).normalized()

        assertEquals(3840, config.width)
        assertEquals(2160, config.height)
        assertEquals(120, config.fps)
        assertEquals(50000, config.bitrateKbps)
        assertEquals(1, config.codec)
    }

    @Test
    fun parseSupportsHevcAndNumericCodecIds() {
        assertEquals(2, StreamConfig.parse("""{"codec":"hevc"}""").codec)
        assertEquals(2, StreamConfig.parse("""{"codec":2}""").codec)
    }
}
