package com.zoavintsoa.secondscreen

import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.SocketTimeoutException
import org.json.JSONObject

data class HostAdvertisement(val address: String, val name: String, val controlPort: Int)

class DiscoveryClient {
    fun discover(timeoutMs: Int = 1500): HostAdvertisement? {
        DatagramSocket(49151).use { socket ->
            socket.broadcast = true
            socket.soTimeout = timeoutMs
            val buffer = ByteArray(4096)
            return try {
                val packet = DatagramPacket(buffer, buffer.size)
                socket.receive(packet)
                val json = JSONObject(String(packet.data, 0, packet.length, Charsets.UTF_8))
                if (json.optString("service") != "secondscreen") return null
                HostAdvertisement(
                    packet.address.hostAddress ?: return null,
                    json.optString("name", "SecondScreen Host"),
                    json.getInt("controlPort")
                )
            } catch (_: SocketTimeoutException) {
                null
            }
        }
    }
}
