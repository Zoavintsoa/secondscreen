// SecondScreen Metal-Accelerated Remote Display Viewport for iPadOS & iOS
// High-performance CAMetalLayer rendering with Apple Pencil pressure and tilt serialization
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

import UIKit
import MetalKit
import CoreVideo

public protocol IOSInputDelegate: AnyObject {
    func didCaptureInputEvent(actionType: UInt8, normX: Float, normY: Float, pressure: Float, button: UInt8)
}

public class SecondScreenMetalView: MTKView {
    private var commandQueue: MTLCommandQueue?
    private var textureCache: CVMetalTextureCache?
    private var currentTexture: MTLTexture?

    public weak var inputDelegate: IOSInputDelegate?

    public init(frame: CGRect) {
        let device = MTLCreateSystemDefaultDevice()!
        super.init(frame: frame, device: device)
        commonInit(device: device)
    }

    required init(coder: NSCoder) {
        let device = MTLCreateSystemDefaultDevice()!
        super.init(coder: coder)
        self.device = device
        commonInit(device: device)
    }

    private func commonInit(device: MTLDevice) {
        self.commandQueue = device.makeCommandQueue()
        self.framebufferOnly = false
        self.isMultipleTouchEnabled = true
        self.isOpaque = true
        self.backgroundColor = .black
        self.contentMode = .scaleAspectFit

        CVMetalTextureCacheCreate(kCFAllocatorDefault, nil, device, nil, &self.textureCache)
    }

    public func renderPixelBuffer(_ pixelBuffer: CVPixelBuffer) {
        guard let cache = textureCache else { return }

        let width = CVPixelBufferGetWidth(pixelBuffer)
        let height = CVPixelBufferGetHeight(pixelBuffer)

        var cvMetalTexture: CVMetalTexture?
        let status = CVMetalTextureCacheCreateTextureFromImage(
            kCFAllocatorDefault,
            cache,
            pixelBuffer,
            nil,
            .bgra8Unorm,
            width,
            height,
            0,
            &cvMetalTexture
        )

        guard status == kCVReturnSuccess, let metalTex = cvMetalTexture else { return }
        self.currentTexture = CVMetalTextureGetTexture(metalTex)

        // Trigger redraw
        self.setNeedsDisplay()
    }

    public override func draw(_ rect: CGRect) {
        guard let drawable = currentDrawable,
              let texture = currentTexture,
              let cmdBuffer = commandQueue?.makeCommandBuffer() else { return }

        if let blitEncoder = cmdBuffer.makeBlitCommandEncoder() {
            let width = min(texture.width, drawable.texture.width)
            let height = min(texture.height, drawable.texture.height)
            if width > 0 && height > 0 {
                blitEncoder.copy(
                    from: texture,
                    sourceSlice: 0,
                    sourceLevel: 0,
                    sourceOrigin: MTLOrigin(x: 0, y: 0, z: 0),
                    sourceSize: MTLSize(width: width, height: height, depth: 1),
                    to: drawable.texture,
                    destinationSlice: 0,
                    destinationLevel: 0,
                    destinationOrigin: MTLOrigin(x: 0, y: 0, z: 0)
                )
            }
            blitEncoder.endEncoding()
        }

        cmdBuffer.present(drawable)
        cmdBuffer.commit()
    }


    // Touch & Apple Pencil Input Capture
    public override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        handleTouch(touches.first, actionType: 0) // DOWN
    }

    public override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        handleTouch(touches.first, actionType: 1) // MOVE
    }

    public override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        handleTouch(touches.first, actionType: 2) // UP
    }

    public override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        handleTouch(touches.first, actionType: 2) // CANCEL
    }

    private func handleTouch(_ touch: UITouch?, actionType: UInt8) {
        guard let t = touch, bounds.width > 0, bounds.height > 0 else { return }

        let loc = t.location(in: self)
        let normX = Float(max(0.0, min(1.0, loc.x / bounds.width)))
        let normY = Float(max(0.0, min(1.0, loc.y / bounds.height)))
        let pressure = Float(t.force > 0 ? (t.force / max(1.0, t.maximumPossibleForce)) : 1.0)
        let button: UInt8 = (t.type == .pencil) ? 4 : 0 // Stylus flag

        inputDelegate?.didCaptureInputEvent(
            actionType: actionType,
            normX: normX,
            normY: normY,
            pressure: pressure,
            button: button
        )
    }
}
