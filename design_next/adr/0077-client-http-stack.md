---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# Client HTTP stack: the in-box WinHTTP, HTTPS only, with operating-system certificate validation and the system proxy

## Context and Problem Statement

The Species client sends a few HTTPS requests: opt-in telemetry ([ADR-0066](0066-telemetry-with-consent.md)) now, and the online services that the design phase specifies later (identity, settlement status and orders). The platform layer offers one HTTPS client for all of them: it refuses every address that is not `https`, validates server certificates through the operating system, follows the system proxy settings, enforces the caller's timeout, embeds no credential, key or token, and runs on its own worker thread so that the main thread never waits. The headless `server` product links the same client interface ([ADR-0056](0056-editions-build-flavours-and-feature-flags.md)); the operating system of a dedicated server is a hosting decision of the design phase.

Players run the client behind home routers, company proxies and filtering security software. Certificate trust, revocation and TLS versions must follow the operating system's policy and its security updates, without a trust store or TLS library that Species has to service. No component chooses the library that implements the client.

Which HTTP stack implements the platform layer's HTTPS client on Windows?

## Decision Drivers

* HTTPS only, with server certificates validated against the operating system's trust store and policy.
* The system proxy settings apply without a Species proxy setting.
* No redistributable and no TLS library to pin and service for the client; security fixes arrive through Windows servicing.
* Requests run off the main thread, with a caller-given timeout and a small error set.
* No credential is ever sent implicitly, and none is embedded in the binary.
* A second back-end can sit behind the same interface for a dedicated server on another operating system.

## Considered Options

* The in-box WinHTTP (Windows HTTP Services)
* libcurl through vcpkg, with the Windows TLS back-end
* The Windows Runtime HTTP client (`Windows.Web.Http.HttpClient`) through C++/WinRT

## Decision Outcome

Proposed option: "The in-box WinHTTP (Windows HTTP Services)". **Default while Proposed:** the platform layer's HTTPS client is built on WinHTTP, because it is part of every supported Windows version, validates certificates through the operating system, can use the system proxy configuration, and adds no dependency to the client.

* **Library:** the client links `winhttp.lib` from the Windows SDK pinned by [ADR-0051](0051-windows-toolchain-and-compiler-policy.md) and uses the in-box `winhttp.dll`. Nothing is installed with the game ([ADR-0071](0071-install-layout-runtime-files-and-user-data.md)).
* **Scope:** WinHTTP is reached only through the platform layer's `HttpsClient`; no other component calls it. The interface carries a request (address, content type, body, timeout) and returns a status and a body, or one error of the platform layer's error set (not `https`, offline, timeout, certificate rejected, other).
* **HTTPS only:** an address whose scheme is not `https` is refused before any connection is made, and so is a redirect to such an address. Every request is a secure request; no option that ignores certificate errors (unknown authority, wrong name, expired, revoked) is ever set, in any build flavour.
* **Certificates and TLS:** server certificates, revocation checking, TLS versions and cipher suites follow the operating system's policy. Species ships no trust store and pins no certificate in the baseline; certificate pinning for a Species service is a design-phase choice with that service.
* **Proxy:** the session uses the system proxy configuration, including automatic proxy configuration where the system has it. Species has no proxy setting of its own.
* **Credentials:** the client sends no credential implicitly: automatic logon with the player's Windows credentials is off, and no cookie or credential store is kept between requests. Credentials for a design-phase service are passed by that service's client per request and never embedded in the binary ([ADR-0066](0066-telemetry-with-consent.md)).
* **Threads and timeouts:** WinHTTP calls run on the platform layer's HTTPS worker thread, which completes one future per request; the caller's timeout bounds name resolution, connection, sending and receiving together. The main thread never waits for a request.
* **Server:** a `server` build on Windows uses the same back-end. A dedicated server on another operating system adds a second back-end behind the same interface, chosen with the hosting decision (for example libcurl); the rules above apply to it unchanged.
* **Not used:** no third-party HTTP or TLS library in the client; no HTTP listener in the client.

### Consequences

* Good, because certificate trust, revocation and TLS follow the operating system and its updates, with no library for Species to service.
* Good, because proxies configured on the player's system work without a Species setting.
* Good, because the client binary gains no dependency and the install directory no file.
* Bad, because WinHTTP is Windows-only, so a non-Windows server needs a second back-end and tests for both.
* Bad, because WinHTTP's error codes must be mapped onto the platform layer's error set, and that mapping needs its own tests.

### Confirmation

* Unit tests: an `http` address and a redirect to an `http` address are refused with "not `https`"; a server that never answers gives "timeout" within the caller's timeout; the binary holds no embedded key or token.
* An integration test against local test servers checks that a self-signed, an expired and a wrong-name certificate are each rejected with "certificate rejected", and that a valid certificate succeeds.
* An integration test on a CI agent with a configured system proxy checks that requests go through the proxy.
* A test checks that no request carries an `Authorization` header or a cookie that the caller did not supply.
* A profiling test checks that no WinHTTP call runs on the main thread during a match with telemetry enabled.

## Pros and Cons of the Options

### The in-box WinHTTP

* Good, because it is part of Windows, uses the operating system's certificate validation and proxy configuration, and needs no redistributable.
* Neutral, because its C API is wrapped once in the platform layer.
* Bad, because it exists only on Windows.

### libcurl with the Windows TLS back-end

* Good, because the same library could serve a client and a server on any operating system.
* Bad, because it is a dependency to pin, build and service for every security release, and automatic proxy configuration on Windows needs extra code.
* Bad, because its TLS back-end, build options and certificate settings are more choices that can drift from the operating system's policy.

### The Windows Runtime HTTP client

* Good, because it is in-box and has a modern asynchronous interface.
* Bad, because it brings the C++/WinRT projection and the Windows Runtime threading model into the platform layer for one small service.
* Bad, because it is no more portable than WinHTTP.

## More Information

* The standards baseline lists no WinHTTP version; WinHTTP is part of every supported Windows version and of the Windows SDK pinned by [ADR-0051](0051-windows-toolchain-and-compiler-policy.md). The WinHTTP behaviours this record relies on (system and automatic proxy configuration, operating-system certificate validation, disabling automatic logon) were not re-checked against Microsoft's documentation on 2026-10-17 (see [Items not verified](README.md#items-not-verified)); confirm them before the record is accepted.
* Related: [ADR-0050](0050-client-platform-and-minimum-os.md) (supported OS), [ADR-0065](0065-crash-reporting.md) (the rule for any later uploader: explicit opt-in, HTTPS only, no credentials), [ADR-0066](0066-telemetry-with-consent.md) (the first consumer), [ADR-0071](0071-install-layout-runtime-files-and-user-data.md) (no runtime installer).
* Component: platform layer (`PLT`), whose HTTPS client (PLT-099, PLT-100) this record implements; online services and their endpoints are design-phase decisions (DEC-248, DEC-249).
