# SecondScreen Transport Architecture

## Production

Production transport is QUIC.

- Reliable bidirectional stream: HELLO, pairing, authentication, configuration, control, input and telemetry.
- QUIC datagrams: video access units.
- TLS 1.3 is provided by QUIC.
- Video loss must not block later video.
- Control remains reliable even when video is lost.

The Windows host targets Microsoft MsQuic. MsQuic officially supports Windows and Linux; its current documentation describes Android as a platform it may work on, but without the same support guarantee as Windows/Linux. We therefore keep a native transport abstraction and will validate the Android MsQuic route before making it the only Android production option.

## Bring-up

The current TCP video server is deliberately isolated:
- discovery UDP: 49151;
- future control: 49152;
- temporary video TCP: 49153.

The temporary TCP path exists only to validate the encoder, decoder and packetization. It must never be mistaken for the production transport.

## Transport abstraction

The native code should expose:
- IControlChannel;
- IVideoChannel;
- ITransportSession.

The display and encoder layers must not know whether bytes travel over TCP, MsQuic or another platform-native QUIC implementation.

## Migration order

1. Keep TCP bring-up stable.
2. Add MsQuic host adapter.
3. Add Android MsQuic native library/JNI adapter.
4. Move control messages to reliable QUIC stream.
5. Move video to QUIC datagrams.
6. Add loss/recovery telemetry.
7. Remove TCP from production packaging.

## Security

Authentication is application-level identity layered on top of QUIC:
- pairing code for first authorization;
- random per-device session credential;
- revocation;
- authenticated input.

The pairing code must never become the session secret.
