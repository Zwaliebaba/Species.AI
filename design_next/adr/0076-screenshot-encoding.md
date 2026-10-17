---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# Screenshot encoding: the in-box Windows Imaging Component PNG encoder; video capture by external tools

## Context and Problem Statement

Players and the team take screenshots, including large poster-size renders assembled from tiles, and CI saves rendered frames for golden-image tests ([ADR-0069](0069-test-framework.md)). The renderer reads frames back through a readback buffer (frame-graph read-back, [ADR-0004](0004-frame-graph-from-the-ordered-pass-table.md)); the image must then be encoded without stalling the frame and written to the player's Pictures folder ([ADR-0071](0071-install-layout-runtime-files-and-user-data.md)). Video capture is needed only for trailers and bug reports, where external capture tools exist; the fixed-step time source ([ADR-0074](0074-platform-clock-with-injectable-time.md)) gives smooth frame-by-frame capture when needed.

Which encoder writes Species screenshots, and does the engine capture video?

## Decision Drivers

* No additional dependency for a format Windows encodes in-box.
* Encoding off the render thread.
* Lossless images for golden-image comparison.

## Considered Options

* The Windows Imaging Component (WIC) PNG encoder
* A current libpng
* A single-header PNG writer
* An engine video recorder

## Decision Outcome

Chosen option: "The Windows Imaging Component (WIC) PNG encoder". This is the default the team proceeds with.

* **Format:** PNG, 8-bit sRGB RGBA for SDR frames; HDR frames are tone-mapped to SDR before encoding.
* **Pipeline:** the renderer copies the frame or tile into a readback buffer, a worker thread maps it after its fence completes and encodes it with WIC, and the file is written as `species_<UTC date and time>.png` under `Pictures\Species\`.
* **Poster renders:** tiles are encoded into one PNG through WIC's frame-encode interface, one tile row at a time, so memory stays bounded.
* **Golden images in CI** use the same encoder, so test images and player screenshots share one path.
* **Video:** no engine video recorder; trailers and reports use external capture tools, with the fixed-step time source for frame-by-frame capture.

### Consequences

* Good, because PNG encoding needs no third-party library on the client.
* Good, because encoding never blocks the render thread.
* Bad, because a non-Windows server or tool build that needs PNG output must add a portable encoder; the server renders nothing, so this affects tools only.

### Confirmation

* A test encodes a known image, decodes it and compares every pixel.
* A performance test takes a screenshot during a busy scene and checks that the frame time stays within the renderer's budget.

## Pros and Cons of the Options

### WIC PNG encoder

* Good, because it is part of Windows and supports large images.
* Neutral, because it is a COM API, wrapped once in the platform layer.

### A current libpng

* Good, because it is portable.
* Bad, because it is a dependency to pin and service for something Windows already provides.

### A single-header PNG writer

* Good, because it is tiny and portable.
* Bad, because compression is weaker and large poster images need extra work.

### An engine video recorder

* Bad, because external capture tools already serve trailers and bug reports better.

## More Information

* The standards baseline lists no WIC version; WIC is part of every supported Windows version and of the Windows SDK pinned by [ADR-0051](0051-windows-toolchain-and-compiler-policy.md).
* Components: renderer (`RND`, readback), platform layer (`PLT`, encoding and file output).
