---
status: accepted
date: 2026-10-19
decision-makers: the owner at gate B (O44, 2026-10-18); recorded by design-phase area A2
---

# MsQuic transport package: `Microsoft.Native.Quic.MsQuic.Schannel` 2.6.3, the fourth allow-listed Microsoft package, used only behind the transport interface

## Context and Problem Statement

R14 builds the executable from the Windows SDK, the MSVC standard library and a closed list of Microsoft packages, and adding to that list is an owner decision recorded as an ADR ([ADR-055](ADR-055-dependencies-three-nuget-packages.md)). At owner gate B the owner chose MsQuic, Microsoft's implementation of QUIC, in its Schannel build, as the transport of all client–server traffic: reliable streams for commands and blobs, QUIC datagrams for state updates, TLS 1.3 (O44); `AGENTS.md` R14 lists the package as its fourth entry. [ADR-088](ADR-088-transport-messages-identity-and-diagnostics.md) states how Species uses QUIC behind its `Transport` interface. This record states the package itself: what it provides, where it sits, its pinned version, the operating systems it needs, its licence and how it is installed. The platform layer has no socket service today (PLT-100), and the dedicated server is a Windows build with a portable core (O41).

Which package, at which version, provides QUIC to Species, which projects may use it, and what does it bring with it?

## Decision Drivers

* One QUIC implementation for the client, its integrated server and the dedicated server, with no protocol code of Species' own (O44, ADR-088).
* A closed, pinned and reviewable dependency surface, restored by NuGet and never committed (R14, `AGENTS.md` §2).
* The network reached through one library only, so that the platform layer stays the only seam to the operating system's files, threads and services, and a port replaces known parts (O41, ADR-083 item 1).
* A licence that allows redistribution, with its notice installed beside the files it covers (R14).
* The operating-system floor of ADR-050 kept, not raised.

## Considered Options

* 1 — `Microsoft.Native.Quic.MsQuic.Schannel` from nuget.org, the build that uses the operating system's Schannel for TLS 1.3, linked only by the transport library
* 2 — `Microsoft.Native.Quic.MsQuic.OpenSSL`, the build that carries its own TLS 1.3 from OpenSSL and runs on Windows 10
* 3 — the in-box MsQuic of Windows (`msquic.sys`)
* 4 — no QUIC package: Species' own transport over UDP secured by Schannel DTLS 1.2 (ADR-088 option B)

## Decision Outcome

Chosen option: "1", decided by the owner (O44). Accepted: the owner took the decision, and nothing remains to measure before it binds; the evidence of the transport built on it is ADR-088's.

* **What it provides.** The package (checked on nuget.org on 2026-10-18) holds, for x64, `msquic.dll` (548,144 bytes at 2.6.3) and its import library `msquic.lib`; the C API header `msquic.h` with the Windows user-mode header `msquic_winuser.h` that it includes; a C++ wrapper header `msquic.hpp` and the private header `msquicp.h`; program databases; the MIT `LICENSE`; and an MSBuild `.targets` file that adds the include folder and the import library to a referencing project and copies `msquic.dll` beside its output. Species uses `msquic.h` through the version-2 API table (`MsQuicOpenVersion` with `QUIC_API_VERSION_2`) and no other header of the package; its x86 and arm64 files are never referenced, because Species builds x64 only (`AGENTS.md` §3).
* **Where it sits.** `QuicTransport`, the remote implementation of ADR-088's `Transport` interface in the messages and transport library (namespace `Species`), is the only code that calls MsQuic, and `QuicTransport.cpp` is the only file that includes `msquic.h`. Because `msquic.h` includes `<windows.h>` and the Winsock headers, `QuicTransport.cpp` includes the one header that owns the Windows macro family first (`AGENTS.md` §4), and no header of the transport library includes `msquic.h`. MsQuic opens and owns the UDP sockets itself, so **the platform layer needs no socket service**: it stays socket-free (PLT-100), and the UPnP port mapping of O45 is an in-box service of the platform layer that opens no socket. The client executable and the headless server executable reference the package so that its `.targets` file copies `msquic.dll` beside them; no other project references it.
* **Pinned version: 2.6.3** (`2.6.3+160440243`, published on nuget.org on 2026-10-13; checked on 2026-10-18 through `https://api.nuget.org/v3-flatcontainer/microsoft.native.quic.msquic.schannel/index.json` and the package's registration entries). It is the newest release of MsQuic's 2.6 line, the newest supported line (released 2026-08-12, supported for at least 18 months, end of support not yet announced; the 2.5 line ends support on 2027-09-01). **The pin departs from the rule of taking the newest stable release that is at least two weeks old**, which on the check date was 2.6.2 (2026-10-01): on 2026-10-14 the publisher marked every release before 2.6.3 and 2.5.12, 2.6.2 and 2.6.1 among them, as deprecated on nuget.org for critical bugs, so the rule would pin a release that its publisher has withdrawn. 2.6.3 is two weeks old on 2026-10-27; no project restores it before the MVP's M4 (MVP-071), and the pin is re-checked then, before its first restore.
* **Operating systems.** The Schannel build needs Schannel's TLS 1.3, which Windows has from Windows 11 and Windows Server 2022 on. The client floor of ADR-050, Windows 11 version 24H2, lies within it, so this record raises no floor. The dedicated server, a Windows build (O41), needs Windows 11 or Windows Server 2022 or later; area D1 sets its floor at or above that (DEC-220). The Schannel build offers no 0-RTT data, which ADR-088 does not use.
* **Licence.** MIT, `Copyright (c) Microsoft Corporation`; the package declares that its licence must be accepted (`requireLicenseAcceptance`), which the owner's decision O44 records. The install directory carries the MIT notice of `LICENSE` in `licences\`, beside the notices of the other allow-listed packages ([ADR-071](ADR-071-install-layout-runtime-files-and-user-data.md), ADR-055).
* **Installation.** `msquic.dll` of the pinned package is installed beside the client executable and beside the headless server executable, never system-wide and never loaded from another folder; the in-box `msquic.sys` of Windows is not used. No other file of the package is installed: the program databases go to the private symbol store with Species' own ([ADR-065](ADR-065-crash-reporting.md)), and the XDP kernel-bypass runtime that the package's `xdp.json` names is neither installed nor enabled.
* **Restore.** Each referencing project lists the package at its exact version in its `packages.config`, restored into `packages\` and never committed (ADR-055, [ADR-054](ADR-054-build-system-msbuild-solution.md)).

### Consequences

* Good, because Species gets streams, datagrams with per-datagram acknowledgement, loss detection and retransmission, congestion control and TLS 1.3 from one maintained, MIT-licensed Microsoft library, and writes no transport or cryptographic protocol (ADR-088).
* Good, because the network is one library behind one interface: the platform layer stays socket-free, and the transport is the only Species code that knows MsQuic.
* Good, because the Schannel build uses the operating system's TLS and certificate store, and carries no TLS library of its own.
* Bad, because `msquic.dll` is a user-mode DLL that Species ships and Windows Update does not service: an MsQuic security fix reaches players only in a Species release, so the pin follows MsQuic's security releases, and every release check re-reads its deprecation state.
* Bad, because the package's `.targets` file acts on every project that references it, so the project-file checker must keep the reference to the transport library and the two executables.
* Neutral, because a dedicated server for another operating system, a later ADR (O41), would use MsQuic's build for that system, whose TLS comes from OpenSSL, and needs its own R14 decision.
* It forecloses: Species' own transport over UDP and Schannel DTLS (ADR-088 option B); the OpenSSL build of MsQuic on Windows, `Microsoft.Native.Quic.MsQuic.OpenSSL`, which is not on R14's list; the in-box `msquic.sys`; any other QUIC or TLS library in the executable; a socket service in the platform layer; and XDP kernel bypass.

### Confirmation

* `Build\CheckProjectFiles.py` rejects a `packages.config` entry of `Microsoft.Native.Quic.MsQuic.Schannel` at any version but the technology baseline's, any project but the messages and transport library and the two executables that references the package, and an include of `msquic.h` in any file but `QuicTransport.cpp` (ADR-055).
* At the MVP's M4 (MVP-071) the client and the headless server, each with the pinned `msquic.dll` beside it, open a QUIC connection on the loopback interface through `QuicTransport` on Windows 11 version 24H2, the floor of ADR-050: `MsQuicOpenVersion` with `QUIC_API_VERSION_2` succeeds, and the peer's datagram support is reported enabled (`QUIC_PARAM_CONN_DATAGRAM_SEND_ENABLED`). The pin is re-checked on nuget.org before that first restore.
* Before a release, the shipping builder checks that the install's licence list names exactly the four packages and the licensed content, and that `msquic.dll` beside each executable is the pinned file.

## Pros and Cons of the Options

### 1 — the Schannel build from nuget.org

* Good, because it is the owner's choice, Microsoft's supported Windows package, and it uses the operating system's TLS.
* Bad, because it needs Windows 11 or Windows Server 2022, and it is a DLL that Species services.

### 2 — the OpenSSL build

* Good, because it runs on Windows 10 and offers 0-RTT.
* Bad, because it carries a third-party TLS library into the executable, which R14 does not allow, and Species needs neither Windows 10 nor 0-RTT (ADR-050).

### 3 — the in-box `msquic.sys`

* Good, because Windows would service it.
* Bad, because it is a kernel-mode driver for Windows' own HTTP and SMB; user-mode applications package `msquic.dll` instead (MsQuic's platform documentation).

### 4 — no QUIC package

* Good, because R14's list would stay at three packages.
* Bad, because Species would write and maintain its own reliability, channels and fragmentation over DTLS 1.2 (ADR-088 option B), which the owner declined at gate B.

## More Information

* Owner decision O44 (gate B, 2026-10-18); `AGENTS.md` R14. Related: [ADR-050](ADR-050-client-platform-and-minimum-os.md), [ADR-055](ADR-055-dependencies-three-nuget-packages.md), [ADR-071](ADR-071-install-layout-runtime-files-and-user-data.md), [ADR-083](ADR-083-server-process-ownership-and-threading.md), [ADR-088](ADR-088-transport-messages-identity-and-diagnostics.md); the [technology baseline](README.md#technology-baseline), item 12.
* Facts and sources, checked on 2026-10-18 (UTC): the package's versions, publication dates, deprecation states and licence expression from nuget.org's registration entries (`https://api.nuget.org/v3/registration5-gz-semver2/microsoft.native.quic.msquic.schannel/index.json`); its contents from the 2.6.3 package itself (`microsoft.native.quic.msquic.schannel.2.6.3.nupkg`: its `.nuspec`, `LICENSE`, `build/native/Microsoft.Native.Quic.MsQuic.schannel.targets` and file list); MsQuic's platform and release documentation at the v2.6.3 tag (https://github.com/microsoft/msquic/blob/v2.6.3/docs/Platforms.md: Schannel's TLS 1.3 needs Windows Server 2022 or Windows 11, no 0-RTT with Schannel, `msquic.sys` in-box for Windows' own features; https://github.com/microsoft/msquic/blob/v2.6.3/docs/Release.md: the supported lines and their dates); TLS 1.3 in Schannel from Windows 11 and Windows Server 2022 (https://learn.microsoft.com/windows/win32/secauthn/protocols-in-tls-ssl--schannel-ssp-) and QUIC's need for Windows 11 or Windows Server 2022 (https://learn.microsoft.com/dotnet/fundamentals/networking/quic/quic-overview).
* ADR-071's install list names the licence notices of "the three allow-listed packages"; this record adds `msquic.dll` beside the executables and the fourth notice, and ADR-071 carries the change when its owner next amends it.
* **Amendment 2026-10-24 (M4 task T7, [ADR-185](ADR-185-the-quic-transport-and-the-servers-certificate.md)).** No project links `msquic.lib`. NeuronCore restores the package for its header and for the build's copy of `msquic.dll` into the output folder, and `QuicTransport` loads the DLL from beside the module that holds it at its first use, never Windows' own copy.
