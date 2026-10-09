import AppKit
import Dispatch

@main
final class SecondScreenMacHostDelegate: NSObject, NSApplicationDelegate {
    private var window: NSWindow?
    private var statusLabel: NSTextField?
    private var sharingEnabled = true

    static func main() {
        let application = NSApplication.shared
        let delegate = SecondScreenMacHostDelegate()
        application.delegate = delegate
        application.setActivationPolicy(.regular)
        application.run()
    }

    func applicationDidFinishLaunching(_ notification: Notification) {
        let content = NSView(frame: NSRect(x: 0, y: 0, width: 620, height: 370))

        let title = NSTextField(labelWithString: "SecondScreen — Zoavintsoa")
        title.font = NSFont.systemFont(ofSize: 24, weight: .semibold)
        title.alignment = .center
        title.frame = NSRect(x: 32, y: 315, width: 556, height: 32)
        content.addSubview(title)

        let subtitle = NSTextField(labelWithString: "Choisissez comment utiliser votre Android")
        subtitle.alignment = .center
        subtitle.textColor = .secondaryLabelColor
        subtitle.frame = NSRect(x: 32, y: 283, width: 556, height: 24)
        content.addSubview(subtitle)

        let details = NSTextField(labelWithString: "Mode prototype : 1280×720 · H.264 · 30 FPS · réseau local")
        details.alignment = .center
        details.frame = NSRect(x: 32, y: 244, width: 556, height: 24)
        content.addSubview(details)

        let modes = NSSegmentedControl(
            labels: ["Mac uniquement", "Dupliquer", "Étendre", "Android seul"],
            trackingMode: .selectOne,
            target: self,
            action: #selector(displayModeChanged(_:))
        )
        modes.frame = NSRect(x: 28, y: 185, width: 564, height: 32)
        for segment in 0..<4 {
            modes.setWidth(141, forSegment: segment)
        }
        modes.selectedSegment = 1
        modes.setEnabled(false, forSegment: 2)
        modes.setEnabled(false, forSegment: 3)
        content.addSubview(modes)

        let modeHint = NSTextField(wrappingLabelWithString:
            "« Dupliquer » diffuse l’écran principal du Mac vers Android. « Mac uniquement » arrête le partage. « Étendre » et « Android seul » restent désactivés tant qu’un véritable écran virtuel macOS n’est pas disponible."
        )
        modeHint.alignment = .center
        modeHint.textColor = .secondaryLabelColor
        modeHint.frame = NSRect(x: 42, y: 115, width: 536, height: 58)
        content.addSubview(modeHint)

        let status = NSTextField(labelWithString: "Démarrage du flux…")
        status.alignment = .center
        status.font = NSFont.systemFont(ofSize: 13, weight: .medium)
        status.frame = NSRect(x: 32, y: 80, width: 556, height: 24)
        content.addSubview(status)
        statusLabel = status

        let permission = NSTextField(wrappingLabelWithString:
            "Si l’image ne s’affiche pas, autorisez SecondScreen dans Réglages Système → Confidentialité et sécurité → Enregistrement de l’écran."
        )
        permission.alignment = .center
        permission.textColor = .secondaryLabelColor
        permission.frame = NSRect(x: 42, y: 25, width: 536, height: 46)
        content.addSubview(permission)

        let window = NSWindow(
            contentRect: NSRect(x: 0, y: 0, width: 620, height: 370),
            styleMask: [.titled, .closable, .miniaturizable],
            backing: .buffered,
            defer: false
        )
        window.title = "SecondScreen — Zoavintsoa"
        window.contentView = content
        window.center()
        window.isReleasedWhenClosed = false
        self.window = window

        NSApp.activate(ignoringOtherApps: true)
        window.makeKeyAndOrderFront(nil)

        startSharing()
    }

    @objc private func displayModeChanged(_ sender: NSSegmentedControl) {
        switch sender.selectedSegment {
        case 0:
            sharingEnabled = false
            statusLabel?.stringValue = "Partage arrêté — écran du Mac uniquement"
            DispatchQueue.global(qos: .userInitiated).async {
                SecondScreenStopTestStream()
            }
        case 1:
            sharingEnabled = true
            statusLabel?.stringValue = "Démarrage du partage vers Android…"
            startSharing()
        default:
            // Unsupported segments are disabled in the UI. Keep this guard so
            // future UI changes cannot imply that extended desktop is available.
            sender.selectedSegment = sharingEnabled ? 1 : 0
        }
    }

    private func startSharing() {
        guard sharingEnabled else { return }
        DispatchQueue.global(qos: .userInitiated).async {
            SecondScreenStartTestStream()
        }
    }

    func applicationDidBecomeActive(_ notification: Notification) {
        // Re-check screen-recording permission after returning from System Settings,
        // but respect an explicit user choice to stop sharing.
        startSharing()
    }

    func applicationWillTerminate(_ notification: Notification) {
        SecondScreenStopTestStream()
    }
}
