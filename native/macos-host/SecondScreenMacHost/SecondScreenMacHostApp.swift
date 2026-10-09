import AppKit
import Dispatch

@main
final class SecondScreenMacHostDelegate: NSObject, NSApplicationDelegate {
    private var window: NSWindow?
    private let virtualDisplayManager = SecondScreenMacVirtualDisplayManager()
    private var virtualDisplayStatusLabel: NSTextField?

    static func main() {
        let application = NSApplication.shared
        let delegate = SecondScreenMacHostDelegate()
        application.delegate = delegate
        application.setActivationPolicy(.regular)
        application.run()
    }

    func applicationDidFinishLaunching(_ notification: Notification) {
        let content = NSView(frame: NSRect(x: 0, y: 0, width: 560, height: 360))

        let title = NSTextField(labelWithString: "SecondScreen — Zoavintsoa")
        title.font = NSFont.systemFont(ofSize: 24, weight: .semibold)
        title.alignment = .center
        title.frame = NSRect(x: 32, y: 304, width: 496, height: 32)
        content.addSubview(title)

        let subtitle = NSTextField(labelWithString: "Prototype macOS · moniteur virtuel expérimental")
        subtitle.alignment = .center
        subtitle.textColor = .secondaryLabelColor
        subtitle.frame = NSRect(x: 32, y: 273, width: 496, height: 24)
        content.addSubview(subtitle)

        let details = NSTextField(labelWithString: "1920×1080 · 60 Hz · API privée non prise en charge")
        details.alignment = .center
        details.frame = NSRect(x: 42, y: 238, width: 476, height: 24)
        content.addSubview(details)

        let permission = NSTextField(labelWithString: "Pour transmettre une image, autorisez l’enregistrement de l’écran dans Réglages Système → Confidentialité et sécurité. La capture du nouvel écran n’est pas encore reliée au flux Android.")
        permission.alignment = .center
        permission.textColor = .secondaryLabelColor
        permission.lineBreakMode = .byWordWrapping
        permission.maximumNumberOfLines = 3
        permission.frame = NSRect(x: 38, y: 174, width: 484, height: 54)
        content.addSubview(permission)

        let createButton = NSButton(
            title: "Créer le moniteur virtuel",
            target: self,
            action: #selector(createExperimentalVirtualDisplay)
        )
        createButton.bezelStyle = .rounded
        createButton.frame = NSRect(x: 42, y: 126, width: 228, height: 32)
        content.addSubview(createButton)

        let stopButton = NSButton(
            title: "Supprimer le moniteur",
            target: self,
            action: #selector(stopExperimentalVirtualDisplay)
        )
        stopButton.bezelStyle = .rounded
        stopButton.frame = NSRect(x: 290, y: 126, width: 228, height: 32)
        content.addSubview(stopButton)

        let status = NSTextField(wrappingLabelWithString: "État : aucun moniteur virtuel créé.")
        status.alignment = .center
        status.textColor = .secondaryLabelColor
        status.font = NSFont.systemFont(ofSize: 12)
        status.frame = NSRect(x: 34, y: 36, width: 492, height: 74)
        content.addSubview(status)
        virtualDisplayStatusLabel = status

        let window = NSWindow(
            contentRect: NSRect(x: 0, y: 0, width: 560, height: 360),
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

        // Keep screen-capture permission and streaming off the AppKit launch path.
        DispatchQueue.global(qos: .userInitiated).async {
            SecondScreenStartTestStream()
        }
    }

    @objc private func createExperimentalVirtualDisplay() {
        virtualDisplayStatusLabel?.stringValue =
            "Création du moniteur virtuel… attente de confirmation macOS."

        virtualDisplayManager.createVirtualDisplay(
            withWidth: 1920,
            height: 1080,
            refreshRate: 60,
            name: "SecondScreen — Zoavintsoa"
        ) { [weak self] created, errorMessage in
            guard let self else { return }

            if created {
                self.virtualDisplayStatusLabel?.stringValue =
                    "Moniteur confirmé en ligne : ID \(self.virtualDisplayManager.displayID), \(self.virtualDisplayManager.width)×\(self.virtualDisplayManager.height). Vérifiez Réglages Système → Moniteurs. Le flux Android utilise encore l’écran principal."
            } else {
                let reason = errorMessage ?? self.virtualDisplayManager.lastError
                self.virtualDisplayStatusLabel?.stringValue =
                    "Échec : \(reason) Cette fonction expérimentale utilise une API privée de macOS."
            }
        }
    }

    @objc private func stopExperimentalVirtualDisplay() {
        virtualDisplayManager.destroyVirtualDisplay()
        virtualDisplayStatusLabel?.stringValue =
            "Demande de suppression envoyée. Si le moniteur reste visible, quittez SecondScreen : la suppression par API privée n’est pas garantie avant la fin du processus."
    }

    func applicationDidBecomeActive(_ notification: Notification) {
        // Re-check capture permission and restart the native stream after returning
        // from System Settings or after the app was re-opened from Finder.
        DispatchQueue.global(qos: .userInitiated).async {
            SecondScreenStartTestStream()
        }
    }

    func applicationWillTerminate(_ notification: Notification) {
        virtualDisplayManager.destroyVirtualDisplay()
        SecondScreenStopTestStream()
    }
}
