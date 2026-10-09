package com.zoavintsoa.secondscreen

import android.content.Context
import android.view.SurfaceHolder
import java.io.BufferedInputStream
import java.io.DataInputStream
import java.net.InetSocketAddress
import java.net.Socket
import java.util.UUID
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicBoolean

class SecondScreenClient(
    private val context: Context,
    private val surface: SurfaceHolder,
    private val onStatus: (String) -> Unit,
    private val onPairingRequired: (String) -> String?
) : SurfaceHolder.Callback {

    enum class ConnectionMode { AUTO, WIFI, USB }

    private val executor = Executors.newSingleThreadExecutor()
    private val running = AtomicBoolean(false)
    private val reconnectRequested = AtomicBoolean(false)

    private var socket: Socket? = null
    private var controlSocket: Socket? = null
    private var controlClient: ControlClient? = null
    private var decoder: VideoStreamDecoder? = null
    private var discoveredHost: HostAdvertisement? = null
    private var streamConfig = StreamConfig()
    private val reassembler = VideoFragmentReassembler()
    private var awaitingKeyframe = true
    private val usbAccessoryClient = UsbAccessoryClient(context)

    private val preferences =
        context.getSharedPreferences("secondscreen", Context.MODE_PRIVATE)

    private var _connectionMode: ConnectionMode =
        runCatching {
            ConnectionMode.valueOf(
                preferences.getString("connectionMode", ConnectionMode.AUTO.name)
                    ?: ConnectionMode.AUTO.name
            )
        }.getOrDefault(ConnectionMode.AUTO)

    val connectionMode: ConnectionMode
        get() = _connectionMode

    private val deviceId: String by lazy {
        preferences.getString("deviceId", null) ?: UUID.randomUUID().toString().also {
            preferences.edit().putString("deviceId", it).apply()
        }
    }

    override fun surfaceCreated(holder: SurfaceHolder) = start()

    override fun surfaceChanged(
        holder: SurfaceHolder,
        format: Int,
        width: Int,
        height: Int
    ) = Unit

    override fun surfaceDestroyed(holder: SurfaceHolder) = stop()

    fun setConnectionMode(mode: ConnectionMode) {
        if (_connectionMode == mode) return
        _connectionMode = mode
        preferences.edit().putString("connectionMode", mode.name).apply()
        reconnectNow()
    }

    fun reconnectNow() {
        if (!running.get()) return
        reconnectRequested.set(true)
        socket?.runCatching { close() }
        controlSocket?.runCatching { close() }
        onStatus("SecondScreen — nouvelle recherche…")
    }

    private fun start() {
        if (!running.compareAndSet(false, true)) return
        reconnectRequested.set(true)
        executor.execute { connectLoop() }
    }

    private fun openControl(host: HostAdvertisement): ControlClient {
        val cs = Socket().apply {
            tcpNoDelay = true
            keepAlive = true
            connect(InetSocketAddress(host.address, host.controlPort), 1500)
            soTimeout = 5000
        }
        controlSocket = cs
        return ControlClient(cs).also { controlClient = it }
    }

    private fun connectLoop() {
        while (running.get()) {
            try {
                reconnectRequested.set(false)

                if (_connectionMode == ConnectionMode.USB || _connectionMode == ConnectionMode.AUTO) {
                    val usbConnected = runCatching { consumeUsbIfAvailable() }.getOrElse { error ->
                        if (_connectionMode == ConnectionMode.USB) {
                            onStatus("SecondScreen — USB : " + (error.message ?: "connexion indisponible"))
                        }
                        false
                    }
                    if (usbConnected) continue
                    if (_connectionMode == ConnectionMode.USB) {
                        Thread.sleep(500)
                        continue
                    }
                }

                onStatus("SecondScreen — recherche d’un hôte…")
                val host = discoverHost() ?: error("Aucun hôte SecondScreen trouvé")

                val capabilities = DeviceCapabilitiesProbe.probe(context)
                if (capabilities.codecs.none { it.codec == 1 }) {
                    error("Aucun décodeur H.264 disponible sur cet Android")
                }

                val testMode = host.mode == "test"
                if (testMode) {
                    streamConfig = streamConfig.copy(
                        width = host.width,
                        height = host.height,
                        fps = host.fps,
                        codec = 1
                    ).normalized()
                    onStatus("SecondScreen — iMac détecté, connexion vidéo…")
                } else {
                    var cc = openControl(host)

                    if (!cc.hello(
                            deviceId,
                            "Android SecondScreen",
                            DeviceCapabilitiesProbe.helloJson(capabilities)
                        )
                    ) {
                        error("HELLO de contrôle refusé")
                    }

                    var authenticated = false
                    val stored = preferences.getString("sessionToken", null)

                    if (!stored.isNullOrBlank()) {
                        val result = runCatching { cc.authenticate(stored) }.getOrNull()
                        if (result?.authenticated == true) {
                            authenticated = true
                            result.streamConfig?.let { streamConfig = it.normalized() }
                        } else {
                            closeSocketsOnly()
                            cc = openControl(host)
                            if (!cc.hello(
                                    deviceId,
                                    "Android SecondScreen",
                                    DeviceCapabilitiesProbe.helloJson(capabilities)
                                )
                            ) {
                                error("HELLO de contrôle refusé après renouvellement")
                            }
                        }
                    }

                    if (!authenticated) {
                        onStatus("SecondScreen — appairage requis")
                        val code = onPairingRequired(host.name)
                            ?: error("Appairage annulé")
                        val result = cc.pair(code.trim())
                        if (!result.authenticated || result.sessionToken.isNullOrBlank()) {
                            error("Appairage refusé")
                        }
                        preferences.edit()
                            .putString("sessionToken", result.sessionToken)
                            .apply()
                        result.streamConfig?.let { streamConfig = it.normalized() }
                    }
                }

                awaitingKeyframe = true
                reassembler.reset()

                val s = Socket().apply {
                    tcpNoDelay = true
                    keepAlive = true
                    receiveBufferSize = 512 * 1024
                    connect(InetSocketAddress(host.address, host.videoPort), 1500)
                }
                socket = s

                preferences.edit()
                    .putString("lastHostAddress", host.address)
                    .putString("lastHostName", host.name)
                    .apply()

                onStatus(
                    if (testMode) "SecondScreen — iMac connecté"
                    else "SecondScreen — sécurisé, vidéo en cours…"
                )

                consumeVideo(DataInputStream(BufferedInputStream(s.getInputStream())))
            } catch (t: Throwable) {
                if (!running.get()) break
                if (t !is InterruptedException) {
                    onStatus(
                        if (_connectionMode == ConnectionMode.USB)
                            "SecondScreen — USB indisponible"
                        else
                            "SecondScreen — reconnexion…"
                    )
                }
                if (running.get()) {
                    try {
                        Thread.sleep(if (reconnectRequested.get()) 150L else 1000L)
                    } catch (_: InterruptedException) {
                        Thread.currentThread().interrupt()
                        break
                    }
                }
            } finally {
                closeSocketsOnly()
                decoder?.release()
                decoder = null
                reassembler.reset()
                awaitingKeyframe = true
            }
        }
    }

    fun acceptQuicDatagram(datagram: ByteArray) {
        if (!running.get() || _connectionMode == ConnectionMode.USB) return
        val frame = reassembler.accept(datagram) ?: return
        val keyFrame = (frame.flags and 1) != 0
        if (awaitingKeyframe && !keyFrame) return
        if (keyFrame) awaitingKeyframe = false
        streamConfig = streamConfig.copy(codec = frame.codec).normalized()
        decode(
            frame.codec,
            streamConfig.width,
            streamConfig.height,
            frame.annexB,
            frame.timestampUs,
            keyFrame
        )
    }

    private fun consumeUsbIfAvailable(): Boolean {
        val accessory = usbAccessoryClient.findSecondScreenAccessory() ?: return false
        if (!usbAccessoryClient.hasPermission(accessory)) {
            onStatus("SecondScreen — USB : autorisation système requise")
            return false
        }
        awaitingKeyframe = true
        reassembler.reset()
        onStatus("SecondScreen — USB connecté, vidéo directe…")
        usbAccessoryClient.consume(accessory) { codec, flags, timestampUs, payload ->
            val keyFrame = (flags and 1) != 0
            if (awaitingKeyframe && !keyFrame) return@consume
            if (keyFrame) awaitingKeyframe = false
            decode(codec, streamConfig.width, streamConfig.height, payload, timestampUs, keyFrame)
        }
        return true
    }

    private fun discoverHost(): HostAdvertisement? {
        val cachedAddress = preferences.getString("lastHostAddress", null)
        val cachedName =
            preferences.getString("lastHostName", "SecondScreen Host") ?: "SecondScreen Host"

        val fresh = DiscoveryClient().discover(5000)
        if (fresh != null) {
            discoveredHost = fresh
            return fresh
        }

        if (_connectionMode == ConnectionMode.AUTO && !cachedAddress.isNullOrBlank()) {
            val cached = HostAdvertisement(
                address = cachedAddress,
                name = cachedName,
                controlPort = 49152,
                videoPort = 49153,
                mode = "test"
            )
            if (probeVideo(cached)) {
                discoveredHost = cached
                return cached
            }
        }
        return null
    }

    private fun probeVideo(host: HostAdvertisement): Boolean =
        runCatching {
            Socket().use {
                it.tcpNoDelay = true
                it.connect(InetSocketAddress(host.address, host.videoPort), 500)
            }
            true
        }.getOrDefault(false)

    private fun consumeVideo(input: DataInputStream) {
        while (running.get() && !reconnectRequested.get()) {
            val magic = ByteArray(4)
            input.readFully(magic)

            when (String(magic, Charsets.US_ASCII)) {
                "SSVF" -> {
                    val codec = input.readUnsignedByte()
                    val flags = input.readUnsignedByte()
                    input.readUnsignedShort()
                    val timestamp = input.readLong()
                    val length = input.readInt()
                    require(length in 1..16_777_216)

                    val au = ByteArray(length)
                    input.readFully(au)

                    val keyFrame = (flags and 1) != 0
                    if (awaitingKeyframe && !keyFrame) continue
                    if (keyFrame) awaitingKeyframe = false

                    decode(
                        codec,
                        streamConfig.width,
                        streamConfig.height,
                        au,
                        timestamp,
                        keyFrame
                    )
                }

                "SSVG" -> {
                    controlClient?.requestKeyframe()
                    error("SSVG nécessite le transport datagramme QUIC")
                }

                else -> error("Trame vidéo inconnue")
            }
        }
    }

    private fun decode(
        codec: Int,
        width: Int,
        height: Int,
        au: ByteArray,
        timestampUs: Long,
        keyFrame: Boolean
    ) {
        if (decoder == null) decoder = VideoStreamDecoder(surface)

        runCatching {
            decoder?.decode(codec, width, height, au, timestampUs, keyFrame)
        }.onFailure {
            awaitingKeyframe = true
            controlClient?.requestKeyframe()
        }
    }

    fun close() {
        stop()
        executor.shutdownNow()
    }

    private fun stop() {
        running.set(false)
        reconnectRequested.set(true)
        closeSocketsOnly()
        decoder?.release()
        decoder = null
    }

    private fun closeSocketsOnly() {
        socket?.runCatching { close() }
        socket = null
        controlClient?.runCatching { close() }
        controlClient = null
        controlSocket?.runCatching { close() }
        controlSocket = null
    }
}
