Repro for Renderdoc crash(es).
- repro_01: Crash happens only on reply as soon as a shader utilizes a resource using SM6.6 `ResourceDescriptorHeap`.
The access is confirmed to be working via Pix. An image of the expected result (visualized as UINT) is provided.
