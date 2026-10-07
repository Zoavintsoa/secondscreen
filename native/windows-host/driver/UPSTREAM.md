# Windows virtual-display driver integration

The driver layer is based on Microsoft's official IddCx Indirect Display Driver sample.

Official reference:
https://github.com/microsoft/Windows-driver-samples/tree/main/video/IndirectDisplay

The sample is intentionally not copied as an unmodified blob. The integration plan is:

1. Import the current Microsoft IddSampleDriver into this directory.
2. Rename the driver identity to SecondScreen.
3. Keep the official IddCx lifecycle and mode negotiation.
4. Replace the sample SwapChainProcessor frame-discard path with the SecondScreen frame bridge.
5. Keep all networking in the host process.
6. Add a device interface for controlled host/driver lifecycle.
7. Package with a test-signed catalog during development.

The Microsoft documentation explicitly identifies IddCx as the model for remote streaming and virtual displays, and states that the swap-chain provides the desktop image as a DirectX surface.
