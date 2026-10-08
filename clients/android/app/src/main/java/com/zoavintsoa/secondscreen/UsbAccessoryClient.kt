package com.zoavintsoa.secondscreen

import android.content.Context
import android.hardware.usb.UsbAccessory
import android.hardware.usb.UsbManager
import java.io.IOException

class UsbAccessoryClient(private val context: Context) {
    private val manager = context.getSystemService(Context.USB_SERVICE) as UsbManager

    fun findSecondScreenAccessory(): UsbAccessory? =
        manager.accessoryList?.firstOrNull { it.manufacturer == MANUFACTURER && it.model == MODEL }

    fun hasPermission(accessory: UsbAccessory): Boolean = manager.hasPermission(accessory)

    fun consume(accessory: UsbAccessory, onFrame: (Int, Int, Long, ByteArray) -> Unit) {
        if (!manager.hasPermission(accessory)) throw SecurityException("Autorisation USB requise")
        val descriptor = manager.openAccessory(accessory)
            ?: throw IOException("Impossible d'ouvrir l'accessoire USB")

        descriptor.use {
            java.io.FileInputStream(it.fileDescriptor).use { input ->
                // Android's accessory API requires the complete USB transfer to be
                // consumed in one read. The macOS transport sends exactly one SSVF
                // frame per bulk transfer, so the buffer is deliberately large enough
                // for the protocol's 16 MiB maximum frame.
                val transfer = ByteArray(MAX_FRAME_SIZE + HEADER_SIZE)
                while (true) {
                    val count = input.read(transfer)
                    if (count < HEADER_SIZE) throw IOException("Transfert USB incomplet")

                    if (transfer[0] != 'S'.code.toByte() ||
                        transfer[1] != 'S'.code.toByte() ||
                        transfer[2] != 'V'.code.toByte() ||
                        transfer[3] != 'F'.code.toByte()
                    ) throw IOException("Trame USB inconnue")

                    val codec = transfer[4].toInt() and 0xFF
                    val flags = transfer[5].toInt() and 0xFF
                    val timestampUs = readLongBE(transfer, 8)
                    val length = readIntBE(transfer, 16)

                    require(length in 1..MAX_FRAME_SIZE) { "Taille USB invalide: " + length }
                    require(HEADER_SIZE + length == count) {
                        "Trame USB tronquée: reçu=" + count + " attendu=" + (HEADER_SIZE + length)
                    }

                    onFrame(codec, flags, timestampUs,
                        transfer.copyOfRange(HEADER_SIZE, HEADER_SIZE + length))
                }
            }
        }
    }

    private fun readIntBE(data: ByteArray, offset: Int): Int =
        ((data[offset].toInt() and 0xFF) shl 24) or
            ((data[offset + 1].toInt() and 0xFF) shl 16) or
            ((data[offset + 2].toInt() and 0xFF) shl 8) or
            (data[offset + 3].toInt() and 0xFF)

    private fun readLongBE(data: ByteArray, offset: Int): Long {
        var result = 0L
        for (i in 0 until 8) result = (result shl 8) or (data[offset + i].toLong() and 0xFF)
        return result
    }

    companion object {
        const val MANUFACTURER = "Zoavintsoa"
        const val MODEL = "SecondScreen"
        const val VERSION = "1"
        const val URI = "https://github.com/Zoavintsoa/secondscreen"
        private const val HEADER_SIZE = 20
        private const val MAX_FRAME_SIZE = 16 * 1024 * 1024
    }
}
