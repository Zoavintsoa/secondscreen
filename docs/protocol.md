# SecondScreen — Wire Protocol v1 Specification

## 1. Packet Framing & Binary Layout
All SecondScreen packets share a unified binary `PacketHeader` across Windows, macOS, Android, and iOS.

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                     Magic: 0x53325343 ('S2SC')                |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|       Version: 0x0001         | MessageType   |  Flags (Key)  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                        Sequence Number                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
+                    Timestamp (Microseconds)                   +
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                         Payload Size                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                      Payload Bytes ...                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

---

## 2. Message Types & Semantics

| Type Code | Name | Channel | Description |
|---|---|---|---|
| `0x01` | `HELLO` | Control | Initial device handshake & capability advertisement |
| `0x02` | `DISCOVER` | UDP 9877 | Broadcast host discovery request & beacon reply |
| `0x03` | `PAIR_REQUEST` | Control | 6-digit dynamic PIN pairing request |
| `0x04` | `PAIR_RESPONSE` | Control | Session token authentication confirmation |
| `0x05` | `DISPLAY_CONFIG` | Control | Resolution, refresh rate, orientation, and DPI scale |
| `0x06` | `STREAM_START` | Control | Codec selection, bitrate, GOP, and transport mode |
| `0x07` | `STREAM_STOP` | Control | Stream termination |
| `0x08` | `FRAME` | Video | Binary H.264/HEVC NALU video packet |
| `0x09` | `PING` | Control | High-precision latency round-trip measurement |
| `0x0A` | `PONG` | Control | Echo timestamp reply |
| `0x0B` | `TELEMETRY` | Control | Real-time measured metrics (FPS, RTT, loss, bitrate) |
| `0x0C` | `INPUT_EVENT` | Control | Digitizer touch, stylus pressure, and wheel events |
| `0x0D` | `DISCONNECT` | Control | Orderly session shutdown |
