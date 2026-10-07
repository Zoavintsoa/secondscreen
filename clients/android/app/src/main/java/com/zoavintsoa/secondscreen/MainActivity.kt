package com.zoavintsoa.secondscreen

import android.os.Bundle
import android.view.WindowManager
import android.widget.TextView
import androidx.activity.ComponentActivity
import androidx.activity.enableEdgeToEdge

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

        client = SecondScreenClient(
            surface = surface,
            onStatus = { message -> runOnUiThread { status.text = message } }
        )
        surface.holder.addCallback(client)
    }

    override fun onDestroy() {
        client.close()
        super.onDestroy()
    }
}
