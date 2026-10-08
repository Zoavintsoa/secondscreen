import AppKit

@NSApplicationMain
final class SecondScreenMacHostDelegate: NSObject, NSApplicationDelegate {
    private var window: NSWindow?

    func applicationDidFinishLaunching(_ notification: Notification) {
        SecondScreenStartTestStream()

        let content = NSView(frame: NSRect(x: 0, y: 0, width: 520, height: 280))
        let title = NSTextField(labelWithString: "SecondScreen — Zoavintsoa")
        title.font = NSFont.systemFont(ofSize: 24, weight: .semibold)
        title.alignment = .center
        title.frame = NSRect(x: 32, y: 205, width: 456, height: 32)
        content.addSubview(title)

        let subtitle = NSTextField(labelWithString: "iMac → Android test stream")
        subtitle.alignment = .center
        subtitle.textColor = .secondaryLabelColor
        subtitle.frame = NSRect(x: 32, y: 168, width: 456, height: 24)
        content.addSubview(subtitle)

        let details = NSTextField(labelWithString: "1280×720 · H.264 · 30 FPS · LAN")
        details.alignment = .center
        details.frame = NSRect(x: 52, y: 116, width: 416, height: 32)
        content.addSubview(details)

        let permission = NSTextField(labelWithString: "Autorisez l’enregistrement de l’écran pour SecondScreen dans Réglages Système → Confidentialité et sécurité.")
        permission.alignment = .center
        permission.textColor = .secondaryLabelColor
        permission.lineBreakMode = .byWordWrapping
        permission.frame = NSRect(x: 52, y: 55, width: 416, height: 52)
        content.addSubview(permission)

        let window = NSWindow(
            contentRect: NSRect(x: 0, y: 0, width: 520, height: 280),
            styleMask: [.titled, .closable, .miniaturizable],
            backing: .buffered,
            defer: false
        )
        window.title = "SecondScreen — Zoavintsoa"
        window.contentView = content
        window.center()
        window.isReleasedWhenClosed = false
        window.makeKeyAndOrderFront(nil)
        self.window = window
        NSApp.activate(ignoringOtherApps: true)
    }

    func applicationWillTerminate(_ notification: Notification) {
        SecondScreenStopTestStream()
    }
}
