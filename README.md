# SecondScreen

**SecondScreen** is a cross-platform engineering project for turning an Android tablet or iPad into a low-latency secondary display for Windows and macOS.

## Current repository status

The repository currently contains a **React/Vite engineering console and protocol testbed**. It includes:

- Host dashboard
- Android/iPad client simulation
- Dual-screen / DaVinci Resolve-oriented laboratory
- Diagnostics and compatibility matrix
- Protocol v1 TypeScript definitions
- Browser display-capture prototype
- Native architecture/reference artifacts for Windows, macOS, Android and iOS

### Important distinction

The web application is **not** an operating-system virtual display driver.

The real product requires native components for:

| Platform | Native role | Main technologies |
|---|---|---|
| Windows | Virtual display host | C++, WDK, IddCx, DXGI |
| macOS | Virtual display host | Swift/Objective-C, ScreenCaptureKit, VideoToolbox |
| Android | Display client | Kotlin, MediaCodec, SurfaceView |
| iPadOS | Display client | Swift, VideoToolbox, Metal |

Physical driver installation, GPU capture, hardware encoding/decoding, LAN transport and OS-level display registration still require native projects, platform SDKs, signing/entitlements and hardware validation.

## Web console

Local development:

```bash
npm ci
npm run dev
```

Production build:

```bash
npm run build
```

Type check:

```bash
npm run lint
```

## GitHub Pages

The `main` branch now has an automated GitHub Pages workflow:

`.github/workflows/deploy-pages.yml`

The Vite base path is configured for:

`/secondscreen/`

After GitHub Pages is enabled with **GitHub Actions** as the source, pushes to `main` automatically build and deploy the web console.

## Security

- Do not commit API keys or tokens.
- Real secrets belong in GitHub Actions Secrets or local environment variables.
- The browser pairing PIN shown in the UI is a prototype value; it is not a production authentication mechanism.

## Roadmap

See [docs/ROADMAP.md](docs/ROADMAP.md).

## License

No license has been selected yet.
