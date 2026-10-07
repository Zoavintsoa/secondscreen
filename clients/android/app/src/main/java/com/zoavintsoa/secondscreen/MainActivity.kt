package com.zoavintsoa.secondscreen

import android.app.AlertDialog
import android.os.Bundle
import android.text.InputType
import android.view.WindowManager
import android.widget.EditText
import android.widget.TextView
import androidx.activity.ComponentActivity
import androidx.activity.enableEdgeToEdge
import java.util.concurrent.CountDownLatch

class MainActivity : ComponentActivity() {
    private lateinit var status: TextView
    private lateinit var client: SecondScreenClient

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        setContentView(R.layout.activity_main)
        status = findViewById(R.id.status)
        val surface = findViewById<android.view.SurfaceView>(R.id.videoSurface)
        client = SecondScreenClient(this, surface.holder,
            { message -> runOnUiThread { status.text = message } },
            { hostName -> requestPairingCode(hostName) })
        surface.holder.addCallback(client)
    }

    private fun requestPairingCode(hostName: String): String? {
        var result: String? = null
        val latch = CountDownLatch(1)
        runOnUiThread {
            val input = EditText(this).apply {
                hint = "123456"
                inputType = InputType.TYPE_CLASS_NUMBER
                selectAll()
            }
            AlertDialog.Builder(this)
                .setTitle("Appairer SecondScreen")
                .setMessage("Entrez le code affiché sur l’ordinateur : $hostName")
                .setView(input)
                .setNegativeButton("Annuler") { _, _ -> latch.countDown() }
                .setPositiveButton("Appairer") { _, _ -> result = input.text.toString(); latch.countDown() }
                .setOnCancelListener { latch.countDown() }
                .show()
        }
        latch.await()
        return result?.takeIf { it.length == 6 }
    }

    override fun onDestroy() {
        client.close()
        super.onDestroy()
    }
}
