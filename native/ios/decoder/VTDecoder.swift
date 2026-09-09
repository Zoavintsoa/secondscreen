// SecondScreen iOS/iPadOS VideoToolbox Hardware Decompression Engine
// Zero-copy decoding directly yielding CVPixelBuffer for Metal rendering
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

import Foundation
import VideoToolbox
import CoreMedia

public protocol IOSDecoderDelegate: AnyObject {
    func didDecompressPixelBuffer(_ pixelBuffer: CVPixelBuffer, presentationTimeUs: UInt64)
    func decoderDidEncounterError(status: OSStatus)
}

public class IOSHardwareDecoder {
    private var decompressionSession: VTDecompressionSession?
    private var formatDescription: CMVideoFormatDescription?
    private var currentSPS: Data?
    private var currentPPS: Data?
    public weak var delegate: IOSDecoderDelegate?

    public init() {}

    public func configureDecoder(sps: Data, pps: Data) -> Bool {
        if currentSPS == sps && currentPPS == pps && decompressionSession != nil {
            return true
        }

        shutdown()
        currentSPS = sps
        currentPPS = pps

        let parameterSets = [
            sps.withUnsafeBytes { $0.baseAddress! },
            pps.withUnsafeBytes { $0.baseAddress! }
        ]
        let parameterSetSizes = [sps.count, pps.count]

        var formatDesc: CMVideoFormatDescription?
        let status = CMVideoFormatDescriptionCreateFromH264ParameterSets(
            allocator: kCFAllocatorDefault,
            parameterSetCount: 2,
            parameterSetPointers: parameterSets.map { UnsafePointer<UInt8>($0.assumingMemoryBound(to: UInt8.self)) },
            parameterSetSizes: parameterSetSizes,
            nalUnitHeaderLength: 4,
            formatDescriptionOut: &formatDesc
        )

        guard status == noErr, let format = formatDesc else {
            NSLog("[SecondScreen iOS] [VTDecoder] Failed to create format description from H.264 parameter sets. OSStatus=%d", status)
            delegate?.decoderDidEncounterError(status: status)
            return false
        }
        self.formatDescription = format
        NSLog("[SecondScreen iOS] [VTDecoder] Format description created successfully from SPS (%d bytes) & PPS (%d bytes).", sps.count, pps.count)

        let destinationImageBufferAttributes: [NSString: Any] = [
            kCVPixelBufferPixelFormatTypeKey: kCVPixelFormatType_32BGRA,
            kCVPixelBufferMetalCompatibilityKey: true,
            kCVPixelBufferOpenGLCompatibilityKey: true
        ]

        var callbackRecord = VTDecompressionOutputCallbackRecord(
            decompressionOutputCallback: { (decompressionOutputRefCon, sourceFrameRefCon, status, infoFlags, imageBuffer, presentationTimeStamp, presentationDuration) in
                guard status == noErr, let pixelBuffer = imageBuffer else {
                    NSLog("[SecondScreen iOS] [VTDecoder] Decompression callback error: OSStatus=%d", status)
                    return
                }
                let decoder = Unmanaged<IOSHardwareDecoder>.fromOpaque(decompressionOutputRefCon!).takeUnretainedValue()
                let pts = UInt64(CMTimeGetSeconds(presentationTimeStamp) * 1_000_000)
                decoder.delegate?.didDecompressPixelBuffer(pixelBuffer, presentationTimeUs: pts)
            },
            decompressionOutputRefCon: Unmanaged.passUnretained(self).toOpaque()
        )

        var session: VTDecompressionSession?
        let sessionStatus = VTDecompressionSessionCreate(
            allocator: kCFAllocatorDefault,
            formatDescription: format,
            videoDecoderSpecification: nil,
            destinationImageBufferAttributes: destinationImageBufferAttributes as CFDictionary,
            outputCallback: &callbackRecord,
            decompressionSessionOut: &session
        )

        guard sessionStatus == noErr, let decomp = session else {
            NSLog("[SecondScreen iOS] [VTDecoder] VTDecompressionSessionCreate failed. OSStatus=%d", sessionStatus)
            delegate?.decoderDidEncounterError(status: sessionStatus)
            return false
        }

        self.decompressionSession = decomp
        NSLog("[SecondScreen iOS] [VTDecoder] VTDecompressionSession created successfully.")
        return true
    }

    public func decodeFrame(naluData: Data, presentationTimeUs: UInt64) {
        // 1. Parse Annex-B NAL units to extract SPS/PPS and convert to AVCC format
        var spsData: Data?
        var ppsData: Data?
        var avccData = Data()

        var i = 0
        let count = naluData.count

        while i < count {
            var startCodeLen = 0
            if i + 4 <= count && naluData[i] == 0 && naluData[i+1] == 0 && naluData[i+2] == 0 && naluData[i+3] == 1 {
                startCodeLen = 4
            } else if i + 3 <= count && naluData[i] == 0 && naluData[i+1] == 0 && naluData[i+2] == 1 {
                startCodeLen = 3
            }

            if startCodeLen > 0 {
                let naluStart = i + startCodeLen
                var nextStart = count
                var j = naluStart
                while j + 3 <= count {
                    if (j + 4 <= count && naluData[j] == 0 && naluData[j+1] == 0 && naluData[j+2] == 0 && naluData[j+3] == 1) ||
                       (naluData[j] == 0 && naluData[j+1] == 0 && naluData[j+2] == 1) {
                        nextStart = j
                        break
                    }
                    j += 1
                }

                let naluLength = nextStart - naluStart
                if naluLength > 0 {
                    let naluSlice = naluData.subdata(in: naluStart..<nextStart)
                    let naluType = naluSlice[0] & 0x1F

                    if naluType == 7 { // SPS
                        spsData = naluSlice
                        NSLog("[SecondScreen iOS] [VTDecoder] SPS received (%d bytes)", naluLength)
                    } else if naluType == 8 { // PPS
                        ppsData = naluSlice
                        NSLog("[SecondScreen iOS] [VTDecoder] PPS received (%d bytes)", naluLength)
                    } else if naluType == 5 { // IDR
                        NSLog("[SecondScreen iOS] [VTDecoder] IDR frame received (%d bytes)", naluLength)
                    }

                    var lenBigEndian = UInt32(naluLength).bigEndian
                    withUnsafeBytes(of: &lenBigEndian) { avccData.append(contentsOf: $0) }
                    avccData.append(naluSlice)
                }

                i = nextStart
            } else {
                i += 1
            }
        }

        // 2. Configure decoder dynamically if SPS and PPS are present
        if let sps = spsData, let pps = ppsData {
            _ = configureDecoder(sps: sps, pps: pps)
        }

        guard let session = decompressionSession, let format = formatDescription, !avccData.isEmpty else { return }

        // 3. Create CMBlockBuffer with AVCC data
        var blockBuffer: CMBlockBuffer?
        var status = CMBlockBufferCreateWithMemoryBlock(
            allocator: kCFAllocatorDefault,
            memoryBlock: nil,
            blockLength: avccData.count,
            blockAllocator: kCFAllocatorDefault,
            customBlockSource: nil,
            offsetToData: 0,
            dataLength: avccData.count,
            flags: 0,
            blockBufferOut: &blockBuffer
        )

        guard status == noErr, let buffer = blockBuffer else { return }

        avccData.withUnsafeBytes { rawPtr in
            CMBlockBufferReplaceDataBytes(with: rawPtr.baseAddress!, blockBuffer: buffer, offsetIntoDestination: 0, dataLength: avccData.count)
        }

        // 4. Create CMSampleBuffer
        var sampleBuffer: CMSampleBuffer?
        let sampleSize = avccData.count
        var timingInfo = CMSampleTimingInfo(
            duration: .invalid,
            presentationTimeStamp: CMTime(value: Int64(presentationTimeUs), timescale: 1_000_000),
            decodeTimeStamp: .invalid
        )

        status = CMSampleBufferCreateReady(
            allocator: kCFAllocatorDefault,
            dataBuffer: buffer,
            formatDescription: format,
            sampleCount: 1,
            sampleTimingEntryCount: 1,
            sampleTimingArray: &timingInfo,
            sampleSizeEntryCount: 1,
            sampleSizeArray: [sampleSize],
            sampleBufferOut: &sampleBuffer
        )

        guard status == noErr, let readySample = sampleBuffer else { return }

        // 5. Submit frame to VideoToolbox Hardware Decoder
        let decodeStatus = VTDecompressionSessionDecodeFrame(
            session,
            sampleBuffer: readySample,
            flags: [._EnableAsynchronousDecompression],
            frameRefcon: nil,
            infoFlagsOut: nil
        )

        if decodeStatus != noErr {
            NSLog("[SecondScreen iOS] [VTDecoder] Frame submit error: OSStatus=%d", decodeStatus)
            delegate?.decoderDidEncounterError(status: decodeStatus)
        }
    }

    public func shutdown() {
        if let session = decompressionSession {
            VTDecompressionSessionInvalidate(session)
            self.decompressionSession = nil
        }
        self.formatDescription = nil
        self.currentSPS = nil
        self.currentPPS = nil
    }

    deinit {
        shutdown()
    }
}


