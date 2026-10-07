# SecondScreen LAN Protocol v1

## Goals
- LAN-first, no cloud dependency.
- Deterministic framing and reconnect.
- H.264 first; HEVC optional.
- Control and video are separate logical channels.
- Every session is authenticated after pairing.

## Discovery
UDP broadcast/mDNS advertisement:
- service: `_secondscreen._tcp`
- payload: `name, hostId, protocolMajor, protocolMinor, controlPort, capabilities`

## Control framing
Each control message:
- 4 bytes magic: `SSCP`
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

## Video framing
The initial implementation uses a reliable TCP video channel to simplify bring-up. The wire format is deliberately transport-independent so it can move to QUIC/UDP later.

Each access unit:
- 4 bytes magic: `SSVF`
- 1 byte codec (1=H264, 2=HEVC)
- 1 byte flags (bit 0 = keyframe)
- 2 bytes reserved
- 8 bytes presentation timestamp in microseconds, unsigned
- 4 bytes access-unit length
- Annex-B encoded access unit

## Stream configuration
JSON fields:
- codec
- width
- height
- fps
- bitrateKbps
- rotation
- colorSpace

The host may lower bitrate/FPS when the network or encoder cannot sustain the requested mode.

## Pairing
The first connection exchanges a short-lived pairing challenge. The client confirms the challenge using the one-time code shown by the host. A session token is then generated for that device. Tokens are local and revocable.

## Reconnect
Client reconnect sequence:
1. discover host
2. reconnect with stored device id
3. authenticate session token
4. request current stream configuration
5. wait for next keyframe
6. resume rendering

A host must force a keyframe after reconnect.
