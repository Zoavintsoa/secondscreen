package com.zoavintsoa.secondscreen

import android.app.AlertDialog
import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.hardware.usb.UsbManager
import android.os.Bundle
import android.text.InputType
import android.view.WindowManager
import android.widget.EditText
import android.widget.ScrollView
import android.widget.TextView
import androidx.activity.ComponentActivity
import androidx.activity.enableEdgeToEdge
import java.util.concurrent.CountDownLatch

class MainActivity : ComponentActivity() {
    private lateinit var status: TextView
    private lateinit var connectionModeButton: android.widget.Button
    private var client: SecondScreenClient? = null

    private val usbPermissionReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            if (intent.action != ACTION_USB_PERMISSION) return
            val granted = intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)
            runOnUiThread { status.text = if (granted) "SecondScreen — USB autorisé, connexion…" else "SecondScreen — autorisation USB refusée" }
            client?.reconnectNow()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        setContentView(R.layout.activity_main)
        status = findViewById(R.id.status)
        connectionModeButton = findViewById(R.id.connectionModeButton)
        registerReceiver(usbPermissionReceiver, IntentFilter(ACTION_USB_PERMISSION), Context.RECEIVER_NOT_EXPORTED)
        requestUsbPermissionIfNeeded()

        findViewById<android.widget.Button>(R.id.aboutButton).setOnClickListener {
            showAbout()
        }

        findViewById<android.widget.Button>(R.id.reconnectButton).setOnClickListener {
            client?.reconnectNow()
        }

        connectionModeButton.setOnClickListener {
            showConnectionMode()
        }

        updateConnectionModeButton()

        if (legalAccepted()) {
            initializeClient()
        } else {
            window.decorView.post { showLegalNotice() }
        }
    }

    private fun legalAccepted(): Boolean =
        getPreferences(Context.MODE_PRIVATE).getBoolean("legal_accepted_v1", false)

    private fun requestUsbPermissionIfNeeded() {
        val manager = getSystemService(Context.USB_SERVICE) as UsbManager
        val accessory = manager.accessoryList?.firstOrNull {
            it.manufacturer == UsbAccessoryClient.MANUFACTURER && it.model == UsbAccessoryClient.MODEL
        } ?: return
        if (manager.hasPermission(accessory)) return
        val intent = Intent(ACTION_USB_PERMISSION).setPackage(packageName)
        val flags = PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        manager.requestPermission(accessory, PendingIntent.getBroadcast(this, 1001, intent, flags))
    }

    override fun onResume() {
        super.onResume()
        requestUsbPermissionIfNeeded()
        client?.reconnectNow()
    }

    private fun initializeClient() {
        if (client != null) return
        val surface = findViewById<android.view.SurfaceView>(R.id.videoSurface)
        client = SecondScreenClient(
            this,
            surface.holder,
            { message -> runOnUiThread { status.text = message } },
            { hostName -> requestPairingCode(hostName) }
        )
        updateConnectionModeButton()
        surface.holder.addCallback(client)
    }

    private fun showConnectionMode() {
        val current = client?.connectionMode ?: SecondScreenClient.ConnectionMode.AUTO
        val labels = arrayOf(
            "Automatique — USB si disponible, sinon Wi-Fi",
            "Wi-Fi / LAN",
            "USB — liaison filaire"
        )
        val checked = when (current) {
            SecondScreenClient.ConnectionMode.AUTO -> 0
            SecondScreenClient.ConnectionMode.WIFI -> 1
            SecondScreenClient.ConnectionMode.USB -> 2
        }
        AlertDialog.Builder(this)
            .setTitle("Mode de connexion")
            .setSingleChoiceItems(labels, checked) { dialog, which ->
                val mode = when (which) {
                    1 -> SecondScreenClient.ConnectionMode.WIFI
                    2 -> SecondScreenClient.ConnectionMode.USB
                    else -> SecondScreenClient.ConnectionMode.AUTO
                }
                client?.setConnectionMode(mode)
                updateConnectionModeButton()
                dialog.dismiss()
            }
            .setNegativeButton("Annuler", null)
            .show()
    }

    private fun updateConnectionModeButton() {
        val mode = client?.connectionMode ?: SecondScreenClient.ConnectionMode.AUTO
        connectionModeButton.text = when (mode) {
            SecondScreenClient.ConnectionMode.AUTO -> "Connexion : Auto"
            SecondScreenClient.ConnectionMode.WIFI -> "Connexion : Wi-Fi"
            SecondScreenClient.ConnectionMode.USB -> "Connexion : USB"
        }
    }

    private fun showLegalNotice() {
        AlertDialog.Builder(this)
            .setTitle("SecondScreen — Zoavintsoa")
            .setMessage(
                "Avant d’utiliser SecondScreen, veuillez consulter et accepter les Conditions d’utilisation et la Politique de confidentialité. " +
                    "Le fonctionnement normal est local/LAN et aucune création de compte n’est requise."
            )
            .setNeutralButton("Conditions") { _, _ -> showLegalDocument("Conditions d’utilisation", TERMS) }
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
                "SecondScreen — Zoavintsoa\n\n" +
                    "Your devices become your workspace.\n\n" +
                    "Créé et développé par Zoavintsoa.\n" +
                    "Native • Cross-platform • Local-first\n" +
                    "Android • Windows • macOS • iPadOS\n\n" +
                    "Version 0.1.0"
            )
            .setNeutralButton("Confidentialité") { _, _ ->
                showLegalDocument("Politique de confidentialité", PRIVACY)
            }
            .setNegativeButton("Conditions") { _, _ ->
                showLegalDocument("Conditions d’utilisation", TERMS)
            }
            .setPositiveButton("Fermer", null)
            .show()
    }

    private fun showLegalDocument(title: String, body: String) {
        val textView = TextView(this).apply {
            text = body
            textSize = 14f
            setPadding(32, 24, 32, 24)
            setTextIsSelectable(true)
        }
        val scroll = ScrollView(this).apply { addView(textView) }
        AlertDialog.Builder(this)
            .setTitle(title)
            .setView(scroll)
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
        return result?.takeIf { it.length == 6 && it.all(Char::isDigit) }
    }

    override fun onDestroy() {
        client?.close()
        client = null
        runCatching { unregisterReceiver(usbPermissionReceiver) }
        super.onDestroy()
    }

    private companion object {
        const val ACTION_USB_PERMISSION = "com.zoavintsoa.secondscreen.USB_PERMISSION"
        const val TERMS = """
SecondScreen — Conditions d’utilisation

1. Objet
SecondScreen est un logiciel de deuxième écran et d’espace de travail local développé par Zoavintsoa.

2. Utilisation
Vous êtes responsable de l’utilisation du logiciel, des appareils connectés et des contenus affichés ou transmis. Utilisez uniquement des appareils et réseaux pour lesquels vous disposez des autorisations nécessaires.

3. Réseau et appairage
SecondScreen privilégie les connexions locales. L’appairage repose sur une confirmation utilisateur et un identifiant de session. Ne communiquez pas un code d’appairage à une personne non autorisée.

4. Disponibilité
Le produit est en développement. Certaines fonctions peuvent être expérimentales, indisponibles selon l’OS ou le matériel, ou nécessiter une validation physique.

5. Responsabilité
Dans les limites permises par la loi applicable, le logiciel est fourni sans garantie de disponibilité, de compatibilité universelle ou d’absence d’erreur pendant cette phase de développement.

6. Propriété
SecondScreen — Zoavintsoa et les éléments originaux du projet restent soumis à leurs droits et licences applicables. Les composants tiers conservent leurs propres licences et notices.

7. Acceptation
L’utilisation de l’application après acceptation signifie que vous avez pris connaissance de ces conditions.
"""

        const val PRIVACY = """
SecondScreen — Politique de confidentialité

1. Principe
SecondScreen est conçu selon une approche local-first. Le fonctionnement normal ne nécessite pas de compte ni de service cloud.

2. Données locales
L’application peut conserver localement des informations nécessaires au fonctionnement, notamment un identifiant d’appareil et un jeton de session d’appairage.

3. Réseau
Les données de deuxième écran sont destinées au réseau local entre l’hôte et le client. SecondScreen ne vend pas les données utilisateur et n’ajoute pas de publicité ou d’analytique cachée dans le produit.

4. Caméra et microphone
Ces capteurs ne doivent être utilisés que lorsqu’une fonction correspondante est explicitement activée et après autorisation du système.

5. Permissions
Les permissions Android sont demandées uniquement lorsqu’une fonction les nécessite. Vous pouvez les gérer dans les réglages du système.

6. Conservation
Les données locales restent sur l’appareil jusqu’à leur suppression ou leur remplacement par l’application ou l’utilisateur.

7. Évolutions
Cette politique peut être mise à jour lorsque de nouvelles fonctions sont ajoutées. La version complète du projet est disponible dans PRIVACY.md.
"""
    }
}
