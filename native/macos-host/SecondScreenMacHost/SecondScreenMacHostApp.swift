import AppKit
import Dispatch

@main
final class SecondScreenMacHostDelegate: NSObject, NSApplicationDelegate {
    private var window: NSWindow?
    private let streamStatus = NSTextField(labelWithString: "Initialisation du flux de test…")
    private let statusDot = NSView()
    private let startButton = NSButton(title: "Démarrer le flux", target: nil, action: nil)
    private let stopButton = NSButton(title: "Arrêter le flux", target: nil, action: nil)
    private let virtualDisplayButton = NSButton(title: "Créer un écran virtuel", target: nil, action: nil)

    static func main() {
        let application = NSApplication.shared
        let delegate = SecondScreenMacHostDelegate()
        application.delegate = delegate
        application.setActivationPolicy(.regular)
        application.run()
    }

    func applicationDidFinishLaunching(_ notification: Notification) {
        buildWindow()
        NSApp.activate(ignoringOtherApps: true)
        window?.makeKeyAndOrderFront(nil)

        streamStatus.stringValue = "Prêt · flux arrêté"
        statusDot.layer?.backgroundColor = NSColor.systemGray.cgColor
        startButton.isEnabled = true
        stopButton.isEnabled = false
    }

    func applicationWillTerminate(_ notification: Notification) {
        SecondScreenStopTestStream()
    }

    private func buildWindow() {
        let window = NSWindow(
            contentRect: NSRect(x: 0, y: 0, width: 900, height: 760),
            styleMask: [.titled, .closable, .miniaturizable, .resizable],
            backing: .buffered,
            defer: false
        )
        window.title = "SecondScreen — Zoavintsoa"
        window.minSize = NSSize(width: 720, height: 640)
        window.isReleasedWhenClosed = false
        window.center()

        let root = NSView()
        root.translatesAutoresizingMaskIntoConstraints = false
        root.wantsLayer = true
        root.layer?.backgroundColor = NSColor.windowBackgroundColor.cgColor
        window.contentView = root

        let scroll = NSScrollView()
        scroll.translatesAutoresizingMaskIntoConstraints = false
        scroll.hasVerticalScroller = true
        scroll.drawsBackground = false
        scroll.borderType = .noBorder
        root.addSubview(scroll)

        let page = NSStackView()
        page.orientation = .vertical
        page.alignment = .leading
        page.distribution = .fill
        page.spacing = 18
        page.translatesAutoresizingMaskIntoConstraints = false
        page.edgeInsets = NSEdgeInsets(top: 28, left: 30, bottom: 28, right: 30)
        scroll.documentView = page

        NSLayoutConstraint.activate([
            scroll.leadingAnchor.constraint(equalTo: root.leadingAnchor),
            scroll.trailingAnchor.constraint(equalTo: root.trailingAnchor),
            scroll.topAnchor.constraint(equalTo: root.topAnchor),
            scroll.bottomAnchor.constraint(equalTo: root.bottomAnchor),
            page.leadingAnchor.constraint(equalTo: scroll.contentView.leadingAnchor),
            page.trailingAnchor.constraint(equalTo: scroll.contentView.trailingAnchor),
            page.topAnchor.constraint(equalTo: scroll.contentView.topAnchor),
            page.bottomAnchor.constraint(equalTo: scroll.contentView.bottomAnchor),
            page.widthAnchor.constraint(equalTo: scroll.contentView.widthAnchor)
        ])

        page.addArrangedSubview(makeHeader())
        page.addArrangedSubview(makeHeroCard())
        page.addArrangedSubview(makeSectionTitle("ÉCRAN VIRTUEL", subtitle: "Gestion du moniteur système"))
        page.addArrangedSubview(makeVirtualDisplayCard())
        page.addArrangedSubview(makeSectionTitle("CONFIGURATION", subtitle: "Paramètres du flux de test actuel"))
        page.addArrangedSubview(makeConfigurationCard())
        page.addArrangedSubview(makeSectionTitle("CONNEXION & DIAGNOSTIC", subtitle: "Comprendre précisément ce qui fonctionne"))
        page.addArrangedSubview(makeDiagnosticsCard())
        page.addArrangedSubview(makeSectionTitle("AUTORISATIONS macOS", subtitle: "Nécessaires à la capture de l’écran"))
        page.addArrangedSubview(makePermissionsCard())
        page.addArrangedSubview(makeFooter())

        for item in page.arrangedSubviews {
            item.widthAnchor.constraint(equalTo: page.widthAnchor, constant: -60).isActive = true
        }

        self.window = window
    }

    private func makeHeader() -> NSView {
        let row = NSStackView()
        row.orientation = .horizontal
        row.alignment = .centerY
        row.spacing = 14

        let icon = NSImageView()
        configureSymbol(icon, name: "display.2", size: 30)
        icon.contentTintColor = NSColor.controlAccentColor
        icon.imageScaling = .scaleProportionallyUpOrDown
        icon.setContentHuggingPriority(.required, for: .horizontal)
        icon.widthAnchor.constraint(equalToConstant: 46).isActive = true
        icon.heightAnchor.constraint(equalToConstant: 46).isActive = true

        let titles = NSStackView()
        titles.orientation = .vertical
        titles.alignment = .leading
        titles.spacing = 3
        titles.addArrangedSubview(label("SecondScreen", size: 25, weight: .bold))
        titles.addArrangedSubview(label("ZOAVINTSOA  /  MAC HOST", size: 10, weight: .semibold, color: .secondaryLabelColor))

        let spacer = NSView()
        spacer.setContentHuggingPriority(.defaultLow, for: .horizontal)

        let version = label("APERÇU TECHNIQUE", size: 10, weight: .semibold, color: .secondaryLabelColor)
        row.addArrangedSubview(icon)
        row.addArrangedSubview(titles)
        row.addArrangedSubview(spacer)
        row.addArrangedSubview(version)
        return row
    }

    private func makeHeroCard() -> NSView {
        let card = cardView()
        let stack = verticalStack(spacing: 14)

        let heading = horizontalStack(spacing: 12)
        let headingText = verticalStack(spacing: 4)
        headingText.addArrangedSubview(label("Votre écran secondaire", size: 20, weight: .bold))
        headingText.addArrangedSubview(label("Diffusion locale depuis ce Mac vers un appareil client.", size: 13, color: .secondaryLabelColor))
        heading.addArrangedSubview(headingText)

        let spacer = NSView()
        spacer.setContentHuggingPriority(.defaultLow, for: .horizontal)
        heading.addArrangedSubview(spacer)

        statusDot.wantsLayer = true
        statusDot.layer?.cornerRadius = 5
        statusDot.layer?.backgroundColor = NSColor.systemOrange.cgColor
        statusDot.widthAnchor.constraint(equalToConstant: 10).isActive = true
        statusDot.heightAnchor.constraint(equalToConstant: 10).isActive = true
        heading.addArrangedSubview(statusDot)
        heading.addArrangedSubview(streamStatus)
        streamStatus.font = NSFont.systemFont(ofSize: 11, weight: .medium)
        streamStatus.textColor = .secondaryLabelColor

        let divider = NSBox()
        divider.boxType = .separator

        let actions = horizontalStack(spacing: 10)
        startButton.target = self
        startButton.action = #selector(startStream)
        startButton.bezelStyle = .rounded
        startButton.controlSize = .regular
        startButton.image = symbolImage("play.fill", accessibilityDescription: "Démarrer")
        startButton.imagePosition = .imageLeading

        stopButton.target = self
        stopButton.action = #selector(stopStream)
        stopButton.bezelStyle = .rounded
        stopButton.controlSize = .regular
        stopButton.image = symbolImage("stop.fill", accessibilityDescription: "Arrêter")
        stopButton.imagePosition = .imageLeading

        actions.addArrangedSubview(startButton)
        actions.addArrangedSubview(stopButton)

        stack.addArrangedSubview(heading)
        stack.addArrangedSubview(divider)
        stack.addArrangedSubview(actions)
        embed(stack, in: card, inset: 20)
        return card
    }

    private func makeVirtualDisplayCard() -> NSView {
        let card = cardView()
        let stack = verticalStack(spacing: 12)
        stack.addArrangedSubview(label("Créer et gérer un écran supplémentaire", size: 15, weight: .semibold))
        let detail = label(
            "L’option est conservée ici. Cette version ne dispose pas encore d’un mécanisme macOS public et validé permettant de créer automatiquement un moniteur virtuel. Aucun écran ne sera déclaré créé sans confirmation du système.",
            size: 12,
            color: .secondaryLabelColor
        )
        detail.maximumNumberOfLines = 4
        stack.addArrangedSubview(detail)
        virtualDisplayButton.target = self
        virtualDisplayButton.action = #selector(createVirtualDisplay)
        virtualDisplayButton.bezelStyle = .rounded
        virtualDisplayButton.controlSize = .regular
        stack.addArrangedSubview(virtualDisplayButton)
        embed(stack, in: card, inset: 18)
        return card
    }

    private func makeConfigurationCard() -> NSView {
        let card = cardView()
        let grid = NSGridView()
        grid.rowSpacing = 14
        grid.columnSpacing = 24
        grid.xPlacement = .leading
        grid.yPlacement = .center

        let values: [(String, String, String, String)] = [
            ("Résolution", "1280 × 720", "Codec vidéo", "H.264"),
            ("Fréquence cible", "30 images/s", "Transport", "Réseau local"),
            ("Port vidéo", "TCP 49153", "Découverte", "UDP 49151"),
            ("Contrôle", "TCP 49152", "Mode", "Flux de test")
        ]

        for row in values {
            let left = makeMetric(title: row.0, value: row.1)
            let right = makeMetric(title: row.2, value: row.3)
            grid.addRow(with: [left, right])
        }

        grid.translatesAutoresizingMaskIntoConstraints = false
        card.addSubview(grid)
        NSLayoutConstraint.activate([
            grid.leadingAnchor.constraint(equalTo: card.leadingAnchor, constant: 20),
            grid.trailingAnchor.constraint(equalTo: card.trailingAnchor, constant: -20),
            grid.topAnchor.constraint(equalTo: card.topAnchor, constant: 18),
            grid.bottomAnchor.constraint(equalTo: card.bottomAnchor, constant: -18)
        ])
        return card
    }

    private func makeDiagnosticsCard() -> NSView {
        let card = cardView()
        let stack = verticalStack(spacing: 0)
        stack.addArrangedSubview(diagnosticRow(
            icon: "desktopcomputer",
            title: "Application macOS",
            detail: "L’interface hôte est ouverte.",
            status: "ACTIVE",
            color: .systemGreen
        ))
        stack.addArrangedSubview(separator())
        stack.addArrangedSubview(diagnosticRow(
            icon: "viewfinder",
            title: "Capture de l’écran",
            detail: "L’autorisation ne garantit pas, à elle seule, que des images sont capturées.",
            status: "À VÉRIFIER",
            color: .systemOrange
        ))
        stack.addArrangedSubview(separator())
        stack.addArrangedSubview(diagnosticRow(
            icon: "dot.radiowaves.left.and.right",
            title: "Vidéo reçue sur Android",
            detail: "Aucune confirmation de réception vidéo n’est actuellement exposée à l’interface.",
            status: "NON CONFIRMÉ",
            color: .systemOrange
        ))
        embed(stack, in: card, inset: 6)
        return card
    }

    private func makePermissionsCard() -> NSView {
        let card = cardView()
        let stack = verticalStack(spacing: 12)
        let text = verticalStack(spacing: 4)
        text.addArrangedSubview(label("Enregistrement de l’écran", size: 14, weight: .semibold))
        text.addArrangedSubview(label("macOS doit autoriser SecondScreen à capturer l’image de l’écran.", size: 12, color: .secondaryLabelColor))
        let button = NSButton(title: "Ouvrir Confidentialité et sécurité", target: self, action: #selector(openScreenRecordingSettings))
        button.bezelStyle = .rounded
        button.controlSize = .regular
        button.image = symbolImage("lock.shield", accessibilityDescription: "Confidentialité")
        button.imagePosition = .imageLeading
        stack.addArrangedSubview(text)
        stack.addArrangedSubview(button)
        embed(stack, in: card, inset: 18)
        return card
    }

    private func makeFooter() -> NSView {
        let row = horizontalStack(spacing: 8)
        row.addArrangedSubview(label("SecondScreen by Zoavintsoa", size: 11, color: .secondaryLabelColor))
        let spacer = NSView()
        spacer.setContentHuggingPriority(.defaultLow, for: .horizontal)
        row.addArrangedSubview(spacer)
        row.addArrangedSubview(label("HÔTE MACOS · VERSION EXPÉRIMENTALE", size: 9, weight: .medium, color: .tertiaryLabelColor))
        return row
    }

    private func makeSectionTitle(_ title: String, subtitle: String) -> NSView {
        let stack = verticalStack(spacing: 4)
        stack.addArrangedSubview(label(title, size: 10, weight: .bold, color: .secondaryLabelColor))
        stack.addArrangedSubview(label(subtitle, size: 12, color: .secondaryLabelColor))
        return stack
    }

    private func makeMetric(title: String, value: String) -> NSView {
        let stack = verticalStack(spacing: 5)
        stack.addArrangedSubview(label(title.uppercased(), size: 9, weight: .semibold, color: .secondaryLabelColor))
        stack.addArrangedSubview(label(value, size: 14, weight: .semibold))
        return stack
    }

    private func diagnosticRow(icon: String, title: String, detail: String, status: String, color: NSColor) -> NSView {
        let row = horizontalStack(spacing: 12)
        let symbol = NSImageView()
        configureSymbol(symbol, name: icon, size: 17)
        symbol.contentTintColor = .secondaryLabelColor
        symbol.widthAnchor.constraint(equalToConstant: 24).isActive = true
        symbol.heightAnchor.constraint(equalToConstant: 24).isActive = true

        let texts = verticalStack(spacing: 4)
        texts.addArrangedSubview(label(title, size: 13, weight: .semibold))
        let detailLabel = label(detail, size: 11, color: .secondaryLabelColor)
        detailLabel.maximumNumberOfLines = 2
        texts.addArrangedSubview(detailLabel)

        let spacer = NSView()
        spacer.setContentHuggingPriority(.defaultLow, for: .horizontal)
        let badge = label(status, size: 9, weight: .bold, color: color)
        badge.setContentHuggingPriority(.required, for: .horizontal)

        row.addArrangedSubview(symbol)
        row.addArrangedSubview(texts)
        row.addArrangedSubview(spacer)
        row.addArrangedSubview(badge)
        row.edgeInsets = NSEdgeInsets(top: 12, left: 12, bottom: 12, right: 12)
        return row
    }

    private func cardView() -> NSView {
        let view = NSView()
        view.translatesAutoresizingMaskIntoConstraints = false
        view.wantsLayer = true
        view.layer?.cornerRadius = 12
        view.layer?.backgroundColor = NSColor.controlBackgroundColor.cgColor
        view.layer?.borderWidth = 1
        view.layer?.borderColor = NSColor.separatorColor.withAlphaComponent(0.45).cgColor
        return view
    }

    private func symbolImage(_ name: String, accessibilityDescription: String?) -> NSImage? {
        if #available(macOS 11.0, *) {
            return NSImage(systemSymbolName: name, accessibilityDescription: accessibilityDescription)
        }
        return NSImage(named: NSImage.applicationIconName)
    }

    private func configureSymbol(_ imageView: NSImageView, name: String, size: CGFloat) {
        if #available(macOS 11.0, *) {
            imageView.image = NSImage(systemSymbolName: name, accessibilityDescription: name)
            imageView.symbolConfiguration = NSImage.SymbolConfiguration(pointSize: size, weight: .medium)
        } else {
            imageView.image = NSImage(named: NSImage.applicationIconName)
        }
    }

    private func label(_ text: String, size: CGFloat, weight: NSFont.Weight = .regular, color: NSColor = .labelColor) -> NSTextField {
        let field = NSTextField(labelWithString: text)
        field.font = NSFont.systemFont(ofSize: size, weight: weight)
        field.textColor = color
        field.lineBreakMode = .byWordWrapping
        field.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
        return field
    }

    private func verticalStack(spacing: CGFloat) -> NSStackView {
        let stack = NSStackView()
        stack.orientation = .vertical
        stack.alignment = .leading
        stack.distribution = .fill
        stack.spacing = spacing
        stack.translatesAutoresizingMaskIntoConstraints = false
        return stack
    }

    private func horizontalStack(spacing: CGFloat) -> NSStackView {
        let stack = NSStackView()
        stack.orientation = .horizontal
        stack.alignment = .centerY
        stack.distribution = .fill
        stack.spacing = spacing
        stack.translatesAutoresizingMaskIntoConstraints = false
        return stack
    }

    private func embed(_ child: NSView, in parent: NSView, inset: CGFloat) {
        parent.addSubview(child)
        NSLayoutConstraint.activate([
            child.leadingAnchor.constraint(equalTo: parent.leadingAnchor, constant: inset),
            child.trailingAnchor.constraint(equalTo: parent.trailingAnchor, constant: -inset),
            child.topAnchor.constraint(equalTo: parent.topAnchor, constant: inset),
            child.bottomAnchor.constraint(equalTo: parent.bottomAnchor, constant: -inset)
        ])
    }

    private func separator() -> NSView {
        let box = NSBox()
        box.boxType = .separator
        return box
    }

    @objc private func startStream() {
        startButton.isEnabled = false
        stopButton.isEnabled = true
        streamStatus.stringValue = "Démarrage demandé · vidéo non confirmée"
        streamStatus.textColor = .secondaryLabelColor
        statusDot.layer?.backgroundColor = NSColor.systemOrange.cgColor
        DispatchQueue.global(qos: .userInitiated).async {
            SecondScreenStartTestStream()
            DispatchQueue.main.async {
                self.startButton.isEnabled = true
                self.streamStatus.stringValue = "Demande traitée · capture/vidéo à confirmer"
            }
        }
    }

    @objc private func stopStream() {
        DispatchQueue.global(qos: .userInitiated).async {
            SecondScreenStopTestStream()
            DispatchQueue.main.async {
                self.streamStatus.stringValue = "Arrêt demandé"
                self.streamStatus.textColor = .secondaryLabelColor
                self.statusDot.layer?.backgroundColor = NSColor.systemGray.cgColor
                self.startButton.isEnabled = true
                self.stopButton.isEnabled = false
            }
        }
    }

    @objc private func createVirtualDisplay() {
        let alert = NSAlert()
        alert.alertStyle = .informational
        alert.messageText = "Création d’écran virtuel indisponible"
        alert.informativeText = "L’option est rétablie, mais cette version ne peut pas créer un moniteur virtuel macOS avec une API publique prise en charge. Les réglages Écrans permettent de gérer les écrans déjà reconnus par macOS ; ils ne créent pas un écran virtuel SecondScreen."
        alert.addButton(withTitle: "Ouvrir les réglages Écrans")
        alert.addButton(withTitle: "Fermer")
        if alert.runModal() == .alertFirstButtonReturn,
           let url = URL(string: "x-apple.systempreferences:com.apple.preference.displays") {
            NSWorkspace.shared.open(url)
        }
    }

    @objc private func openScreenRecordingSettings() {
        let candidates = [
            "x-apple.systempreferences:com.apple.preference.security?Privacy_ScreenCapture",
            "x-apple.systempreferences:com.apple.settings.PrivacySecurity.extension?Privacy_ScreenCapture"
        ]
        for value in candidates {
            if let url = URL(string: value), NSWorkspace.shared.open(url) {
                return
            }
        }
    }
}
