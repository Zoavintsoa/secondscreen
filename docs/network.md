# SecondScreen — Network Transport Specification

## 1. Network Topology & Port Mapping

```
Host (Windows / macOS)                           Client (Android / iPadOS)
┌───────────────────────┐                        ┌───────────────────────┐
│ Control Server (TCP)  │◄────── Port 9876 ─────►│ Control Client (TCP)  │
│ Discovery Beacon (UDP)│◄────── Port 9877 ─────►│ Discovery Probe (UDP) │
│ Video Stream (TCP/UDP)│─────── Port 9878 ─────►│ Video Receiver (UDP)  │
└───────────────────────┘                        └───────────────────────┘
```

---

## 2. Low-Latency Optimization Strategies
1. **TCP_NODELAY**: Disables Nagle's algorithm on all control and video sockets.
2. **Buffer Sizing**: Socket send/receive buffers tuned to 2 MB to prevent kernel queue jitter.
3. **Microsecond Precision Timestamps**: Every video frame header includes a 64-bit microsecond clock timestamp captured at the exact moment of GPU VRAM texture availability.
4. **Bandwidth Adaptation**: If round-trip latency increases or packet drops occur, the host encoder dynamically lowers the CBR bitrate target.
