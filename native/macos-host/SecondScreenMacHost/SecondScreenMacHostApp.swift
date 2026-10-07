import SwiftUI
import ScreenCaptureKit
import VideoToolbox

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
            Text("SecondScreen")
                .font(.title)
            Text("macOS host foundation")
                .foregroundStyle(.secondary)
            Text("Virtual-display transport remains local-first.")
                .font(.caption)
        }
        .padding(32)
        .frame(minWidth: 420, minHeight: 220)
    }
}
