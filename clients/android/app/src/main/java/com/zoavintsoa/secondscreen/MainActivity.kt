package com.zoavintsoa.secondscreen

import android.app.AlertDialog
import android.content.Context
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
    private var client: SecondScreenClient? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        setContentView(R.layout.activity_main)
        status = findViewById(R.id.status)

        findViewById<android.widget.Button>(R.id.aboutButton).setOnClickListener {
            showAbout()
        }

        if (legalAccepted()) {
            initializeClient()
        } else {
            window.decorView.post { showLegalNotice() }
        }
    }

    private fun legalAccepted(): Boolean =
        getPreferences(Context.MODE_PRIVATE).getBoolean("legal_accepted_v1", false)

    private fun initializeClient() {
        val surface = findViewById<android.view.SurfaceView>(R.id.videoSurface)
        client = SecondScreenClient(
            this,
            surface.holder,
            { message -> runOnUiThread { status.text = message } },
            { hostName -> requestPairingCode(hostName) }
        )
        surface.holder.addCallback(client)
    }

    private fun showLegalNotice() {
        AlertDialog.Builder(this)
            .setTitle("SecondScreen — Zoavintsoa")
            .setMessage(
                "Avant d’utiliser SecondScreen, veuillez accepter les Conditions d’utilisation et la Politique de confidentialité. " +
                    "SecondScreen privilégie les connexions locales et demande uniquement les permissions nécessaires aux fonctions activées."
            )
            .setNegativeButton("Quitter") { _, _ -> finish() }
            .setPositiveButton("J’accepte") { _, _ ->
                getPreferences(Context.MODE_PRIVATE)
                    .edit()
                    .putBoolean("legal_accepted_v1", true)
                    .apply()
                initializeClient()
            }
            .setCancelable(false)
            .show()
    }

    private fun showAbout() {
        AlertDialog.Builder(this)
            .setTitle("À propos de SecondScreen")
            .setMessage(
                "SecondScreen\n\n" +
                    "Your devices become your workspace.\n\n" +
                    "Créé et développé par Zoavintsoa.\n" +
                    "Native • Cross-platform • Local-first\n\n" +
                    "Android • Windows • macOS • iPadOS"
            )
            .setPositiveButton("Fermer", null)
            .show()
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
                .setPositiveButton("Appairer") { _, _ ->
                    result = input.text.toString()
                    latch.countDown()
                }
                .setOnCancelListener { latch.countDown() }
                .show()
        }
        latch.await()
        return result?.takeIf { it.length == 6 }
    }

    override fun onDestroy() {
        client?.close()
        client = null
        super.onDestroy()
    }
}
