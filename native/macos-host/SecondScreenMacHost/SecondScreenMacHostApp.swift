import AppKit

final class SecondScreenMacHostDelegate: NSObject, NSApplicationDelegate {
    private var window: NSWindow?

    func applicationDidFinishLaunching(_ notification: Notification) {
        let content = NSView(frame: NSRect(x: 0, y: 0, width: 520, height: 280))

        let title = NSTextField(labelWithString: "SecondScreen — Zoavintsoa")
        title.font = NSFont.systemFont(ofSize: 24, weight: .semibold)
        title.alignment = .center
        title.frame = NSRect(x: 32, y: 205, width: 456, height: 32)
        content.addSubview(title)

        let subtitle = NSTextField(labelWithString: "macOS host foundation")
        subtitle.alignment = .center
        subtitle.textColor = .secondaryLabelColor
        subtitle.frame = NSRect(x: 32, y: 168, width: 456, height: 24)
        content.addSubview(subtitle)

        let details = NSTextField(
            labelWithString: "Capture and encode boundaries are isolated by macOS capability tier."
        )
        details.alignment = .center
        details.lineBreakMode = .byWordWrapping
        details.frame = NSRect(x: 52, y: 105, width: 416, height: 48)
        content.addSubview(details)

        let legacy = NSTextField(
            labelWithString: "Virtual display requires a supported public display mechanism on this macOS target."
        )
        legacy.alignment = .center
        legacy.textColor = .secondaryLabelColor
        legacy.lineBreakMode = .byWordWrapping
        legacy.frame = NSRect(x: 52, y: 48, width: 416, height: 48)
        content.addSubview(legacy)

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
}

let app = NSApplication.shared
let delegate = SecondScreenMacHostDelegate()
app.delegate = delegate
app.setActivationPolicy(.regular)
app.run()
