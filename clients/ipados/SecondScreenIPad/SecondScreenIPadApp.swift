import SwiftUI
import UIKit
import VideoToolbox
import MetalKit

struct IPadCapabilities {
    let osVersion:String
    let maxRefreshRate:Int
    let touch:Bool
    let pencil:Bool
    let metal:Bool
    let h264:Bool
    let hevc:Bool

    static func probe() -> IPadCapabilities {
        let device=UIDevice.current
        let screen=UIScreen.main
        let h264=true
        let hevc=VTIsHardwareDecodeSupported(kCMVideoCodecType_HEVC)
        let pencil:Bool
        if #available(iOS 12.1, *) { pencil=UIPencilInteraction.prefersPencilOnlyDrawing } else { pencil=false }
        let metal=MTLCreateSystemDefaultDevice() != nil
        return IPadCapabilities(
            osVersion:device.systemVersion,
            maxRefreshRate:screen.maximumFramesPerSecond,
            touch:screen.traitCollection.userInterfaceIdiom == .pad,
            pencil:pencil,
            metal:metal,
            h264:h264,
            hevc:hevc
        )
    }
}

@main
struct SecondScreenIPadApp: App {
    var body: some Scene {
        WindowGroup { ContentView() }
    }
}

private struct ContentView: View {
    @State private var capabilities=IPadCapabilities.probe()

    var body: some View {
        ZStack {
            Color.black.edgesIgnoringSafeArea(.all)
            VStack(spacing:16) {
                Text("SecondScreen")
                    .foregroundColor(.white)
                    .font(.title)
                Text("iPadOS " + capabilities.osVersion)
                    .foregroundColor(.gray)
                Text(capabilities.h264 ? "H.264 ✓" : "H.264 indisponible")
                    .foregroundColor(.white)
                Text(capabilities.hevc ? "HEVC ✓" : "HEVC — fallback H.264")
                    .foregroundColor(.white)
                Text(capabilities.metal ? "Metal ✓" : "Renderer conservateur")
                    .foregroundColor(.white)
                Text(capabilities.pencil ? "Pencil ✓" : "Touch uniquement")
                    .foregroundColor(.white)
            }
        }
    }
}
