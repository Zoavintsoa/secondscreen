package com.zoavintsoa.secondscreen

import android.content.Context
import android.hardware.usb.UsbAccessory
import android.hardware.usb.UsbManager
import java.io.BufferedInputStream
import java.io.DataInputStream
import java.io.EOFException
import java.io.FileInputStream
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

        descriptor.use { parcel ->
            DataInputStream(BufferedInputStream(FileInputStream(parcel.fileDescriptor))).use { input ->
                // Android USB accessory I/O is a byte stream: read() is not guaranteed
                // to return one complete SSVF frame. Read the fixed header first, then
                // exactly the payload length advertised by that header.
                val header = ByteArray(HEADER_SIZE)
                while (true) {
                    try {
                        input.readFully(header)
                    } catch (e: EOFException) {
                        throw IOException("Connexion USB interrompue pendant l'en-tête vidéo", e)
                    }

                    if (header[0] != 'S'.code.toByte() ||
                        header[1] != 'S'.code.toByte() ||
                        header[2] != 'V'.code.toByte() ||
                        header[3] != 'F'.code.toByte()
                    ) throw IOException("Signature de trame USB inconnue")

                    val codec = header[4].toInt() and 0xFF
                    val flags = header[5].toInt() and 0xFF
                    val reserved = ((header[6].toInt() and 0xFF) shl 8) or
                        (header[7].toInt() and 0xFF)
                    val timestampUs = readLongBE(header, 8)
                    val length = readIntBE(header, 16)

                    if (codec != CODEC_H264 && codec != CODEC_HEVC) {
                        throw IOException("Codec vidéo USB non pris en charge: $codec")
                    }
                    if (reserved != 0) throw IOException("En-tête USB invalide: champs réservés non nuls")
                    if (length !in 1..MAX_FRAME_SIZE) {
                        throw IOException("Taille de trame USB invalide: $length")
                    }

                    val payload = ByteArray(length)
                    try {
                        input.readFully(payload)
                    } catch (e: EOFException) {
                        throw IOException("Trame USB tronquée: charge utile incomplète", e)
                    }
                    onFrame(codec, flags, timestampUs, payload)
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
        private const val CODEC_H264 = 1
        private const val CODEC_HEVC = 2
    }
}
