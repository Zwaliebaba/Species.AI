---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# Client HTTP stack: the in-box WinHTTP, HTTPS only, with operating-system certificate validation and the system proxy

## Context and Problem Statement

The Species client sends a few HTTPS requests: opt-in telemetry ([ADR-066](ADR-066-telemetry-with-consent.md)) now, and the online services that the design phase specifies later (identity, settlement status and orders). The platform layer offers one HTTPS client for all of them: it refuses every address that is not `https`, validates server certificates through the operating system, follows the system proxy settings, enforces the caller's timeout, embeds no credential, key or token, and runs on its own worker thread so that the main thread never waits. The headless `server` product links the same client interface ([ADR-056](ADR-056-products-configurations-and-feature-flags.md)); the operating system of a dedicated server is a hosting decision of the design phase.

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
* libcurl with the Windows TLS back-end
* The Windows Runtime HTTP client (`Windows.Web.Http.HttpClient`) through C++/WinRT

## Decision Outcome

Proposed option: "The in-box WinHTTP (Windows HTTP Services)". **Default while Proposed:** the platform layer's HTTPS client is built on WinHTTP, because it is part of every supported Windows version, validates certificates through the operating system, can use the system proxy configuration, and adds no dependency to the client. **Confirmed when:** the unit and integration tests of the Confirmation section pass on a CI agent, before telemetry is first switched on in a build that leaves the team; in the baseline the client serves only consented telemetry ([ADR-066](ADR-066-telemetry-with-consent.md)), which is off the path of the first playable (O20).

* **Library:** the client links `winhttp.lib` from the Windows SDK pinned by [ADR-051](ADR-051-windows-toolchain-and-compiler-policy.md) and uses the in-box `winhttp.dll`. Nothing is installed with the game ([ADR-071](ADR-071-install-layout-runtime-files-and-user-data.md)).
* **Scope:** WinHTTP is reached only through the platform layer's `HttpsClient`; no other component calls it. The interface carries a request (address, content type, body, timeout) and returns a status and a body, or one error of the platform layer's error set (not `https`, offline, timeout, certificate rejected, other).
* **HTTPS only:** an address whose scheme is not `https` is refused before any connection is made. Every request is opened as a secure request (`WINHTTP_FLAG_SECURE`). No option that ignores certificate errors (`WINHTTP_OPTION_SECURITY_FLAGS` with an ignore flag) is ever set, in any build flavour.
* **Redirects are not followed:** WinHTTP follows redirects automatically by default, also for a POST, and carries caller-added headers to the new address unchanged. The client therefore disables automatic redirection on every request (`WINHTTP_OPTION_DISABLE_FEATURE` with `WINHTTP_DISABLE_REDIRECTS`), and a redirect status is returned to the caller as the result, never followed.
* **Certificates:** WinHTTP validates the server certificate chain, name and validity dates against the operating system's trust store. It does **not** check revocation unless asked, so the client enables it on every request (`WINHTTP_OPTION_ENABLE_FEATURE` with `WINHTTP_ENABLE_SSL_REVOCATION`). A revoked certificate, or a revocation check that cannot complete, fails the request with "certificate rejected": the client fails closed and never sets `WINHTTP_OPTION_IGNORE_CERT_REVOCATION_OFFLINE`. Species ships no trust store and pins no certificate in the baseline; certificate pinning for a Species service is a design-phase choice with that service.
* **TLS versions:** the client does not set `WINHTTP_OPTION_SECURE_PROTOCOLS`, so the operating-system default applies: on Windows 11 only TLS 1.2 and TLS 1.3 are enabled, and the system's Schannel configuration can restrict that further.
* **Proxy:** the session is opened with `WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY`, which uses the system and per-user proxy settings, including automatic proxy discovery, and handles failover between proxies. Species has no proxy setting of its own.
* **Credentials and cookies:** WinHTTP's default automatic-logon level sends the player's Windows credentials (NTLM and Negotiate) to intranet servers, and its default cookie handling stores and resends cookies within a session. The client sets `WINHTTP_OPTION_AUTOLOGON_POLICY` to `WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH` and disables cookies (`WINHTTP_DISABLE_COOKIES`) on every request, so no credential and no cookie is ever sent that the caller did not supply. WinHTTP shares no cookies, cache or credentials with the player's browser. Credentials for a design-phase service are passed by that service's client per request and never embedded in the binary ([ADR-066](ADR-066-telemetry-with-consent.md)).
* **Threads and timeouts:** WinHTTP calls run on the platform layer's HTTPS worker thread, which completes one future per request; the main thread never waits for a request. `WinHttpSetTimeouts` sets the resolve, connect, send and receive timeouts from the caller's timeout, and the worker checks the total deadline between WinHTTP calls and abandons a request once it has passed (closing the request handle after the current call returns, as WinHTTP requires for a synchronous request). Nothing schedules an asynchronous procedure call on the worker thread, because synchronous WinHTTP is not reentrant.
* **Server:** the dedicated server is a Windows build ([ADR-051](ADR-051-windows-toolchain-and-compiler-policy.md)) and uses the same back-end. A dedicated server on another operating system needs a later ADR, which also chooses its HTTPS back-end behind the same interface; the rules above apply to it unchanged.
* **Not used:** no third-party HTTP or TLS library in the client; no HTTP listener in the client.

### Consequences

* Good, because certificate trust, revocation and TLS follow the operating system and its updates, with no library for Species to service.
* Good, because proxies configured on the player's system work without a Species setting.
* Good, because the client binary gains no dependency and the install directory no file.
* Bad, because WinHTTP is Windows-only, so a non-Windows server needs a second back-end and tests for both.
* Bad, because WinHTTP's error codes must be mapped onto the platform layer's error set, and that mapping needs its own tests.

### Confirmation

* Unit tests: an `http` address is refused with "not `https`"; a redirect response is returned as the result and not followed; a server that never answers gives "timeout" within the caller's timeout; the binary holds no embedded key or token.
* An integration test against local test servers checks that a self-signed, an expired, a wrong-name and a revoked certificate, and a certificate whose revocation list cannot be fetched, are each rejected with "certificate rejected", and that a valid certificate succeeds.
* An integration test on a CI agent with a configured system proxy checks that requests go through the proxy.
* A test checks that no request carries an `Authorization` header or a cookie that the caller did not supply, including against a test server, addressed by machine name, that challenges with NTLM and Negotiate and sets a cookie.
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
* Bad, because `AGENTS.md` R14 excludes third-party code from the executable.

### The Windows Runtime HTTP client

* Good, because it is in-box and has a modern asynchronous interface.
* Bad, because it brings the C++/WinRT projection and the Windows Runtime threading model into the platform layer for one small service.
* Bad, because it is no more portable than WinHTTP.

## More Information

* The standards baseline lists no WinHTTP version; WinHTTP is part of every supported Windows version and of the Windows SDK pinned by [ADR-051](ADR-051-windows-toolchain-and-compiler-policy.md).
* Amendment 2026-10-17 (verification): the WinHTTP behaviours this record relies on were checked against Microsoft Learn. Revocation checking is off by default and is enabled per request; automatic logon defaults to the intranet level and is raised to high per request; cookies are handled automatically by default and are disabled per request; redirects are followed by default (also for a POST, carrying caller-added headers) and are disabled per request; `WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY` uses the system and per-user proxy settings on Windows 8.1 and later; Windows 11 enables only TLS 1.2 and TLS 1.3 for WinHTTP by default; synchronous WinHTTP is not reentrant. Microsoft's game-development guidance for WinHTTP opens sessions with `WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY` as well. Sources: https://learn.microsoft.com/windows/win32/winhttp/winhttp-security-considerations · https://learn.microsoft.com/windows/win32/winhttp/option-flags · https://learn.microsoft.com/windows/win32/api/winhttp/nf-winhttp-winhttpopen · https://learn.microsoft.com/windows/win32/winhttp/authentication-in-winhttp · https://learn.microsoft.com/windows/win32/winhttp/cookie-handling-in-winhttp · https://learn.microsoft.com/windows/win32/winhttp/about-winhttp · https://learn.microsoft.com/gaming/gdk/docs/features/console/networking/web-requests/intro-winhttp
* Related: [ADR-050](ADR-050-client-platform-and-minimum-os.md) (supported OS), [ADR-065](ADR-065-crash-reporting.md) (the rule for any later uploader: explicit opt-in, HTTPS only, no credentials), [ADR-066](ADR-066-telemetry-with-consent.md) (the first consumer), [ADR-071](ADR-071-install-layout-runtime-files-and-user-data.md) (no runtime installer).
* Component: platform layer (`PLT`), whose HTTPS client (PLT-099, PLT-100) this record implements; online services and their endpoints are design-phase decisions (DEC-248, DEC-249).
* **Amendment 2026-10-17 (AGENTS.md, O21).** The dedicated server is a Windows build and uses WinHTTP too; libcurl and every other third-party HTTP or TLS library are excluded by `AGENTS.md` R14. The record stays Proposed: the in-box Windows Runtime HTTP client remains an alternative, and WinHTTP is the default.
