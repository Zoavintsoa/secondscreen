package com.zoavintsoa.secondscreen

import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.InetSocketAddress
import java.net.SocketTimeoutException
import org.json.JSONObject

data class HostAdvertisement(
    val address: String,
    val name: String,
    val controlPort: Int,
    val videoPort: Int,
    val mode: String = "production",
    val width: Int = 1920,
    val height: Int = 1080,
    val fps: Int = 60
)

class DiscoveryClient {
    fun discover(timeoutMs: Int = 5000): HostAdvertisement? {
        DatagramSocket(null).use { socket ->
            socket.reuseAddress = true
            socket.broadcast = true
            socket.soTimeout = 500
            socket.bind(InetSocketAddress(49151))

            val buffer = ByteArray(4096)
            val deadline = System.currentTimeMillis() + timeoutMs

            while (System.currentTimeMillis() < deadline) {
                val remaining = (deadline - System.currentTimeMillis()).coerceAtLeast(1L)
                socket.soTimeout = minOf(500L, remaining).toInt()

                try {
                    val packet = DatagramPacket(buffer, buffer.size)
                    socket.receive(packet)

                    val json = runCatching {
                        JSONObject(String(packet.data, packet.offset, packet.length, Charsets.UTF_8))
                    }.getOrNull() ?: continue

                    if (json.optString("service") != "secondscreen") continue

                    val address = packet.address.hostAddress ?: continue
                    val controlPort = json.optInt("controlPort", 49152)
                    val videoPort = json.optInt("videoPort", 49153)

                    if (controlPort !in 1..65535 || videoPort !in 1..65535) continue

                    return HostAdvertisement(
                        address = address,
                        name = json.optString("name", "SecondScreen Host"),
                        controlPort = controlPort,
                        videoPort = videoPort,
                        mode = json.optString("mode", "production"),
                        width = json.optInt("width", 1920).coerceIn(16, 7680),
                        height = json.optInt("height", 1080).coerceIn(16, 4320),
                        fps = json.optInt("fps", 60).coerceIn(1, 240)
                    )
                } catch (_: SocketTimeoutException) {
                    // Continue listening until the complete discovery window expires.
                }
            }
            return null
        }
    }
}
