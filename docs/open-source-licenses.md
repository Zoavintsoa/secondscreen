# SECOND SCREEN — DÉPENDANCES OPEN SOURCE RETENUES

Ce fichier liste uniquement les dépendances **effectivement retenues** pour intégration, telles qu'identifiées dans `docs/open-source-audit.md`. Il ne liste pas les projets consultés à titre de référence uniquement (voir l'audit pour la liste complète).

À maintenir à jour à chaque intégration réelle de code tiers.

---

## Dépendances déjà vendues dans le dépôt

| Composant | Origine | Chemin dans le dépôt | Licence | Statut |
| --- | --- | --- | --- | --- |
| `nvEncodeAPI.h` | `FFmpeg/nv-codec-headers` (en-têtes NVIDIA redistribués) | `native/windows/vendor/nvidia/nvEncodeAPI.h` | Vérifier l'en-tête de licence exact du fichier dans le dépôt `nv-codec-headers` avant release (généralement MIT pour les en-têtes du projet FFmpeg) | Présent, non encore compilé dans un binaire |

## Dépendances recommandées, non encore intégrées

| Composant | Repository | Version visée | Licence | Usage prévu | Obligations |
| --- | --- | --- | --- | --- | --- |
| **msquic** | `github.com/microsoft/msquic` | Dernière release stable au moment de l'intégration | **MIT** | Second transport réseau (`QuicTransport`), canal vidéo bas-latence, derrière l'abstraction `NetworkTransport` | Conserver la notice de copyright MIT de Microsoft dans la documentation tierce du produit final (ex. écran "licences" ou fichier `THIRD_PARTY_NOTICES`) |

## Références consultées, jamais copiées (aucune obligation de licence — non redistribuées)

Ces projets ont été lus/étudiés pour comparaison architecturale mais **aucun de leur code n'a été copié, adapté ou lié** dans SecondScreen. Listés ici pour traçabilité de l'audit, pas parce qu'ils créent une obligation.

| Projet | Licence | Raison de la consultation |
| --- | --- | --- |
| Microsoft Indirect Display Driver Sample (`Windows-driver-samples`) | MIT | Référence de conception pour notre driver IddCx existant |
| itsmikethetech/Virtual-Display-Driver | MIT + CC0/Public Domain | Référence de packaging/installation |
| NVIDIA Video Codec SDK Samples (`NVIDIA/video-sdk-samples`) | Licence d'échantillons NVIDIA (permissive) | Patron d'implémentation pour `HardwareEncoder::EncodeTexture()` — **si du code de ce dépôt est effectivement adapté à l'avenir, il doit être ajouté au tableau ci-dessus avec attribution explicite dans les commentaires du fichier source concerné** |
| Google WebRTC (libwebrtc) | BSD-3-Clause | Évalué et écarté pour coût d'intégration disproportionné |
| Sunshine (LizardByte) | GPL-3.0 | Évalué et écarté — copyleft incompatible avec distribution commerciale fermée |
| Moonlight (qt/android/ios/embedded/common-c) | GPL-3.0 | Évalué et écarté — même raison |
| NVIDIA/NvPipe | BSD-3-Clause | Évalué et écarté — déprécié par NVIDIA elle-même |

---

## Règle de mise à jour

Avant chaque intégration réelle de code tiers dans le dépôt :

1. Ajouter une ligne dans « Dépendances déjà vendues » ou « Dépendances recommandées, non encore intégrées » → déplacer vers « déjà vendues »
2. Vérifier le fichier `LICENSE` exact du commit/de la release utilisée (pas seulement la page GitHub générale)
3. Copier la notice de copyright complète dans un fichier `THIRD_PARTY_NOTICES.md` à la racine du produit final avant toute distribution
4. Ne jamais lier ou copier de code sous GPL/AGPL tant que SecondScreen vise une distribution commerciale à licence fermée
