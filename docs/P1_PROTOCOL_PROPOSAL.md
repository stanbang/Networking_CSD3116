# Historical draft - superseded by main integration

The user authorized integrating contributor main and beginning P1. The actual merged Ast schema is documented in P1_PROTOCOL.md and INTEGRATION.md; this earlier design draft is preserved for history, not a demand to recreate equivalent working code. Further breaking changes still require approval.

---
# P1 protocol change proposal - awaiting approval

Purpose: make the requested breaking-IDL approval concrete. CSD3116 team with Codex assistance, 4 November 2026.

P0 and security Option A are approved by the user. This document requests separate approval of the breaking v2 IDL change; no actual IDL or baseline source has been changed. Native DDS Security is selected, not implemented yet.

## Compatibility change

Replace the four unversioned types in `idl/messages.idl` with namespaced v2 types. Existing baseline binaries cannot communicate with v2. Preserve the exact original IDL at `idl/v1/messages.idl`, generating baseline bindings separately under `build/gen/baseline/`; keep baseline source and third-party files unchanged. Never silently fall back to insecure v1 when v2 authentication fails.

## Proposed application messages

All messages carry a 16-bit version (2) and unsigned 64-bit sequence number. Positions are compact unsigned 16-bit coordinates; angles are unsigned 16-bit normalized turns. IDs are unsigned bytes, validated as 1..5 before indexing. Tick counters are unsigned 64-bit. Exact generated sizes and CDR overhead must be measured and documented before bandwidth claims.

| Message | Proposed fields beyond version/sequence | Bounds and purpose |
|---|---|---|
| JoinRequest | random client_nonce[16], auth_blob sequence<octet,256> (must be empty for native security) | Correlate concurrent joiners; auth material never grants an unauthenticated slot. DDS handles certificate exchange outside application IDL; nonempty reserved auth_blob is rejected. |
| JoinResponse | echoed client_nonce[16], status, assigned_player_id, random session_id[16], server_tick, lease_ms | Explicit status enum (accepted/full/rejected/version mismatch); rejected IDs cannot index arrays. Response accepted only from verified server after P2. |
| Input | session_id[16], claimed_player_id, tick_ack, key_bitmask | Known bits only; identity derived from verified session, never claimed ID. Sequence/replay and tick plausibility checks before state mutation. |
| Leave | session_id[16], claimed_player_id | Only the session owner can leave; stale session cannot remove a reused slot. |
| Heartbeat | session_id[16], claimed_player_id, tick_ack | Bounded 3-second lease; only validated fresh messages renew it. Never renew on rejected traffic. |
| Snapshot | server_tick, wave, game_over, ships, bullets, asteroids | Bounded sequences of at most 5/128/64. Ship: id, position, angle, lives, score, invulnerability ticks. Bullet: stable pool index, position, owner. Asteroid: stable pool index, position, angle, size enum. Validate lengths, indexes, coordinates and enums on the client. |

P1 uses snapshots for square-demo state while preserving room for P3 gameplay. It does not claim asteroid mechanics before P3. Native DDS Security handles authentication and encryption without adding a custom application crypto envelope. Application session IDs are correlation values, not secrets or substitutes for authentication.

## P1 acceptance plan

1. Keep `make baseline` working with v1 bindings; build v2 server/client separately with zero new-source compiler warnings.
2. Concurrent join requests receive unique assignments addressed by nonce; rejected/version-invalid responses cannot assign a slot.
3. Validate IDs 0, 1, 5, 6 and maximal values; reject unknown keys/statuses, invalid counts/ticks, old sessions and oversized bounded data before indexing.
4. Voluntary leave and killed-client expiry free a slot within 3 seconds plus one bounded tick; stale packets cannot control a replacement player.
5. Enforce a five-player cap and bounded placement attempts. Make test outcomes depend on real state and nonzero failure exits.
6. Run builder/reviewer/tester checks, update play/stop tools to explicitly select v2, then open server/two WSLg clients and present the manual P1 checklist. STOP until APPROVED P1 or FIX.

P1 protocol tests must be labelled pre-security tests until P2 passes. Native plugin presence alone is not authentication, encryption, topic authorization or session-to-publisher binding. The implementation must prove the approved mechanism's identity-binding API/permissions against installed headers and official documentation before claiming P2 security.

## Pending decisions

- Security: A native DDS Security APPROVED by the user. Installed authentication, access-control and crypto plugins were found. Implementation and negative/ciphertext tests remain required.
- Breaking IDL: explicit user approval of the final v2 proposal is required before editing `idl/messages.idl`. This proposal is not an approval request to alter baseline source.
- Actual human retry capture: no completed `.pcap` was found in the known output directory at this check. Offline filter tests are not real network evidence.


## Final proposed field widths for approved native-security design

IDL integer types: `octet`=8-bit unsigned; `unsigned short`=16-bit; `unsigned long`=32-bit; `unsigned long long`=64-bit. These are schema choices, not measured wire-size claims. Every top-level message has version:u16 and seq:u64, with explicit final extensibility in the generated IDL.

- JoinRequest: client_nonce:octet[16], auth_blob:sequence<octet,256> (empty required).
- JoinResponse: client_nonce:octet[16], status:u8 (accepted/full/rejected/version_mismatch), assigned_player_id:u8, session_id:octet[16], server_instance:octet[16], server_tick:u64, lease_ms:u32 (3000). Rejected responses use id0 and cannot allocate or index a slot.
- Input: session_id:octet[16], claimed_player_id:u8, tick_ack:u64, key_bitmask:u16. Known bits are up/down/left/right/fire/restart; P1 uses arrows, P3 interprets rotation/thrust/fire/restart.
- Leave: session_id:octet[16], claimed_player_id:u8.
- Heartbeat: session_id:octet[16], claimed_player_id:u8, tick_ack:u64.
- Ship: player_id:u8, x:u16, y:u16, angle:u16, lives:u8, score:u32, invulnerability_ticks:u16.
- Bullet: pool_id:u16, x:u16, y:u16, owner_id:u8.
- Asteroid: pool_id:u16, x:u16, y:u16, angle:u16, size:u8 (small/medium/large only).
- Snapshot: server_instance:octet[16], server_tick:u64, wave:u32, game_over:boolean, ships:sequence<Ship,5>, bullets:sequence<Bullet,128>, asteroids:sequence<Asteroid,64>. Only present pool entries are transmitted, and duplicate IDs are rejected. Player scores are carried in Ship records.

All message types are in module `AsteroidsV2`. All positions must be in800x600 bounds, all semantic IDs/pool indexes validated before use, and the client validates counts/enums as well as the DDS-generated sequence bounds. P1 square placement additionally respects square dimensions. Array indexes are never inferred as safe just because the wire type is unsigned. Session sequences must not wrap; a new handshake/session is required at exhaustion. Server-instance changes require rejoin rather than accepting stale snapshots.

## Native-security topic/identity boundary

Proposed client identity names are client1 through client6 for testing a sixth refused join; the gameplay cap stays5. JoinRequest/Input/Leave/Heartbeat topic names have the exact certificate identity suffix (for example `AsteroidsV2_Input_client1`). Signed permissions authorize each certificate to publish only its own client channels. Server readers tag messages by their configured channel, not by the claimed player ID. Each authenticated identity may hold at most one leased slot. The server publishes only the response/snapshot channels, and clients are forbidden from publishing those channels by signed access-control permissions. JoinResponse remains nonce-addressed; Snapshot identifies the current server instance.

Certificate subjects, signed permissions, governance, encryption and actual identity-to-channel enforcement must be checked with the installed plugin APIs/configuration and negative tests in P2. Topic names or self-asserted identity strings alone provide no authentication. P1's pre-security traffic remains explicitly untrusted until P2 passes. Unauthenticated participants must not allocate production slots once security is enabled.

## Rubric recheck

Team Lab pages14/16/18/19/20 were reread after the user's security approval. The PDF requires the actual animated multiplayer game, mandated technologies, authentication and encrypted DDS, optimization/threading application, commented ZIP, presentation PDF, team-narrated MP4 and AI declarations. Grading is program40%, slides20%, video20%, peer evaluation20%; the program's optimization/security component is40% of the program mark. Slide clarity/structure/technical-content weights are25/25/50; video has four25% categories. No grade or all-requirements-met claim is made. `PDF_REQUIREMENTS.md` remains the requirement-to-evidence checklist, and every unmet row must stay visible through final submission.

Approval requested: replace `idl/messages.idl` with the specified v2 types and preserve v1 bindings separately. P0 approval and security approval do not themselves authorize this breaking edit. After IDL approval, builder implements P1, reviewer audits, tester verifies, then the user gets the required desktop/manual P1 approval gate.

## Independent review clarifications (part of approval scope)

- JoinResponse is published on each exact per-identity response topic, for example AsteroidsV2_JoinResponse_client1, rather than a shared response channel. Each client may read only its own response topic and the common AsteroidsV2_Snapshot. No wildcard client-publish grants or payload-supplied partitions may widen this boundary. A certificate identity has at most one active session/slot, even with duplicate participants using that certificate; a new join cannot overwrite it. Client6 is DDS-authorized so its sixth-player test reaches a genuine application FULL rejection.
- Input/Heartbeat/Leave share one increasing client sequence counter per session. A 64-message replay window records duplicates; older valid reordered Input never replaces a newer applied Input. Only a new highest sequence renews the lease or authorizes Leave. JoinRequest retries retain nonce/sequence; a new process uses a fresh nonce and cannot replace an existing active session. Server responses and snapshots use separate per-topic counters within server_instance. Snapshot freshness resets only after a fresh nonce-correlated JoinResponse binds that server_instance to the client session.
- Accepted JoinResponse requires id1..5 and fresh nonzero session/server-instance values; other statuses require id0 and never allocate/index a slot. Native-security auth_blob must have length0 or be rejected. Tick acknowledgement must not exceed current tick and must fit the documented lag policy, with explicit join-time handling.
- Positions are integer pixels in0..799/0..599; angle is an unsigned fraction of one turn over0..65535; lives are remaining lives, score is cumulative points, invulnerability is remaining60Hz ticks. Asteroid size enum is small=0/medium=1/large=2. Ship IDs1..5, bullet pool IDs0..127, asteroid pool IDs0..63, no duplicate IDs per sequence. Key bits0..5 are up/down/left/right/fire/restart; higher bits reject. No network floats are introduced.
- Planned application CDR payload limit is8KiB, to be verified against generated serialization and enforced before any measured bandwidth claim. At30Hz the planned ceiling is240KiB/s per client before DDS/security/UDP overhead; this is a design ceiling, not measured traffic. Fragmentation and actual overhead must be measured and documented. Small-P0 capture truncation settings must be revised for full secure-game payload evidence.
- Native rejection may happen before the application sees a JoinRequest. P2 must verify a supported middleware logging/callback path for wrong/no-cert evidence and honest authentication-failure counters. Do not invent an identity for unauthenticated peers or claim reader-only application auditing sees every DDS rejection.
- Packet capture evidence must establish unreadable game payloads; native DDS/IP/certificate metadata can remain visible. Session/nonce/epoch values are correlation and replay contexts, never bearer authorization. No unsupported claim that every identifier or metadata byte is hidden is permitted.
