---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# Screenshot encoding: the in-box Windows Imaging Component PNG encoder; video capture by external tools

## Context and Problem Statement

Players and the team take screenshots, including large poster-size renders assembled from tiles, and CI saves rendered frames for golden-image tests ([ADR-069](ADR-069-test-framework.md)). The renderer reads frames back through a readback buffer (frame-graph read-back, [ADR-004](ADR-004-frame-graph-from-the-ordered-pass-table.md)); the image must then be encoded without stalling the frame and written to the player's Pictures folder ([ADR-071](ADR-071-install-layout-runtime-files-and-user-data.md)). Video capture is needed only for trailers and bug reports, where external capture tools exist; the fixed-step time source ([ADR-074](ADR-074-platform-clock-with-injectable-time.md)) gives smooth frame-by-frame capture when needed.

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

Chosen option: "The Windows Imaging Component (WIC) PNG encoder". This is the default the team proceeds with; the choice of encoder is fixed by `AGENTS.md` R14, which excludes the third-party alternatives, and the record stays Proposed only for the video-capture part.

* **Format:** PNG, 8-bit sRGB RGBA for SDR frames; HDR frames are tone-mapped to SDR before encoding.
* **Pipeline:** the renderer copies the frame or tile into a readback buffer, a worker thread maps it after its fence completes and encodes it with WIC, and the file is written as `species_<UTC date and time>.png` under `Pictures\Species\`.
* **Poster renders:** tiles are encoded into one PNG through WIC's frame-encode interface, one tile row at a time, so memory stays bounded; every pixel's alpha is written fully opaque, so the image shows exactly the colour channels of the frames.
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
* Bad, because it is a dependency to pin and service for something Windows already provides, and `AGENTS.md` R14 excludes it.

### A single-header PNG writer

* Good, because it is tiny and portable.
* Bad, because compression is weaker, large poster images need extra work, and `AGENTS.md` R14 excludes it.

### An engine video recorder

* Bad, because external capture tools already serve trailers and bug reports better.

## More Information

* The standards baseline lists no WIC version; WIC is part of every supported Windows version and of the Windows SDK pinned by [ADR-051](ADR-051-windows-toolchain-and-compiler-policy.md).
* Components: renderer (`RND`, readback), platform layer (`PLT`, encoding and file output).
* **Amendment 2026-10-17 (AGENTS.md, O21).** WIC is the only PNG encoder `AGENTS.md` R14 leaves; libpng and single-header writers are excluded.
* **Amendment 2026-10-17.** Posters are written with opaque alpha (RND-114); the file name gains a numeric suffix when a file of that name exists (PLT-097).
