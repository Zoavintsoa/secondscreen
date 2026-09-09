// SecondScreen iOS/iPadOS Main View Controller
// True immersive fullscreen remote secondary display receiver
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

import UIKit
import MetalKit

public class SecondScreenIOSViewController: UIViewController, IOSDecoderDelegate, IOSClientDelegate, IOSInputDelegate {

    private var metalView: SecondScreenMetalView!
    private var decoder: IOSHardwareDecoder!
    private var client: SecondScreenIOSClient!

    private var pairingOverlay: UIView!
    private var ipTextField: UITextField!
    private var pinTextField: UITextField!
    private var connectButton: UIButton!
    private var statusLabel: UILabel!
    private var telemetryPill: UILabel!

    public override func viewDidLoad() {
        super.viewDidLoad()
        view.backgroundColor = .black

        // 1. Setup Metal Viewport (Full Viewport)
        metalView = SecondScreenMetalView(frame: view.bounds)
        metalView.autoresizingMask = [.flexibleWidth, .flexibleHeight]
        metalView.inputDelegate = self
        view.addSubview(metalView)

        // 2. Setup Decoder & Network Client
        decoder = IOSHardwareDecoder()
        decoder.delegate = self

        client = SecondScreenIOSClient()
        client.delegate = self

        // 3. Setup Pairing Overlay UI
        setupPairingUI()

        // 4. Setup Telemetry Overlay (Hidden during pure fullscreen)
        telemetryPill = UILabel(frame: CGRect(x: 24, y: 48, width: 280, height: 28))
        telemetryPill.backgroundColor = UIColor(white: 0.1, alpha: 0.8)
        telemetryPill.layer.cornerRadius = 14
        telemetryPill.layer.masksToBounds = true
        telemetryPill.textColor = .systemGreen
        telemetryPill.font = .monospacedSystemFont(ofSize: 11, weight: .semibold)
        telemetryPill.textAlignment = .center
        telemetryPill.text = "FPS: N/A | Latency: N/A"
        telemetryPill.isHidden = true
        view.addSubview(telemetryPill)

        // 5. Edge Screen Tap Gesture to reveal Disconnect / Menu
        let edgeGesture = UIScreenEdgePanGestureRecognizer(target: self, action: #selector(handleEdgePan))
        edgeGesture.edges = .top
        view.addGestureRecognizer(edgeGesture)
    }

    public override var prefersStatusBarHidden: Bool {
        return true
    }

    public override var prefersHomeIndicatorAutoHidden: Bool {
        return true
    }

    private func setupPairingUI() {
        pairingOverlay = UIView(frame: CGRect(x: 0, y: 0, width: 340, height: 280))
        pairingOverlay.center = view.center
        pairingOverlay.autoresizingMask = [.flexibleTopMargin, .flexibleBottomMargin, .flexibleLeftMargin, .flexibleRightMargin]
        pairingOverlay.backgroundColor = UIColor(red: 0.06, green: 0.06, blue: 0.07, alpha: 0.95)
        pairingOverlay.layer.cornerRadius = 12
        pairingOverlay.layer.borderColor = UIColor(white: 0.2, alpha: 1.0).cgColor
        pairingOverlay.layer.borderWidth = 1.0

        statusLabel = UILabel(frame: CGRect(x: 20, y: 20, width: 300, height: 30))
        statusLabel.text = "Connect to Windows / Mac Host"
        statusLabel.textColor = .white
        statusLabel.font = .systemFont(ofSize: 14, weight: .bold)
        statusLabel.textAlignment = .center

        ipTextField = UITextField(frame: CGRect(x: 20, y: 65, width: 300, height: 40))
        ipTextField.placeholder = "Host IP (e.g. 192.168.1.145)"
        ipTextField.backgroundColor = UIColor(white: 0.04, alpha: 1.0)
        ipTextField.textColor = .white
        ipTextField.layer.cornerRadius = 6
        ipTextField.textAlignment = .center

        pinTextField = UITextField(frame: CGRect(x: 20, y: 120, width: 300, height: 45))
        pinTextField.placeholder = "6-Digit PIN"
        pinTextField.backgroundColor = UIColor(white: 0.04, alpha: 1.0)
        pinTextField.textColor = .white
        pinTextField.font = .monospacedSystemFont(ofSize: 22, weight: .bold)
        pinTextField.textAlignment = .center
        pinTextField.keyboardType = .numberPad

        connectButton = UIButton(type: .system)
        connectButton.frame = CGRect(x: 20, y: 185, width: 300, height: 45)
        connectButton.setTitle("Connect & Fullscreen", for: .normal)
        connectButton.setTitleColor(.white, for: .normal)
        connectButton.backgroundColor = UIColor(red: 0.15, green: 0.39, blue: 0.92, alpha: 1.0)
        connectButton.layer.cornerRadius = 6
        connectButton.titleLabel?.font = .systemFont(ofSize: 14, weight: .bold)
        connectButton.addTarget(self, action: #selector(didTapConnect), for: .touchUpInside)

        pairingOverlay.addSubview(statusLabel)
        pairingOverlay.addSubview(ipTextField)
        pairingOverlay.addSubview(pinTextField)
        pairingOverlay.addSubview(connectButton)

        view.addSubview(pairingOverlay)
    }

    @objc private func didTapConnect() {
        guard let ip = ipTextField.text, !ip.isEmpty,
              let pin = pinTextField.text, !pin.isEmpty else { return }

        statusLabel.text = "Connecting to \(ip)..."
        client.connect(to: ip, port: 9876, pin: pin)
    }

    @objc private func handleEdgePan() {
        let alert = UIAlertController(title: "SecondScreen Options", message: "Remote Display Settings", preferredStyle: .actionSheet)
        alert.addAction(UIAlertAction(title: "Toggle Telemetry Overlay", style: .default) { [weak self] _ in
            self?.telemetryPill.isHidden.toggle()
        })
        alert.addAction(UIAlertAction(title: "Disconnect", style: .destructive) { [weak self] _ in
            self?.client.disconnect()
        })
        alert.addAction(UIAlertAction(title: "Cancel", style: .cancel))
        present(alert, animated: true)
    }

    // IOSDecoderDelegate
    public func didDecompressPixelBuffer(_ pixelBuffer: CVPixelBuffer, presentationTimeUs: UInt64) {
        DispatchQueue.main.async {
            self.metalView.renderPixelBuffer(pixelBuffer)
        }
    }

    public func decoderDidEncounterError(status: OSStatus) {
        DispatchQueue.main.async {
            NSLog("[SecondScreen iOS] Hardware decode error: OSStatus=%d", status)
            self.telemetryPill.isHidden = false
            self.telemetryPill.text = String(format: "Decode Error: OSStatus %d", status)
        }
    }


    // IOSClientDelegate
    public func clientDidUpdateState(_ state: IOSClientState) {
        DispatchQueue.main.async {
            switch state {
            case .streaming:
                self.pairingOverlay.isHidden = true
                self.setNeedsUpdateOfHomeIndicatorAutoHidden()
            case .disconnected, .error:
                self.pairingOverlay.isHidden = false
                self.statusLabel.text = "Disconnected. Enter PIN to reconnect."
            default:
                break
            }
        }
    }

    public func clientDidReceiveVideoFrame(naluData: Data, presentationTimeUs: UInt64, isKeyFrame: Bool) {
        decoder.decodeFrame(naluData: naluData, presentationTimeUs: presentationTimeUs)
    }

    public func clientDidReceiveTelemetry(fps: Float, rttMs: Float, totalLatencyMs: Float, isMeasured: Bool) {
        DispatchQueue.main.async {
            if isMeasured {
                self.telemetryPill.text = String(format: "FPS: %.1f | RTT: %.1f ms | E2E: %.1f ms", fps, rttMs, totalLatencyMs)
            } else {
                self.telemetryPill.text = "FPS: N/A | Latency: N/A (Not measured)"
            }
        }
    }

    // IOSInputDelegate
    public func didCaptureInputEvent(actionType: UInt8, normX: Float, normY: Float, pressure: Float, button: UInt8) {
        client.sendInputEvent(actionType: actionType, normX: normX, normY: normY, pressure: pressure, button: button)
    }
}
