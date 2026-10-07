import SwiftUI

@main
struct SecondScreenMacHostApp: App {
    var body: some Scene {
        WindowGroup {
            ContentView()
        }
    }
}

private struct ContentView: View {
    var body: some View {
        VStack(spacing: 12) {
            Text("SecondScreen — Zoavintsoa")
                .font(.title)
            Text("macOS host foundation")
                .foregroundColor(.secondary)
            Text("Capture/encode boundaries are isolated by macOS capability tier.")
                .font(.caption)
                .multilineTextAlignment(.center)
            Text("Virtual display: not available through a supported public API on this target.")
                .font(.caption2)
                .foregroundColor(.secondary)
                .multilineTextAlignment(.center)
        }
        .padding(32)
        .frame(minWidth: 460, minHeight: 250)
    }
}
