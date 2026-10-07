# SecondScreen LAN Protocol v1

## Design principles

SecondScreen is a native remote-display protocol, not a browser streaming protocol.

- LAN-first and cloud-free.
- Transport-independent wire format.
- QUIC is the production transport.
- H.264 is the mandatory baseline codec; HEVC is optional.
- Control is reliable; video is latency-oriented.
- Every paired session is authenticated.
- A reconnect must recover on a keyframe, never on a stale delta frame.
- Version negotiation is explicit.
- The host is authoritative for display mode and stream configuration.

## Versioning

- Protocol major: 1
- Protocol minor: 1
- Major changes are incompatible.
- Minor changes are backward compatible when unknown fields/messages can be ignored safely.
- Every HELLO/CAPABILITIES exchange includes protocolMajor and protocolMinor.

## Discovery

The preferred advertisement is:

- mDNS/Bonjour service: _secondscreen._tcp
- UDP broadcast is a Windows/Android bring-up fallback.
- Advertisement fields: service, hostId, name, protocolMajor, protocolMinor, controlPort, capabilities.

Discovery never grants access. A discovered host must still authenticate the session.

## Production transport

Production transport is QUIC:

- reliable bidirectional QUIC stream: pairing, authentication, capabilities, stream configuration, control and keyframe requests;
- QUIC datagrams: encoded video access units;
- TLS provided by QUIC protects the transport;
- stale video datagrams may be discarded instead of blocking newer frames;
- a reliable control path remains available even when video packets are lost.

The Windows implementation targets MsQuic. Apple-native implementations may use Network.framework QUIC while keeping the same application protocol. Android will use the same QUIC wire contract.

TCP is retained only as a deterministic bring-up transport until QUIC is integrated.

## Control framing

Each control message:

- 4 bytes magic: SSCP
- 1 byte protocol major
- 1 byte message type
- 2 bytes flags
- 4 bytes payload length, big-endian
- payload: UTF-8 JSON

Message types:

- 0x01 HELLO
- 0x02 PAIR_REQUEST
- 0x03 PAIR_RESPONSE
- 0x04 CAPABILITIES
- 0x05 STREAM_CONFIG
- 0x06 PING
- 0x07 PONG
- 0x08 CLOSE
- 0x09 INPUT
- 0x0A AUTH
- 0x0B KEYFRAME_REQUEST
- 0x0C STREAM_PAUSE
- 0x0D STREAM_RESUME
- 0x0E STATS

## Pairing and authentication

Pairing is intentionally split into two credentials:

1. Human pairing code:
   - six digits;
   - valid for 120 seconds;
   - single use;
   - shown by the host UI.
2. Session credential:
   - 256 bits of OS-generated entropy;
   - stored per device;
   - revocable;
   - never derived from the six-digit code.

The session credential is presented only after the encrypted QUIC channel is established.

Recommended authentication exchange:

1. client sends HELLO with persistent deviceId;
2. host sends pairing challenge when the device is unknown;
3. user confirms the six-digit code;
4. host issues a random session credential;
5. client stores the credential locally;
6. reconnect uses AUTH over QUIC;
7. host rejects unknown/revoked credentials;
8. host sends KEYFRAME_REQUEST after successful authentication.

The host must not accept a pairing code as a long-term password.

## Video transport

Each access unit:

- 4 bytes magic: SSVF
- 1 byte codec (1=H264, 2=HEVC)
- 1 byte flags (bit 0 = keyframe, bit 1 = config)
- 2 bytes reserved
- 8 bytes presentation timestamp in microseconds, unsigned
- 4 bytes access-unit length
- Annex-B encoded access unit

The first packet after stream configuration must carry codec configuration as needed by the decoder. A reconnect must receive a keyframe plus required decoder configuration before delta frames are accepted.

## Stream configuration

JSON fields:

- codec
- width
- height
- fps
- bitrateKbps
- rotation
- colorSpace
- keyframeInterval
- maxLatencyMs

Initial profiles:

| Profile | Resolution | FPS | Target bitrate |
|---|---:|---:|---:|
| Safe | 1280x720 | 30 | 4 Mbps |
| Balanced | 1920x1080 | 60 | 8 Mbps |
| Quality | 2560x1440 | 60 | 16 Mbps |
| Ultra | 3840x2160 | 60 | 30 Mbps |

These are starting profiles, not hard guarantees. The host may lower quality when encoder, thermal, Wi-Fi or packet-loss telemetry requires it.

## Latency model

The target is end-to-end display latency below 50 ms on a strong local network.

The pipeline is measured as:

capture -> encode -> queue -> network -> decode -> render

The host and client report timestamps for each stage. The controller uses these measurements rather than guessing from FPS alone.

## Reconnect

Client sequence:

1. discover host;
2. open QUIC;
3. authenticate stored device credential;
4. receive current STREAM_CONFIG;
5. send KEYFRAME_REQUEST;
6. discard deltas until a valid keyframe/configuration arrives;
7. resume rendering;
8. report recovery latency.

## Input

Touch, stylus and mouse events use INPUT messages with:

- device type;
- action;
- normalized X/Y;
- pressure where available;
- contact identifier;
- timestamp;
- optional button/modifier state.

Input is disabled until the session is authenticated.

## Telemetry

STATS is advisory and never blocks video. Suggested fields:

- decoded FPS;
- presented FPS;
- dropped frames;
- datagrams lost;
- decode queue depth;
- estimated glass-to-glass latency;
- bitrate;
- encode time;
- decode time;
- network RTT;
- jitter.

## Security boundaries

- Discovery is unauthenticated metadata only.
- QUIC provides encrypted transport.
- Pairing establishes device identity.
- Session credentials are revocable.
- Input is accepted only after authentication.
- No cloud account is required.
- No third-party server is required for display streaming.

## Implementation rule

A feature is not considered complete because a class, UI, simulator or placeholder exists. It is complete only after the corresponding native path works on real hardware.
