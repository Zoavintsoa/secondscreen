package com.zoavintsoa.secondscreen

import java.io.DataInputStream
import java.io.DataOutputStream
import java.net.Socket
import java.nio.ByteBuffer
import java.nio.ByteOrder

data class ControlResult(val authenticated: Boolean, val sessionToken: String? = null)

class ControlClient(private val socket: Socket) {
    private val input = DataInputStream(socket.getInputStream())
    private val output = DataOutputStream(socket.getOutputStream())

    fun hello(deviceId: String, deviceName: String): Boolean {
        send(0x01, """{"deviceId":"""" + escape(deviceId) + """","deviceName":"""" + escape(deviceName) + """"}""")
        return receive()?.type == 0x04
    }

    fun authenticate(token: String): Boolean {
        send(0x0A, """{"sessionToken":"""" + escape(token) + """"}""")
        return receive()?.type == 0x05
    }

    fun pair(code: String): ControlResult {
        send(0x02, """{"code":"""" + escape(code) + """"}""")
        val response = receive() ?: return ControlResult(false)
        if (response.type != 0x03 || !response.json.contains(""""status":"paired"""")) return ControlResult(false)
        val token = Regex(""""sessionToken"\s*:\s*"([^"]{64})"""").find(response.json)?.groupValues?.get(1)
        return ControlResult(token != null, token)
    }

    fun close() {
        runCatching { send(0x08, "{}") }
        runCatching { socket.close() }
    }

    private fun send(type: Int, json: String) {
        val payload = json.toByteArray(Charsets.UTF_8)
        require(payload.size <= 64 * 1024)
        val frame = ByteBuffer.allocate(12 + payload.size).order(ByteOrder.BIG_ENDIAN)
        frame.put(byteArrayOf('S'.code.toByte(), 'S'.code.toByte(), 'C'.code.toByte(), 'P'.code.toByte()))
        frame.put(1); frame.put(type.toByte()); frame.putShort(0); frame.putInt(payload.size); frame.put(payload)
        output.write(frame.array()); output.flush()
    }

    private fun receive(): ControlResponse? {
        val magic = ByteArray(4); input.readFully(magic)
        require(magic.contentEquals(byteArrayOf('S'.code.toByte(), 'S'.code.toByte(), 'C'.code.toByte(), 'P'.code.toByte())))
        require(input.readUnsignedByte() == 1)
        val type = input.readUnsignedByte()
        input.readUnsignedShort()
        val length = input.readInt()
        require(length in 0..(64 * 1024))
        val payload = ByteArray(length); input.readFully(payload)
        return ControlResponse(type, payload.toString(Charsets.UTF_8))
    }

    private fun escape(value: String) = value.replace("\\", "\\\\").replace("\"", "\\\"")
    private data class ControlResponse(val type: Int, val json: String)
}
