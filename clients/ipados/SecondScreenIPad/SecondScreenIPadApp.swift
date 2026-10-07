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
        if #available(iOS 12.1, *) {
            pencil=UIPencilInteraction.prefersPencilOnlyDrawing
        } else {
            pencil=false
        }
        let metal=MTLCreateSystemDefaultDevice() != nil
        return IPadCapabilities(
            osVersion:device.systemVersion,
            maxRefreshRate:screen.maximumFramesPerSecond,
            touch:true,
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
    @State private var showingAbout=false
    @State private var showingTerms=false
    @State private var showingPrivacy=false

    var body: some View {
        ZStack {
            Color.black.ignoresSafeArea()
            VStack(spacing:16) {
                Text("SecondScreen — Zoavintsoa")
                    .foregroundColor(.white)
                    .font(.title)
                Text("Your devices become your workspace.")
                    .foregroundColor(.gray)
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

                HStack {
                    Button("À propos") { showingAbout=true }
                    Button("Conditions") { showingTerms=true }
                    Button("Confidentialité") { showingPrivacy=true }
                }
                .padding(.top, 8)
            }
            .padding(32)
        }
        .sheet(isPresented: $showingAbout) {
            LegalSheet(title:"À propos de SecondScreen — Zoavintsoa", body: aboutText)
        }
        .sheet(isPresented: $showingTerms) {
            LegalSheet(title:"Conditions d’utilisation", body: termsText)
        }
        .sheet(isPresented: $showingPrivacy) {
            LegalSheet(title:"Politique de confidentialité", body: privacyText)
        }
    }

    private let aboutText = """
SecondScreen — Zoavintsoa

Your devices become your workspace.

Créé et développé par Zoavintsoa.
Native • Cross-platform • Local-first

Cible iPadOS 13 et versions compatibles selon les capacités réelles de l’appareil.
Version 0.1.0.
"""

    private let termsText = """
SecondScreen — Conditions d’utilisation

SecondScreen est un logiciel de deuxième écran développé par Zoavintsoa.

Vous êtes responsable de l’utilisation des appareils, réseaux et contenus auxquels vous donnez accès. L’appairage doit être effectué uniquement entre appareils autorisés.

Le produit est en développement : certaines fonctions peuvent être expérimentales ou indisponibles selon l’OS et le matériel. Aucune fonctionnalité n’est considérée comme matériellement validée tant qu’elle n’a pas été testée sur l’appareil concerné.

Les composants tiers restent soumis à leurs licences et notices respectives.
"""

    private let privacyText = """
SecondScreen — Politique de confidentialité

Le produit est conçu local-first. Aucun compte ou cloud n’est requis pour le fonctionnement LAN prévu.

Les informations nécessaires au fonctionnement, comme l’identifiant local de l’appareil et les informations de session d’appairage, peuvent être conservées localement.

Les fonctions caméra/microphone ne doivent être utilisées que lorsqu’elles sont explicitement activées et autorisées par le système.

SecondScreen ne vend pas les données utilisateur et n’ajoute pas d’analytique cachée au produit.
"""
}

private struct LegalSheet: View {
    let title:String
    let body:String

    var bodyView: some View {
        ScrollView {
            Text(body)
                .frame(maxWidth:.infinity, alignment:.leading)
                .padding(24)
                .textSelection(.enabled)
        }
    }

    var body: some View {
        NavigationStack {
            bodyView
                .navigationTitle(title)
                .navigationBarTitleDisplayMode(.inline)
        }
    }
}
