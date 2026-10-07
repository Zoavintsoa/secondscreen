# SecondScreen transport

## Production path
- QUIC connection secured by TLS.
- One reliable bidirectional QUIC stream carries SSCP control frames.
- QUIC datagrams carry SSVG video fragments.
- Datagram size is learned from MsQuic after negotiation; it is never hard-coded as the production limit.
- Loss of a video fragment invalidates the affected access unit.
- The client waits for/request a keyframe before resuming decode after a lost access unit.
- TCP/SSVF remains a bring-up compatibility path only.

## Security
The six-digit pairing code is a user confirmation mechanism, not the transport secret. TLS protects the QUIC channel. The host issues a separate random session credential after successful pairing. Host identity must be pinned or explicitly trusted during the first pairing flow. Certificate material must never be derived from the six-digit code.

## Adapter boundary
QUIC_TRANSPORT.h defines the transport-neutral contract. The Windows MsQuic implementation is isolated in MsQuicServer.* so the host core does not depend on MsQuic symbols. The current adapter is SDK-gated and does not report false success when MsQuic is unavailable.

## Platform
MsQuic provides reliable streams and secure unreliable datagrams. Datagram reception must be enabled/negotiated by both peers, and the peer's supported datagram size is learned from the connection. Android remains an adapter boundary until real-device acceptance.
