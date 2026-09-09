# SECOND SCREEN — AUDIT D'INTÉGRATION OPEN SOURCE

Audit réalisé le 9 septembre 2026, à partir de `docs/project-audit-2026-09-09.md` et `docs/implementation-progress.md`. Recherche effectuée en ligne (dépôts GitHub, pages officielles) le jour de l'audit ; les licences peuvent évoluer, à revérifier avant intégration réelle.

**Règle appliquée :** aucun code n'a été copié ou intégré dans le dépôt à l'issue de cet audit. C'est une analyse comparative, pas une modification.

---

## 0. RAPPEL DE L'ÉTAT ACTUEL (résumé de l'audit fonctionnel)

D'après `project-audit-2026-09-09.md` et `implementation-progress.md` :

- Driver IddCx : implémenté, cohérent, **non compilé**
- Capture DXGI : implémentée, **non testée sur GPU réel**
- NVENC : intégré au code source, **non compilé**, QuickSync/AMF sont des stubs refusés par la factory
- Réseau : TCP framé et authentifié par PIN, **canal vidéo dédié absent**, pas de chiffrement
- Écran virtuel Windows : dépend du driver, **jamais installé physiquement**
- Android/iOS : décodeurs amorcés, **jamais compilés/testés sur appareil**
- macOS virtuel : s'appuie sur des API privées CoreDisplay, **non distribuable**

Conclusion : le projet a une architecture cohérente mais rien n'est encore validé physiquement. L'intégration open source doit combler des lacunes précises (transport bas-niveau, référence IddCx, référence NVENC/D3D11), pas remplacer une architecture qui fonctionne déjà.

---

## 1. PROJETS ANALYSÉS

### 1.1 Windows Virtual Display

| Projet | Repository | Licence | Composant intéressant | Constat |
| --- | --- | --- | --- | --- |
| **Microsoft Indirect Display Driver Sample** | `microsoft/Windows-driver-samples/video/IndirectDisplay` | MIT (dépôt Windows-driver-samples) | Driver IddCx de référence officiel (`Direct3DDevice`, `SwapChainProcessor`, `IndirectDeviceContext`) | C'est **la base sur laquelle notre propre `Driver.cpp`/`Device.cpp` est déjà construit** — mêmes concepts (WDF contexts, swapchain worker thread, D3D11 render device). Rien à importer, juste à comparer. |
| **itsmikethetech/Virtual-Display-Driver** | `github.com/itsmikethetech/Virtual-Display-Driver` | MIT + CC0/Public Domain (revendiqué par l'auteur, dérivé du sample Microsoft) | Version packagée/durcie du sample Microsoft, EDID haute résolution, scripts d'installation, usage confirmé avec Sunshine/OBS/VR | Utile **comme référence de packaging et de checklist d'installation** (ex. génération EDID via `edid-generator`), pas comme remplacement de notre driver qui est déjà à un niveau de complétude comparable. |

**Verdict Étape 5.1 :** Ne pas remplacer notre IddCx. Notre implémentation suit déjà le modèle officiel. Utiliser le repo `itsmikethetech` uniquement comme **référence croisée** pour la checklist d'installation et la génération EDID, pas comme dépendance de code.

### 1.2 Capture d'écran / GPU

Recherché : DXGI Desktop Duplication zero-copy, Windows Graphics Capture, capture GPU sans copie CPU.

- Aucun projet open source autonome et maintenu n'a été identifié qui améliore fondamentalement le chemin **DXGI Desktop Duplication → texture D3D11** que notre `DXGICapture.cpp` implémente déjà. C'est une API Windows native (pas de bibliothèque tierce à intégrer) ; la bonne pratique (garder la texture côté GPU, éviter `Map`/`CopyResource` vers la RAM) est une question d'implémentation, pas de dépendance externe.
- **Windows Graphics Capture API** (`Windows.Graphics.Capture`) est une alternative native plus récente que DXGI Desktop Duplication, avec un support multi-moniteur plus simple, mais **elle est conçue pour capturer des fenêtres/écrans visibles à l'utilisateur** — moins adaptée pour capturer spécifiquement un moniteur virtuel IddCx headless que DXGI Desktop Duplication, qui reste le bon choix pour ce cas d'usage.

**Verdict :** Rien à intégrer ici. Le vrai travail restant est l'exécution et la mesure réelles sur GPU, pas une nouvelle dépendance.

### 1.3 Encodage matériel (NVENC)

| Projet | Repository | Licence | Composant intéressant | Constat |
| --- | --- | --- | --- | --- |
| **NVIDIA Video Codec SDK Samples** | `github.com/NVIDIA/video-sdk-samples` | Licence d'échantillons NVIDIA (permissive, style BSD/MIT — usage/modification/distribution autorisés, sans garantie) | `NvEncoderD3D11.cpp/.h` — wrapper de référence officiel pour piloter NVENC directement depuis une texture D3D11, sans repasser par la RAM | **Directement pertinent.** C'est la référence canonique pour le chemin `GPU texture → NVENC` que notre `NVENCEncoder.cpp` doit implémenter. Notre `nvEncodeAPI.h` vendu vient déjà de la même famille d'API (via `nv-codec-headers`, licence MIT/LGPL selon le fichier — à revérifier fichier par fichier). |
| **NVIDIA/NvPipe** | `github.com/NVIDIA/NvPipe` | BSD-3-Clause | Wrapper haut niveau NVENC/NVDEC | **Déprécié** par NVIDIA elle-même, qui redirige vers les wrappers `NvCodec` du Video Codec SDK ci-dessus. Ne pas utiliser. |

**Verdict Étape 5.2 :** Utiliser `NvEncoderD3D11.cpp/.h` de `NVIDIA/video-sdk-samples` **comme référence de code à consulter/adapter** (pas comme dépendance liée) pour terminer l'implémentation stub de `HardwareEncoder::EncodeTexture()`. Licence NVIDIA d'échantillons : vérifier le fichier de licence exact dans le dépôt avant toute copie littérale de code, même petite.

### 1.4 Streaming réseau bas niveau

| Projet | Repository | Licence | Composant intéressant | Constat |
| --- | --- | --- | --- | --- |
| **msquic (Microsoft)** | `github.com/microsoft/msquic` | **MIT** | Implémentation QUIC complète, C/C++, optimisée latence, faite pour Windows en priorité (Schannel), utilisée en production par Microsoft (SMB over QUIC, HTTP/3) | **Le candidat le plus solide identifié dans tout cet audit.** QUIC apporte nativement : multiplexage de flux (contrôle + vidéo séparés sans tête-de-ligne bloquante), 0-RTT, chiffrement TLS 1.3 obligatoire, tolérance au changement de réseau — répond directement aux lacunes listées dans `project-audit-2026-09-09.md` (pas de canal vidéo dédié, pas de chiffrement). |
| **Google WebRTC (libwebrtc)** | `webrtc.googlesource.com` / miroirs GitHub | BSD-3-Clause | Pile complète audio/vidéo temps réel, NAT traversal (ICE/STUN/TURN), congestion control adaptatif | Techniquement excellent, mais **coût d'intégration disproportionné** pour un usage LAN pur : build system `depot_tools` très lourd, dépendances transitives nombreuses, conçu pour un scénario P2P/Internet avec négociation SDP — complexité qui ne sert à rien sur un simple flux PC→tablette en LAN. |
| **Sunshine / Moonlight (protocole ENet + RTP-like)** | `LizardByte/Sunshine`, `moonlight-stream/*` | **GPL-3.0** (les deux, y compris `moonlight-common-c`) | Protocole de streaming bas-latence éprouvé en production (des millions d'installations) | Voir §3 — **Groupe C, écarté pour intégration de code** malgré la pertinence technique. Reste utile comme référence d'architecture (lecture seule du concept, pas du code). |

**Verdict Étape 5.3 :** **msquic (MIT) est le composant recommandé le plus impactant de tout cet audit.** Il résout directement le problème documenté « le port vidéo annoncé n'est pas actif, TCP sert à la fois au contrôle et à la vidéo » en remplaçant le futur canal vidéo dédié par un flux QUIC séparé, sans les obligations copyleft de Sunshine/Moonlight ni la complexité de build de WebRTC.

### 1.5 iPad / iOS

- Pas de projet open source identifié qui fournisse un pipeline `Network.framework → VideoToolbox → Metal` prêt à l'emploi et sous licence permissive, indépendant de Moonlight (GPL-3.0). Notre approche (client natif direct) reste la bonne stratégie.
- `moonlight-ios` (GPL-3.0) est structurellement proche de ce que vise `native/ios/` mais son code ne peut être réutilisé (voir §3). Utile uniquement comme lecture d'architecture (schéma de threads de décodage, gestion des changements de format), jamais comme source copiée.

### 1.6 Android

- Même constat que iOS : `moonlight-android` (GPL-3.0) est la référence technique la plus proche (MediaCodec + Surface), mais écartée pour intégration de code pour la même raison de licence.
- Rien d'autre de pertinent et maintenu identifié sous licence Groupe A/B pour ce périmètre précis.

---

## 2. TABLEAU COMPARATIF (Étape 4)

| Fonction | Notre implémentation | Projet Open Source | Meilleur choix | Raisons |
| --- | --- | --- | --- | --- |
| Virtual Display (IddCx) | Implémentée, non compilée, cohérente avec le sample officiel | MS Indirect Display Sample (MIT) / itsmikethetech (MIT/CC0) | **Notre implémentation**, sample MS en référence croisée | Déjà alignée sur la référence officielle ; aucun gain à remplacer |
| Capture DXGI | Implémentée, non testée sur GPU | — (API native Windows) | **Notre implémentation** | Pas de bibliothèque tierce pertinente ; travail restant = exécution/mesure réelle |
| NVENC | Stub `EncodeTexture()` vide | NVIDIA NvEncoderD3D11 (licence samples NVIDIA) | **Adapter la référence NVIDIA** | Comble directement le trou documenté ; c'est la référence canonique de l'éditeur de l'API elle-même |
| Network / transport vidéo | TCP framé, authentifié, sans canal vidéo dédié ni chiffrement | msquic (MIT) | **Intégrer msquic comme second transport** | Résout latence, multiplexage et chiffrement sans dette de licence |
| Pairing | PIN validé, token de session, pas de device store persistant | Sunshine (GPL-3.0, écarté) | **Notre implémentation**, garder telle quelle | Aucune alternative de licence compatible identifiée ; le mécanisme actuel est suffisant |
| Streaming (couche protocole applicatif) | Framing TCP maison | Sunshine/Moonlight (GPL-3.0, écarté) | **Notre implémentation**, msquic en dessous | Le protocole applicatif reste nôtre ; seul le transport change |
| Décodeur Android | MediaCodec amorcé, résolution figée en dur | moonlight-android (GPL-3.0, écarté) | **Notre implémentation** | Écart de licence ; travail restant = négociation de format et tests réels, pas une dépendance |
| Décodeur iPad | VideoToolbox/Metal amorcés, pas de projet Xcode | moonlight-ios (GPL-3.0, écarté) | **Notre implémentation** | Idem ; le vrai blocage est l'absence de projet Xcode, pas l'algorithme de décodage |
| Discovery | UDP maison (pas du vrai mDNS malgré le nom) | Bonjour/mDNS standard (pas un « projet » à part) | **Remplacer par une vraie lib mDNS/DNS-SD** | Hors périmètre de cet audit OSS spécifique — recommandation séparée : utiliser l'implémentation mDNS native de chaque OS (Bonjour sur Apple, NSD sur Android) plutôt qu'un protocole UDP maison |

---

## 3. CLASSEMENT DES LICENCES (Étape 3)

### Groupe A — intégration généralement simple

| Projet | Licence | Obligations |
| --- | --- | --- |
| Microsoft Indirect Display Driver Sample | MIT | Attribution du copyright dans le code dérivé |
| msquic | MIT | Attribution du copyright |
| Google WebRTC (libwebrtc) | BSD-3-Clause | Attribution, pas d'usage du nom Google pour promotion |
| NVIDIA Video Codec SDK Samples | Licence d'échantillons NVIDIA (permissive) | Vérifier le fichier `LICENSE` exact du dépôt avant copie ; généralement usage/modification/distribution libres, sans garantie |
| itsmikethetech/Virtual-Display-Driver | MIT + CC0/Public Domain | Attribution recommandée par l'auteur lui-même |

### Groupe B — à examiner attentivement

Aucun projet retenu dans cet audit ne tombe dans ce groupe (LGPL/MPL). À revisiter si FFmpeg (LGPL en configuration standard, GPL si compilé avec certains composants comme x264 activé) est envisagé plus tard pour un fallback logiciel — non recommandé ici car NVENC/QuickSync/AMF matériels couvrent déjà le besoin.

### Groupe C — risque important pour SecondScreen (écartés)

| Projet | Licence | Raison de l'exclusion |
| --- | --- | --- |
| Sunshine (LizardByte) | **GPL-3.0** | Copyleft fort : toute intégration de code dans un binaire distribué obligerait à publier SecondScreen entier sous GPL-3.0, incompatible avec une distribution commerciale fermée. À noter aussi : Sunshine a lui-même introduit des composants payants (driver HID sous licence commerciale) — signe que même ce projet OSS a rencontré les limites du modèle pur GPL pour un produit distribué. |
| Moonlight (tous clients : qt, android, ios, embedded, common-c) | **GPL-3.0** | Même raison. Toute la famille Moonlight, y compris la bibliothèque partagée `moonlight-common-c`, est sous GPL-3.0. |

**Décision explicite :** aucun code Sunshine ou Moonlight ne doit être copié, adapté ligne à ligne, ou lié statiquement/dynamiquement dans SecondScreen tant que le produit vise une distribution commerciale à licence fermée. Consultation en lecture seule à titre de référence architecturale uniquement, autorisée.

---

## 4. ARCHITECTURE FINALE RECOMMANDÉE (Étape 6)

```text
SecondScreen
│
├── Windows Host
│   ├── IddCx                    → NOTRE CODE (aligné sur MS Indirect Display Sample, référence uniquement)
│   ├── Virtual Display           → NOTRE CODE
│   ├── DXGI Capture              → NOTRE CODE
│   ├── NVENC                     → NOTRE CODE, complété avec la référence NvEncoderD3D11 (NVIDIA)
│   ├── Network
│   │     ├── SecondScreenTransport (TCP, contrôle + PIN)   → NOTRE CODE, conservé
│   │     └── QuicTransport (msquic, MIT)                    → NOUVEAU, canal vidéo dédié bas-latence
│   └── Pairing                    → NOTRE CODE
│
├── iPadOS Client
│   ├── Network.framework          → NOTRE CODE
│   ├── VideoToolbox                → NOTRE CODE
│   └── Metal                       → NOTRE CODE
│
├── Android Client
│   ├── Network                    → NOTRE CODE
│   ├── MediaCodec                 → NOTRE CODE
│   └── Surface                    → NOTRE CODE
│
└── Shared Protocol                → NOTRE CODE, transport abstrait :

NetworkTransport
       │
       ├── SecondScreenTransport   (TCP, existant, fallback/contrôle)
       │
       └── QuicTransport           (msquic, nouveau, vidéo bas-latence)
```

### Composants restant développés par nous
IddCx, Virtual Display, DXGI Capture, protocole applicatif, pairing/PIN, clients iPad/Android (algorithmes de décodage), discovery.

### Composants à intégrer depuis l'open source
- **msquic (MIT)** comme second transport (`QuicTransport`), derrière l'abstraction `NetworkTransport` déjà esquissée dans la feuille de route.

### Composants utilisés uniquement comme référence (jamais copiés)
- Microsoft Indirect Display Driver Sample (déjà la base conceptuelle de notre driver)
- NVIDIA NvEncoderD3D11.cpp/.h (patron d'implémentation pour combler le stub NVENC)
- Sunshine / Moonlight (lecture d'architecture uniquement, licence incompatible)

### Composants à abandonner
- Rien. Aucune partie de notre code actuel n'est remplacée ; QuickSync/AMF restent des priorités basses déjà correctement désactivées par la factory (comportement à conserver, pas un composant à externaliser).

---

## 5. RISQUES JURIDIQUES / TECHNIQUES

1. **Risque de licence principal évité** : la tentation la plus naturelle (« Sunshine/Moonlight font déjà tout ça ») aurait introduit du GPL-3.0 dans un produit destiné à une distribution commerciale fermée. Décision : ne pas y toucher.
2. **msquic sur Windows** utilise Schannel (TLS natif Windows) par défaut — cohérent avec un déploiement Windows-first, pas de dépendance OpenSSL supplémentaire à gérer sur ce PC.
3. **NVIDIA Video Codec SDK Samples** : vérifier le fichier `LICENSE` précis du dépôt `NVIDIA/video-sdk-samples` avant toute copie de code, même minime — les licences d'échantillons NVIDIA sont généralement permissives mais peuvent contenir des clauses spécifiques (ex. limitation à l'usage avec du matériel NVIDIA). À documenter dans `docs/open-source-licenses.md` dès l'intégration réelle.
4. **Ne pas sur-adopter msquic avant validation physique** : conformément à l'étape 7 de la feuille de route, ne pas remplacer le transport TCP existant tant que le canal QUIC n'a pas été testé en parallèle avec un vrai fallback vers `SecondScreenTransport`.

---

## 6. RECOMMANDATIONS FINALES

**Composant recommandé pour intégration progressive :**
- `msquic` (MIT) comme second transport pour le canal vidéo, derrière l'abstraction `NetworkTransport` déjà planifiée dans la feuille de route. C'est le seul composant de tout l'audit qui apporte un avantage concret ET n'a pas de conflit de licence.

**Composants déconseillés :**
- Tout code Sunshine ou Moonlight (GPL-3.0)
- WebRTC (licence compatible mais coût d'intégration disproportionné pour un usage LAN)
- NvPipe (déprécié par NVIDIA elle-même)

**Ne rien remplacer d'autre pour l'instant.** Le driver IddCx, la capture DXGI, et les décodeurs mobiles sont déjà architecturés correctement ; le travail restant est la compilation et la validation physique documentées dans `implementation-progress.md`, pas une nouvelle dépendance externe.
