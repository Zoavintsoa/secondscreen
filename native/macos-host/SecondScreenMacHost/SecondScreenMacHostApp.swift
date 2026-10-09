import AppKit
import Dispatch

@main
final class SecondScreenMacHostDelegate: NSObject, NSApplicationDelegate {
    private var window: NSWindow?
    private var statusLabel: NSTextField?
    private var sharingEnabled = true

    private let accent = NSColor(calibratedRed: 0.20, green: 0.78, blue: 0.96, alpha: 1.0)
    private let panel = NSColor(calibratedRed: 0.10, green: 0.12, blue: 0.16, alpha: 1.0)

    static func main() {
        let application = NSApplication.shared
        let delegate = SecondScreenMacHostDelegate()
        application.delegate = delegate
        application.setActivationPolicy(.regular)
        application.run()
    }

    func applicationDidFinishLaunching(_ notification: Notification) {
        let content = NSView(frame: NSRect(x: 0, y: 0, width: 720, height: 470))
        content.wantsLayer = true
        content.layer?.backgroundColor = NSColor(calibratedRed: 0.055, green: 0.065, blue: 0.085, alpha: 1).cgColor

        let icon = NSImageView(frame: NSRect(x: 42, y: 385, width: 48, height: 48))
        if #available(macOS 11.0, *) {
            icon.image = NSImage(systemSymbolName: "rectangle.on.rectangle", accessibilityDescription: "SecondScreen")
        }
        icon.contentTintColor = accent
        icon.imageScaling = .scaleProportionallyUpOrDown
        content.addSubview(icon)

        let title = NSTextField(labelWithString: "SecondScreen")
        title.font = NSFont.systemFont(ofSize: 27, weight: .bold)
        title.textColor = .white
        title.frame = NSRect(x: 102, y: 399, width: 390, height: 34)
        content.addSubview(title)

        let brand = NSTextField(labelWithString: "ZOAVINTSOA  /  LOCAL DISPLAY WORKSPACE")
        brand.font = NSFont.monospacedSystemFont(ofSize: 10, weight: .medium)
        brand.textColor = accent
        brand.frame = NSRect(x: 104, y: 380, width: 430, height: 18)
        content.addSubview(brand)

        let version = NSTextField(labelWithString: "PROTOTYPE 0.1")
        version.font = NSFont.monospacedSystemFont(ofSize: 10, weight: .medium)
        version.textColor = .secondaryLabelColor
        version.alignment = .right
        version.frame = NSRect(x: 570, y: 400, width: 108, height: 20)
        content.addSubview(version)

        let intro = NSTextField(labelWithString: "Connectez votre appareil Android à votre espace de travail.")
        intro.font = NSFont.systemFont(ofSize: 13, weight: .regular)
        intro.textColor = NSColor(calibratedWhite: 0.78, alpha: 1)
        intro.frame = NSRect(x: 42, y: 344, width: 630, height: 22)
        content.addSubview(intro)

        let card = NSBox(frame: NSRect(x: 34, y: 155, width: 652, height: 170))
        card.boxType = .custom
        card.borderType = .noBorder
        card.fillColor = panel
        card.cornerRadius = 14
        content.addSubview(card)

        let modeTitle = NSTextField(labelWithString: "MODE D’AFFICHAGE")
        modeTitle.font = NSFont.systemFont(ofSize: 11, weight: .bold)
        modeTitle.textColor = NSColor(calibratedWhite: 0.75, alpha: 1)
        modeTitle.frame = NSRect(x: 54, y: 285, width: 250, height: 18)
        content.addSubview(modeTitle)

        let modes = NSSegmentedControl(
            labels: ["Mac uniquement", "Dupliquer", "Étendre", "Android seul"],
            trackingMode: .selectOne,
            target: self,
            action: #selector(displayModeChanged(_:))
        )
        modes.frame = NSRect(x: 52, y: 236, width: 616, height: 34)
        modes.selectedSegment = 1
        modes.setEnabled(false, forSegment: 2)
        modes.setEnabled(false, forSegment: 3)
        content.addSubview(modes)

        let modeHint = NSTextField(wrappingLabelWithString:
            "Dupliquer transmet l’écran principal du Mac vers Android. Mac uniquement arrête le partage. Étendre et Android seul seront activés uniquement après l’implémentation d’un véritable écran virtuel."
        )
        modeHint.font = NSFont.systemFont(ofSize: 11, weight: .regular)
        modeHint.textColor = .secondaryLabelColor
        modeHint.frame = NSRect(x: 54, y: 174, width: 612, height: 50)
        content.addSubview(modeHint)

        let statusDot = NSView(frame: NSRect(x: 44, y: 112, width: 9, height: 9))
        statusDot.wantsLayer = true
        statusDot.layer?.backgroundColor = accent.cgColor
        statusDot.layer?.cornerRadius = 4.5
        content.addSubview(statusDot)

        let status = NSTextField(labelWithString: "Démarrage du flux…")
        status.font = NSFont.systemFont(ofSize: 12, weight: .medium)
        status.textColor = .white
        status.frame = NSRect(x: 62, y: 104, width: 590, height: 24)
        statusLabel = status
        content.addSubview(status)

        let permission = NSTextField(wrappingLabelWithString:
            "Si l’image ne s’affiche pas, autorisez SecondScreen dans Réglages Système → Confidentialité et sécurité → Enregistrement de l’écran."
        )
        permission.font = NSFont.systemFont(ofSize: 10, weight: .regular)
        permission.textColor = .tertiaryLabelColor
        permission.frame = NSRect(x: 44, y: 38, width: 632, height: 44)
        content.addSubview(permission)

        let window = NSWindow(
            contentRect: NSRect(x: 0, y: 0, width: 720, height: 470),
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
        startSharing()
    }

    func applicationWillTerminate(_ notification: Notification) {
        SecondScreenStopTestStream()
    }
}
