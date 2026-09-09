// SecondScreen iOS/iPadOS Asynchronous Network Client (Network.framework)
// Receives binary framed video stream & telemetry from Windows or macOS Host
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

import Foundation
import Network

public enum IOSClientState {
    case disconnected
    case discovering
    case connecting
    case pairing
    case streaming
    case reconnecting
    case error(String)
}

public protocol IOSClientDelegate: AnyObject {
    func clientDidUpdateState(_ state: IOSClientState)
    func clientDidReceiveVideoFrame(naluData: Data, presentationTimeUs: UInt64, isKeyFrame: Bool)
    func clientDidReceiveTelemetry(fps: Float, rttMs: Float, totalLatencyMs: Float, isMeasured: Bool)
}

private let PROTOCOL_MAGIC: UInt32 = 0x53325343
private let PACKET_HEADER_SIZE = 24
private let INPUT_EVENT_SIZE = 22
private let TELEMETRY_DATA_SIZE = 37

public class SecondScreenIOSClient {
    private var connection: NWConnection?
    private let queue = DispatchQueue(label: "com.secondscreen.ios.network", qos: .userInteractive)
    public weak var delegate: IOSClientDelegate?

    private var hostAddress: String?
    private var hostPort: UInt16 = 9876
    private var pairingPin: String?

    public init() {}

    public func connect(to host: String, port: UInt16 = 9876, pin: String) {
        self.hostAddress = host
        self.hostPort = port
        self.pairingPin = pin

        delegate?.clientDidUpdateState(.connecting)

        let endpoint = NWEndpoint.hostPort(host: NWEndpoint.Host(host), port: NWEndpoint.Port(rawValue: port)!)
        let params = NWParameters.tcp
        params.preferNoPath = false

        let conn = NWConnection(to: endpoint, using: params)
        self.connection = conn

        conn.stateUpdateHandler = { [weak self] state in
            guard let self = self else { return }
            switch state {
            case .ready:
                NSLog("[SecondScreen iOS] TCP Connected to \(host):\(port). Sending PAIR_REQUEST...")
                self.delegate?.clientDidUpdateState(.pairing)
                self.sendPairRequest(pin: pin)
                self.receiveNextPacketHeader()
            case .failed(let error):
                NSLog("[SecondScreen iOS] Connection failed: \(error.localizedDescription)")
                self.delegate?.clientDidUpdateState(.error(error.localizedDescription))
            case .cancelled:
                self.delegate?.clientDidUpdateState(.disconnected)
            default:
                break
            }
        }

        conn.start(queue: queue)
    }

    private func sendPairRequest(pin: String) {
        guard let conn = connection else { return }
        var header = Data(count: PACKET_HEADER_SIZE)
        header.withUnsafeMutableBytes { ptr in
            ptr.storeBytes(of: PROTOCOL_MAGIC.littleEndian, toByteOffset: 0, as: UInt32.self)
            ptr.storeBytes(of: UInt16(1).littleEndian, toByteOffset: 4, as: UInt16.self)
            ptr.storeBytes(of: UInt8(0x03), toByteOffset: 6, as: UInt8.self) // PAIR_REQUEST
            ptr.storeBytes(of: UInt8(0), toByteOffset: 7, as: UInt8.self)
            ptr.storeBytes(of: UInt32(1).littleEndian, toByteOffset: 8, as: UInt32.self)
            ptr.storeBytes(of: UInt64(Date().timeIntervalSince1970 * 1_000_000).littleEndian, toByteOffset: 12, as: UInt64.self)
            ptr.storeBytes(of: UInt32(pin.utf8.count).littleEndian, toByteOffset: 20, as: UInt32.self)
        }

        var packet = header
        packet.append(pin.data(using: .utf8)!)

        conn.send(content: packet, completion: .contentProcessed { error in
            if let err = error {
                NSLog("[SecondScreen iOS] Failed to send PAIR_REQUEST: \(err)")
            }
        })
    }

    private func receiveNextPacketHeader() {
        guard let conn = connection else { return }

        conn.receive(minimumIncompleteLength: PACKET_HEADER_SIZE, maximumLength: PACKET_HEADER_SIZE) { [weak self] content, _, isComplete, error in
            guard let self = self, let data = content, data.count == PACKET_HEADER_SIZE else { return }

            let magic = data.withUnsafeBytes { $0.load(fromByteOffset: 0, as: UInt32.self).littleEndian }
            guard magic == PROTOCOL_MAGIC else {
                NSLog("[SecondScreen iOS] Invalid packet magic: 0x%X", magic)
                self.receiveNextPacketHeader()
                return
            }

            let type = data[6]
            let flags = data[7]
            let version = data.withUnsafeBytes { $0.load(fromByteOffset: 4, as: UInt16.self).littleEndian }
            let timestamp = data.withUnsafeBytes { $0.load(fromByteOffset: 12, as: UInt64.self).littleEndian }
            let payloadSize = Int(data.withUnsafeBytes { $0.load(fromByteOffset: 20, as: UInt32.self).littleEndian })

            guard version == 1, payloadSize <= 16 * 1024 * 1024 else {
                self.delegate?.clientDidUpdateState(.error)
                self.connection?.cancel()
                return
            }

            if type == 0x04 && payloadSize == 0 { // PAIR_RESPONSE
                NSLog("[SecondScreen iOS] Authenticated successfully. Entering STREAMING mode.")
                self.delegate?.clientDidUpdateState(.streaming)
                self.receiveNextPacketHeader()
            } else if payloadSize > 0 {
                self.receivePayload(type: type, flags: flags, timestamp: timestamp, size: payloadSize)
            } else {
                self.receiveNextPacketHeader()
            }
        }
    }

    private func receivePayload(type: UInt8, flags: UInt8, timestamp: UInt64, size: Int) {
        guard let conn = connection else { return }

        conn.receive(minimumIncompleteLength: size, maximumLength: size) { [weak self] content, _, _, error in
            guard let self = self, let payload = content else { return }

            if type == 0x08 { // FRAME
                let isKey = (flags & 0x01) != 0
                self.delegate?.clientDidReceiveVideoFrame(naluData: payload, presentationTimeUs: timestamp, isKeyFrame: isKey)
            } else if type == 0x0B && payload.count >= TELEMETRY_DATA_SIZE { // TELEMETRY
                let fps = payload.withUnsafeBytes { $0.load(fromByteOffset: 0, as: Float.self) }
                let rtt = payload.withUnsafeBytes { $0.load(fromByteOffset: 8, as: Float.self) }
                let totalLat = payload.withUnsafeBytes { $0.load(fromByteOffset: 20, as: Float.self) }
                let isMeasured = payload[36] == 1 // Byte 36: isMeasured flag

                self.delegate?.clientDidReceiveTelemetry(fps: fps, rttMs: rtt, totalLatencyMs: totalLat, isMeasured: isMeasured)
            } else if type == 0x04 {
                self.delegate?.clientDidUpdateState(.streaming)
            }

            self.receiveNextPacketHeader()
        }
    }

    public func sendInputEvent(actionType: UInt8, normX: Float, normY: Float, pressure: Float, button: UInt8) {
        guard let conn = connection else { return }

        var header = Data(count: PACKET_HEADER_SIZE)
        header.withUnsafeMutableBytes { ptr in
            ptr.storeBytes(of: PROTOCOL_MAGIC.littleEndian, toByteOffset: 0, as: UInt32.self)
            ptr.storeBytes(of: UInt16(1).littleEndian, toByteOffset: 4, as: UInt16.self)
            ptr.storeBytes(of: UInt8(0x0C), toByteOffset: 6, as: UInt8.self) // INPUT_EVENT
            ptr.storeBytes(of: UInt8(0), toByteOffset: 7, as: UInt8.self)
            ptr.storeBytes(of: UInt32(0).littleEndian, toByteOffset: 8, as: UInt32.self)
            ptr.storeBytes(of: UInt64(Date().timeIntervalSince1970 * 1_000_000).littleEndian, toByteOffset: 12, as: UInt64.self)
            ptr.storeBytes(of: UInt32(INPUT_EVENT_SIZE).littleEndian, toByteOffset: 20, as: UInt32.self)
        }

        var payload = Data(count: INPUT_EVENT_SIZE)
        payload.withUnsafeMutableBytes { ptr in
            ptr.storeBytes(of: actionType, toByteOffset: 0, as: UInt8.self)
            ptr.storeBytes(of: normX, toByteOffset: 1, as: Float.self)
            ptr.storeBytes(of: normY, toByteOffset: 5, as: Float.self)
            ptr.storeBytes(of: pressure, toByteOffset: 9, as: Float.self)
            ptr.storeBytes(of: button, toByteOffset: 13, as: UInt8.self)
            ptr.storeBytes(of: UInt64(Date().timeIntervalSince1970 * 1000).littleEndian, toByteOffset: 14, as: UInt64.self)
        }

        var packet = header
        packet.append(payload)

        conn.send(content: packet, completion: .contentProcessed { _ in })
    }

    public func disconnect() {
        connection?.cancel()
        connection = nil
        delegate?.clientDidUpdateState(.disconnected)
    }
}
