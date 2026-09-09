// SecondScreen ScreenCaptureKit GPU Capture Engine (macOS 12.3 Monterey, 13, 14, 15)
// Zero-copy Metal texture capture pipeline directly acquiring OS Display 2 frames
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

import Foundation
import ScreenCaptureKit
import Metal
import CoreMedia
import CoreVideo

public protocol MacCaptureDelegate: AnyObject {
    func didCaptureGPUFrame(pixelBuffer: CVPixelBuffer, presentationTimeUs: UInt64, frameNumber: UInt64)
    func captureDidEncounterError(error: Error)
}

public class MacScreenCaptureSource: NSObject, SCStreamOutput, SCStreamDelegate {
    private var stream: SCStream?
    private let targetDisplayID: CGDirectDisplayID
    private let metalDevice: MTLDevice
    private var frameSequence: UInt64 = 0
    private let clockStart = DispatchTime.now().uptimeNanoseconds

    public weak var delegate: MacCaptureDelegate?

    public init(displayID: CGDirectDisplayID) {
        self.targetDisplayID = displayID
        self.metalDevice = MTLCreateSystemDefaultDevice()!
        super.init()
    }

    public func startCapture(width: Int, height: Int, fps: Int) async throws {
        // Enumerate shareable OS content to locate the SecondScreen virtual display
        let content = try await SCShareableContent.current
        guard let display = content.displays.first(where: { $0.displayID == self.targetDisplayID }) else {
            throw NSError(
                domain: "com.secondscreen.macos",
                code: 404,
                userInfo: [NSLocalizedDescriptionKey: "Target virtual display (ID \(self.targetDisplayID)) not found in OS topology."]
            )
        }

        let filter = SCContentFilter(display: display, excludingApplications: [], exceptingWindows: [])
        let config = SCStreamConfiguration()
        config.width = width
        config.height = height
        config.minimumFrameInterval = CMTime(value: 1, timescale: CMTimeScale(fps))
        config.pixelFormat = kCVPixelFormatType_32BGRA
        config.showsCursor = true
        config.scalesToFit = false
        config.queueDepth = 3

        let newStream = SCStream(filter: filter, configuration: config, delegate: self)
        let queue = DispatchQueue(label: "com.secondscreen.capture.queue", qos: .userInteractive)
        try newStream.addStreamOutput(self, type: .screen, sampleHandlerQueue: queue)
        try await newStream.startCapture()
        self.stream = newStream

        NSLog("[SecondScreen macOS] ScreenCaptureKit zero-copy capture active on Display ID: %u (%dx%d @ %d fps)", 
              self.targetDisplayID, width, height, fps)
    }

    public func stopCapture() async {
        if let activeStream = self.stream {
            try? await activeStream.stopCapture()
            self.stream = nil
            NSLog("[SecondScreen macOS] ScreenCaptureKit capture stopped.")
        }
    }

    public func stream(_ stream: SCStream, didOutputSampleBuffer sampleBuffer: CMSampleBuffer, of type: SCStreamOutputType) {
        guard type == .screen, let imageBuffer = sampleBuffer.imageBuffer else { return }

        let nowUs = (DispatchTime.now().uptimeNanoseconds - clockStart) / 1000
        frameSequence += 1

        delegate?.didCaptureGPUFrame(
            pixelBuffer: imageBuffer,
            presentationTimeUs: nowUs,
            frameNumber: frameSequence
        )
    }

    public func stream(_ stream: SCStream, didStopWithError error: Error) {
        NSLog("[SecondScreen macOS] Stream stopped with error: %@", error.localizedDescription)
        delegate?.captureDidEncounterError(error: error)
    }
}
