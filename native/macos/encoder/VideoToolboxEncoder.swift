// SecondScreen Apple VideoToolbox Hardware Video Encoder (Apple Silicon M1/M2/M3/M4 & Intel)
// Sub-frame low-latency CBR compression pipeline
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

import Foundation
import VideoToolbox
import CoreMedia
import CoreVideo

public protocol MacEncoderDelegate: AnyObject {
    func didEncodeFrame(naluData: Data, isKeyFrame: Bool, presentationTimeUs: UInt64, frameNumber: UInt64)
}

public class VideoToolboxEncoder {
    private var compressionSession: VTCompressionSession?
    private var frameIndex: UInt64 = 0
    public weak var delegate: MacEncoderDelegate?

    public init() {}

    public func initialize(width: Int32, height: Int32, fps: Int32 = 60, bitrateKbps: Int32 = 20000, codec: CMVideoCodecType = kCMVideoCodecType_H264) -> Bool {
        shutdown()

        let encoderSpecification: [NSString: Any] = [
            kVTVideoEncoderSpecification_EnableHardwareAcceleratedVideoEncoder: true,
            kVTVideoEncoderSpecification_RequireHardwareAcceleratedVideoEncoder: true
        ]

        var session: VTCompressionSession?
        let callback: VTCompressionOutputCallback = { (outputCallbackRefCon, sourceFrameRefCon, status, infoFlags, sampleBuffer) in
            guard status == noErr, let buffer = sampleBuffer else { return }
            let encoder = Unmanaged<VideoToolboxEncoder>.fromOpaque(outputCallbackRefCon!).takeUnretainedValue()
            encoder.processEncodedSampleBuffer(buffer)
        }

        let status = VTCompressionSessionCreate(
            allocator: kCFAllocatorDefault,
            width: width,
            height: height,
            codecType: codec,
            encoderSpecification: encoderSpecification as CFDictionary,
            imageBufferAttributes: nil,
            compressedDataAllocator: nil,
            outputCallback: callback,
            refcon: Unmanaged.passUnretained(self).toOpaque(),
            compressionSessionOut: &session
        )

        guard status == noErr, let compression = session else {
            NSLog("[SecondScreen macOS] VTCompressionSessionCreate failed: %d", status)
            return false
        }

        self.compressionSession = compression

        // Configure Low-Latency Realtime Streaming Properties
        VTSessionSetProperty(compression, key: kVTCompressionPropertyKey_RealTime, value: kCFBooleanTrue)
        VTSessionSetProperty(compression, key: kVTCompressionPropertyKey_ProfileLevel, value: kVTProfileLevel_H264_High_AutoLevel)
        VTSessionSetProperty(compression, key: kVTCompressionPropertyKey_AverageBitRate, value: (bitrateKbps * 1000) as CFNumber)
        VTSessionSetProperty(compression, key: kVTCompressionPropertyKey_MaxKeyFrameInterval, value: fps as CFNumber)
        VTSessionSetProperty(compression, key: kVTCompressionPropertyKey_AllowFrameReordering, value: kCFBooleanFalse) // 0 B-frames for sub-16ms latency
        VTSessionSetProperty(compression, key: kVTCompressionPropertyKey_ExpectedFrameRate, value: fps as CFNumber)

        VTCompressionSessionPrepareToEncodeFrames(compression)
        NSLog("[SecondScreen macOS] VideoToolbox hardware encoder initialized (%dx%d @ %d fps, %d kbps CBR)", 
              width, height, fps, bitrateKbps)
        return true
    }

    public func encodePixelBuffer(_ pixelBuffer: CVPixelBuffer, presentationTimeUs: UInt64, forceKeyFrame: Bool) {
        guard let session = compressionSession else { return }

        frameIndex += 1
        let pts = CMTime(value: Int64(presentationTimeUs), timescale: 1_000_000)
        let frameProperties: [NSString: Any]? = forceKeyFrame ? [kVTEncodeFrameOptionKey_ForceKeyFrame: true] : nil

        let status = VTCompressionSessionEncodeFrame(
            session,
            imageBuffer: pixelBuffer,
            presentationTimeStamp: pts,
            duration: CMTime.invalid,
            frameProperties: frameProperties as CFDictionary?,
            sourceFrameRefcon: nil,
            infoFlagsOut: nil
        )

        if status != noErr {
            NSLog("[SecondScreen macOS] VTCompressionSessionEncodeFrame error: %d", status)
        }
    }

    private func processEncodedSampleBuffer(_ sampleBuffer: CMSampleBuffer) {
        guard let dataBuffer = CMSampleBufferGetDataBuffer(sampleBuffer) else { return }

        var totalLength: Int = 0
        var dataPointer: UnsafeMutablePointer<Int8>?
        let status = CMBlockBufferGetDataPointer(dataBuffer, atOffset: 0, lengthAtOffsetOut: nil, totalLengthOut: &totalLength, dataPointerOut: &dataPointer)

        guard status == noErr, let ptr = dataPointer, totalLength > 0 else { return }

        let naluData = Data(bytes: ptr, count: totalLength)
        let isKey = isSampleBufferKeyFrame(sampleBuffer)
        let pts = UInt64(CMTimeGetSeconds(CMSampleBufferGetPresentationTimeStamp(sampleBuffer)) * 1_000_000)

        delegate?.didEncodeFrame(
            naluData: naluData,
            isKeyFrame: isKey,
            presentationTimeUs: pts,
            frameNumber: frameIndex
        )
    }

    private func isSampleBufferKeyFrame(_ sampleBuffer: CMSampleBuffer) -> Bool {
        guard let attachments = CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, createIfNecessary: false) as? [[CFString: Any]],
              let first = attachments.first else {
            return true
        }
        let notSync = first[kCMSampleAttachmentKey_NotSync] as? Bool ?? false
        return !notSync
    }

    public func shutdown() {
        if let session = compressionSession {
            VTCompressionSessionInvalidate(session)
            self.compressionSession = nil
        }
    }

    deinit {
        shutdown()
    }
}
