# SECOND SCREEN — PROJECT AUDIT

Audit source réalisé le 9 septembre 2026. Les statuts décrivent le code présent, pas une validation sur matériel physique.

## 1. EXISTING COMPONENTS

### Frontend

| Statut | Élément | Constat |
| --- | --- | --- |
| [S] | HostView, ClientOnlyView, Diagnostics, LiveDualScreen | Interface React/Vite fonctionnelle pour la démonstration. Le laboratoire double écran et la surface client rendent une simulation. |
| [~] | `networkStreamService` | Capture navigateur via `getDisplayMedia`, appairage local et changements d'état. Aucun transport vers le binaire natif ou un appareil client. |
| [S] | Écran virtuel et télémetrie dans l'UI | Les deux écrans, le PIN et les données de diagnostic sont initialisés côté navigateur. La source active est `SIMULATION` par défaut. |

### Windows host

| Statut | Élément | Constat |
| --- | --- | --- |
| [~] | Capture DXGI Desktop Duplication | Implémentation D3D11/DXGI présente pour capturer une sortie choisie. Le host démarre par défaut sur l'index `0`, pas sur un écran SecondScreen identifié. |
| [~] | NVENC H.264 | Encodage D3D11/NVENC réellement amorcé, avec chargement dynamique de `nvEncodeAPI64.dll`. Il n'a pas été compilé ni exécuté sur GPU NVIDIA. |
| [S] | Quick Sync et AMF | Les classes testent la présence d'une DLL puis remplissent un paquet sans flux H.264 encodé. Elles ne sont pas des encodeurs fonctionnels. |
| [~] | Serveur WinSock2 | Écoute TCP et découverte UDP présentes. Le flux vidéo est envoyé sur le socket TCP de contrôle; le second canal vidéo déclaré n'est pas ouvert ni utilisé. |
| [ ] | Framing TCP robuste | Le serveur traite chaque `recv` comme un paquet complet. TCP peut fragmenter ou fusionner les messages; il faut un accumulateur, des limites de taille et un parseur séquentiel. |
| [ ] | Authentification réelle | Une requête `PAIR_REQUEST` active le streaming sans comparer le PIN reçu au PIN attendu. |
| [S] | Métriques de bout en bout | Les latences de décodage, rendu, jitter et pertes sont constantes, mais `isMeasured` est envoyé à `1`. |

### Windows virtual display

| Statut | Élément | Constat |
| --- | --- | --- |
| [~] | Projet UMDF/IddCx | Le projet, l'INF, l'EDID et des callbacks IddCx existent. Ils nécessitent une compilation WDK et une validation d'installation. |
| [ ] | Pilote intégré au produit | Le pilote n'est pas une cible CMake et le binaire `SecondScreenHost` ne le pilote pas. Les classes de `virtual_display/` ne sont pas incluses dans le host. |
| [ ] | Chaîne de swapchain vers encodage | Le worker du pilote acquiert puis termine les buffers IddCx, sans transmettre de surface à l'encodeur ou au réseau. |
| [S] | Gestionnaire `virtual_display/` | Il manipule le nom codé en dur `\\\\.\\DISPLAY2` et ne crée pas de moniteur. Il ne peut fonctionner qu'après la création réelle par le pilote IddCx. |

### Android client

| Statut | Élément | Constat |
| --- | --- | --- |
| [~] | Client TCP et découverte UDP | Le protocole de base est implémenté, mais l'appairage ne vérifie pas la réponse et la lecture suppose des en-têtes/payloads intacts. |
| [~] | Décodage MediaCodec et `SurfaceView` | MediaCodec est configuré pour une surface et peut recevoir des NALU H.264. La configuration vidéo reste codée à 1920×1080; aucun projet n'a été construit ou testé sur tablette. |
| [~] | Input reverse | Les événements tactiles sont sérialisés. Le host ne les convertit pas aux coordonnées du moniteur virtuel et accepte des événements non authentifiés. |

### iOS et macOS

| Statut | Élément | Constat |
| --- | --- | --- |
| [~] | Client iOS | Fichiers Network.framework, VideoToolbox et Metal présents, mais aucun projet Xcode, target, Info.plist ou dépendance de build n'est fourni. |
| [~] | Capture/encodage macOS | Source ScreenCaptureKit et encodeur VideoToolbox isolés, sans application hôte ni réseau les reliant. |
| [ ] | Écran virtuel macOS public et livrable | `MacVirtualDisplay.m` déclare des API privées CoreDisplay. Cela n'est pas une solution de production distribuable; DriverKit nécessiterait les autorisations Apple appropriées. |

## 2. BUILD STATUS

| Cible | Statut | Preuve / limite |
| --- | --- | --- |
| Frontend | [ ] Non vérifié | Les dépendances Node ne sont pas installées dans cet environnement, donc `tsc` est indisponible. |
| Tests C++ portables | [ ] Non vérifiés | CMake n'est pas installé dans cet environnement. Les tests inspectent des structures et des flux H.264 simulés, pas le matériel. |
| Windows host | [ ] Non vérifié | Requiert Windows, Visual Studio/SDK et GPU. La cible exclut le pilote et les sources `virtual_display/`. |
| Windows driver | [ ] Non vérifié | Requiert WDK, signature de test et installation sur Windows. |
| Android | [ ] Non vérifié | La structure Gradle est présente, mais le wrapper Gradle est absent et aucun build d'appareil n'a été exécuté. |
| iOS | [ ] Non compilable tel quel | Aucun projet Xcode ni manifest de target n'est présent. |
| macOS | [ ] Non compilable tel quel | Sources isolées; pas de projet d'application ou de target. |

## 3. NETWORK STATUS

Le protocole binaire version 1 est défini de manière cohérente dans le C++ et repris par les clients mobiles (en-tête 24 octets, little-endian). La découverte UDP répond à des requêtes, mais ce n'est pas du mDNS malgré l'intitulé Android.

Le transport n'est pas prêt pour le streaming faible latence : TCP sert à la fois au contrôle et à la vidéo; le port vidéo annoncé n'est pas actif. Il manque le framing fiable, les plafonds de payload, l'authentification du PIN, les identités de session, le chiffrement et une politique de reconnexion/retour de keyframe.

## 4. VIDEO PIPELINE STATUS

Le chemin DXGI → texture D3D11 → NVENC → paquet H.264 est le seul candidat à une validation réelle. Il reste à relier la résolution réelle de la sortie capturée à l'encodeur, à corriger la gestion des ressources/textures, à mesurer les étapes, puis à l'exécuter sur la GTX 1060.

Quick Sync et AMF sont des placeholders. Android et iOS possèdent des décodeurs de départ, mais la gestion complète de configuration H.264, des changements de format, de la réception fragmentée et de la cadence de rendu n'est pas validée.

## 5. VIRTUAL DISPLAY STATUS

Un vrai écran secondaire Windows exige un pilote indirect IddCx/UMDF, pas React ni DXGI seul. Le dépôt contient une ébauche de ce pilote, mais elle n'est ni compilée, ni installée, ni raccordée au host. Le host actuel capture l'écran dont l'index est fourni; il ne crée pas ni ne sélectionne avec certitude un écran virtuel SecondScreen.

Le premier objectif réel est donc Windows → Android : terminer et valider le pilote IddCx sur une machine Windows de test, exposer son moniteur/swapchain au processus de streaming, puis vérifier l'extension du bureau dans les réglages Windows.

## 6. BLOCKERS

1. Aucun environnement Windows/WDK/GPU Android n'est disponible pour compiler, signer, installer et tester le pilote.
2. L'intégration pilote ↔ host est absente; les deux programmes ne partagent ni surface, ni contrat IPC, ni cycle de vie.
3. Le protocole actuel autorise un streaming sans PIN valide et n'est pas robuste sur TCP.
4. Les fallback Quick Sync/AMF et plusieurs métriques annoncées comme réelles ne le sont pas.
5. Les projets Xcode et Gradle nécessaires aux builds Apple/Android ne sont pas complets.

## 7. NEXT IMPLEMENTATION PRIORITY

1. Écrire un plan de validation Windows IddCx fondé sur l'exemple officiel Windows Indirect Display Driver, puis obtenir un build WDK et l'installation du pilote de test.
2. Créer un contrat IPC clair entre le pilote (swapchain IddCx) et le host, afin que seules les images du moniteur virtuel soient encodées.
3. Corriger le host : découverte du moniteur SecondScreen, sélection de résolution réelle, factory d'encodeur et métriques honnêtes (`N/A` tant qu'elles ne sont pas mesurées).
4. Remplacer le framing TCP par un parseur accumulé et une authentification de session; conserver TCP pour le contrôle et introduire un canal vidéo conçu pour la faible latence.
5. Finaliser Android avec négociation de format et tests sur appareil physique; ne commencer iOS/macOS qu'avec des projets de build complets et des API Apple publiques/autorisées.
