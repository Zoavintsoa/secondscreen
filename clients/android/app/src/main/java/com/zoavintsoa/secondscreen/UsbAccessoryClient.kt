package com.zoavintsoa.secondscreen

import android.content.Context
import android.hardware.usb.UsbAccessory
import android.hardware.usb.UsbManager
import java.io.BufferedInputStream
import java.io.DataInputStream
import java.io.IOException

class UsbAccessoryClient(private val context: Context) {
    private val manager = context.getSystemService(Context.USB_SERVICE) as UsbManager

    fun findSecondScreenAccessory(): UsbAccessory? =
        manager.accessoryList?.firstOrNull { it.manufacturer == MANUFACTURER && it.model == MODEL }

    fun hasPermission(accessory: UsbAccessory): Boolean = manager.hasPermission(accessory)

    fun consume(accessory: UsbAccessory, onFrame: (Int, Int, Long, ByteArray) -> Unit) {
        if (!manager.hasPermission(accessory)) throw SecurityException("Autorisation USB requise")
        val descriptor = manager.openAccessory(accessory) ?: throw IOException("Impossible d'ouvrir l'accessoire USB")
        descriptor.use {
            DataInputStream(BufferedInputStream(java.io.FileInputStream(it.fileDescriptor), 256 * 1024)).use { input ->
                while (true) {
                    val magic = ByteArray(4)
                    input.readFully(magic)
                    if (!magic.contentEquals(SSVF)) throw IOException("Trame USB inconnue")
                    val codec = input.readUnsignedByte()
                    val flags = input.readUnsignedByte()
                    input.readUnsignedShort()
                    val timestampUs = input.readLong()
                    val length = input.readInt()
                    require(length in 1..MAX_FRAME_SIZE) { "Taille USB invalide: $length" }
                    val payload = ByteArray(length)
                    input.readFully(payload)
                    onFrame(codec, flags, timestampUs, payload)
                }
            }
        }
    }

    companion object {
        const val MANUFACTURER = "Zoavintsoa"
        const val MODEL = "SecondScreen"
        const val VERSION = "1"
        const val URI = "https://github.com/Zoavintsoa/secondscreen"
        private val SSVF = byteArrayOf('S'.code.toByte(), 'S'.code.toByte(), 'V'.code.toByte(), 'F'.code.toByte())
        private const val MAX_FRAME_SIZE = 16 * 1024 * 1024
    }
}
