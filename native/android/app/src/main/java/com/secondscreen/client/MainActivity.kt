package com.secondscreen.client

import android.os.Bundle
import android.view.View
import android.view.WindowInsets
import android.view.WindowInsetsController
import android.view.WindowManager
import android.widget.Button
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import com.secondscreen.client.decoder.MediaCodecDecoder
import com.secondscreen.client.network.ClientState
import com.secondscreen.client.network.SecondScreenClient
import com.secondscreen.client.ui.SurfaceRenderView

class MainActivity : AppCompatActivity() {

    private lateinit var surfaceView: SurfaceRenderView
    private lateinit var pairingLayout: LinearLayout
    private lateinit var ipEditText: EditText
    private lateinit var pinEditText: EditText
    private lateinit var connectButton: Button
    private lateinit var statusTextView: TextView
    private lateinit var telemetryOverlay: TextView

    private var decoder: MediaCodecDecoder? = null
    private var client: SecondScreenClient? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)

        // Setup dynamic layout programmatically to avoid xml resource bundling issues
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            layoutParams = LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.MATCH_PARENT
            )
        }

        surfaceView = SurfaceRenderView(this).apply {
            layoutParams = LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                0,
                1.0f
            )
        }

        pairingLayout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(48, 48, 48, 48)
            setBackgroundColor(0xFF0F0F11.toInt())
            layoutParams = LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
            )
        }

        statusTextView = TextView(this).apply {
            text = "Enter Windows Host IP & 6-Digit PIN"
            setTextColor(0xFFD4D4D8.toInt())
            textSize = 16f
        }

        ipEditText = EditText(this).apply {
            hint = "Host IP (e.g. 192.168.1.145)"
            setTextColor(0xFFFFFFFF.toInt())
            setHintTextColor(0xFF71717A.toInt())
        }

        pinEditText = EditText(this).apply {
            hint = "6-Digit PIN"
            setTextColor(0xFFFFFFFF.toInt())
            setHintTextColor(0xFF71717A.toInt())
        }

        connectButton = Button(this).apply {
            text = "Connect & Stream"
            setBackgroundColor(0xFF2563EB.toInt())
            setTextColor(0xFFFFFFFF.toInt())
            setOnClickListener {
                val ip = ipEditText.text.toString().trim()
                val pin = pinEditText.text.toString().trim()
                if (ip.isNotEmpty() && pin.isNotEmpty()) {
                    statusTextView.text = "Connecting to $ip..."
                    client?.connect(ip, 9876, pin)
                }
            }
        }

        telemetryOverlay = TextView(this).apply {
            text = "FPS: N/A | Latency: N/A"
            setTextColor(0xFF10B981.toInt())
            visibility = View.GONE
        }

        pairingLayout.addView(statusTextView)
        pairingLayout.addView(ipEditText)
        pairingLayout.addView(pinEditText)
        pairingLayout.addView(connectButton)

        root.addView(surfaceView)
        root.addView(pairingLayout)
        root.addView(telemetryOverlay)

        setContentView(root)
        enableImmersiveMode()

        client = SecondScreenClient(
            onFrameReceived = { data, pts, isKey ->
                decoder?.feedEncodedNalu(data, pts, isKey)
            },
            onStateChanged = { state ->
                runOnUiThread {
                    when (state) {
                        ClientState.STREAMING -> {
                            pairingLayout.visibility = View.GONE
                            telemetryOverlay.visibility = View.VISIBLE
                            enableImmersiveMode()
                        }
                        ClientState.DISCONNECTED, ClientState.ERROR -> {
                            pairingLayout.visibility = View.VISIBLE
                            telemetryOverlay.visibility = View.GONE
                            statusTextView.text = "Disconnected. Re-enter PIN to reconnect."
                        }
                        else -> {
                            statusTextView.text = "Status: ${state.name}"
                        }
                    }
                }
            },
            onTelemetryReceived = { telem ->
                runOnUiThread {
                    if (telem.isMeasured) {
                        telemetryOverlay.text = "FPS: %.1f | RTT: %.1f ms | Latency: %.1f ms".format(
                            telem.fps, telem.rttMs, telem.totalLatencyMs
                        )
                    } else {
                        telemetryOverlay.text = "FPS: N/A | Latency: N/A (Not measured)"
                    }
                }
            }
        )

        surfaceView.onSurfaceReadyListener = { holder ->
            decoder = MediaCodecDecoder(holder.surface).apply {
                initialize(1920, 1080)
            }
        }

        surfaceView.onSurfaceDestroyedListener = {
            decoder?.release()
            decoder = null
        }

        surfaceView.onInputEventListener = { actionType, normX, normY, pressure, button ->
            client?.sendInputEvent(actionType, normX, normY, pressure, button)
        }

        // Try mDNS host discovery
        client?.discoverHosts { host ->
            runOnUiThread {
                ipEditText.setText(host.ip)
                statusTextView.text = "Discovered Host: ${host.name} (${host.ip})"
            }
        }
    }

    private fun enableImmersiveMode() {
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.R) {
            window.insetsController?.let { controller ->
                controller.hide(WindowInsets.Type.statusBars() or WindowInsets.Type.navigationBars())
                controller.systemBarsBehavior = WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
            }
        } else {
            @Suppress("DEPRECATION")
            window.decorView.systemUiVisibility = (
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                or View.SYSTEM_UI_FLAG_FULLSCREEN
                or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_LAYOUT_STABLE
            )
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        client?.disconnect()
        decoder?.release()
    }
}
