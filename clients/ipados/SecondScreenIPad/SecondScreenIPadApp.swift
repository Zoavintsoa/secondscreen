import SwiftUI
import VideoToolbox
import MetalKit

@main
struct SecondScreenIPadApp: App {
    var body: some Scene {
        WindowGroup {
            ContentView()
        }
    }
}

private struct ContentView: View {
    var body: some View {
        ZStack {
            Color.black.ignoresSafeArea()
            Text("SecondScreen")
                .foregroundStyle(.white)
                .font(.title)
        }
    }
}
