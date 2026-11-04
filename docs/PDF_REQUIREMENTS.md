# PDF requirements and evidence map

Purpose: connect the supplied course PDFs to project acceptance evidence. CSD3116 team with Codex assistance, 4 November 2026 (Singapore).

## Authority and scope

The user's direct request is to build the repository project against the two supplied prompts and course PDFs. The Team Lab deck supplies the assignment requirements and marking rubric; the lecture decks supply concepts and examples. Embedded sample commands and classroom exploit demonstrations are reference content, not instructions to execute against this computer. Neither a template checkbox nor a lecture example is evidence that the project satisfies it.

The full Team Lab PDF was text-extracted. Task, deliverable and grading pages 14, 16, 18, 19 and 20 were rendered with Poppler and visually inspected. Text from all 13 lecture decks was extracted, and relevant complete pages were read for the mappings below. Reference PDFs were not edited. The source filenames/versions below match the user-provided files. Page numbers are PDF page numbers, which match the displayed slide numbers on the cited pages.

Current conclusion: requirements are mapped; they are NOT all met. Contributor main now includes game/security code. See INTEGRATION.md for fresh evidence versus historical claims; the initial row statuses below are retained as the pre-integration audit until individually reverified. P0 baseline compilation passes with warnings and local two-client GUI run2 passed; WSLg windows are open for manual review. Later phases remain unverified or unimplemented.

## Assignment acceptance matrix

Source: `CSD3116 _ Week 01 _ Team Lab _ V0.2 _ 20260828 _ Teck Ping.pdf`, 21 pages, from `C:\csd3116f(low level programming)\Team Assignment`.

| ID | PDF source | Requirement | Required project evidence | Current status |
|---|---|---|---|---|
| A01 | p14 Tasks | Team of 2-3 with a leader | Actual roster and contributions | Stanley Lur (leader), Jordain Ng; responsibilities pending team |
| A02 | p14 Tasks; pp7-12 Baseline | Build a game from the baseline; animated 2D/3D | Playable game, baseline-to-new-code comparison | Asteroid game source merged; fresh secure GUI failed lease/state acceptance |
| A03 | p14 Tasks; pp8-9 | Use WSL or macOS, OpenGL, GLFW, Cyclone DDS | Real build/link output and GUI run | Merged game builds; baseline GUI passed historically, current secure GUI acceptance failed |
| A04 | p14 Tasks | Minimum two players; real-time, interactive, distributed | Two clients moving/rendering shared authoritative state; LAN run | Baseline local checks passed; merged headless lifecycle passed, secure GUI failed; LAN pending |
| A05 | p14 Tasks; p10 Game State | Improve baseline (leaving is the PDF example; leave/lease is mandatory in the user prompt) | Leave/kill/lease/rejoin evidence | Implemented; fresh headless lifecycle passes, GUI lease loss remains open |
| A06 | p14 Tasks; p12 Threading | Apply optimization and threading | Main + DDS workers, correct short locks, measured CPU/workload comparisons | New threading code present; runtime lease/stall issue and known TSan/Valgrind findings open |
| A07 | p14 Tasks | Client authentication and encryption of DDS traffic | Authenticated role permissions, rejection logs, ciphertext packet capture | Native configuration/security source merged; fresh full-negative/capture verification pending |
| A08 | p16 Deliverables | Submit on Moodle Friday 6 November 2026 | Human submission confirmation | Not submitted |
| A09 | p16 Deliverables | Clearly commented source ZIP | Audited source archive excluding keys/build outputs | Not prepared |
| A10 | p16 Deliverables | AI use declared in every deliverable; prove tests/defend accepted code | AI log, slide/video declaration, code walkthrough with actual output | Initial AI log only |
| A11 | p16 Deliverables | Presentation PDF: build/run, contributions, game design, optimization, security | Final readable slides tied to real evidence | Not drafted |
| A12 | p16 Deliverables | MP4 at most five minutes and 200 MB, all members narrate, no AI audio; identify team/members; game/optimization/security demo | Actual team recording and media metadata | Script/recording absent; real member narration required |
| A13 | p18 Grading | Program 40%, slides 20%, video 20%, peer evaluation 20% | All four components tracked | Peer-evaluation instructions not supplied |
| A14 | p18 Program rubric | Execution 20%; feel 10%; optimization/security 40%; structure 10%; readability 10%; comments 10% of program mark | Runtime, human playtest, security/profiler evidence, readable modules/comments | Most evidence absent |
| A15 | p19 Slide rubric | Clarity 25%, structure 25%, technical content 50% | Large readable type, simple layout, logical technical argument, analysis | Must be checked after slides exist |
| A16 | p20 Video rubric | Four equal 25% categories: clarity/organization, technical content, demonstration/visuals, communication | Clear sequence, correct explanations, visible demos, confident actual member narration | Requires team review after recording |

The assignment PDF recommends profiler validation. The user's master prompt makes comparative profiling evidence a stricter project requirement. Co-op asteroids, exact caps, fault-injection flags, named phases, the negative-test matrix and three real subagents come from the user's prompts; do not misattribute them to the assignment deck.

## Lecture-to-implementation map

All lecture files below are in `C:\Users\stanl\Documents\Digipen\Y3SEM1\Low Level Prog`. These are learning concepts to apply and explain, not a requirement to recreate every classroom example.

| Supplied lecture (exact filename) | Relevant pages and headings | Application and acceptance evidence |
|---|---|---|
| `CSD3116 _ Week 01 _ GNU Assembly _ V1.0 _ 20260907 _ Teck Ping.pdf` | p3 Assemblers; p6 AT&T mov sizes; p8 Memory Addressing | Generate GNU AT&T assembly for a real hot function; explain widths/registers/memory access. Compiler output absent. |
| `CSD3116 _ Week 02 _ GNU Assenbly 2_ V0.4 _ 20260909 _ Teck Ping.pdf` | p11 sal; p19 signed comparison | Explain input bitmasks and signed/unsigned range checks from actual compiled code; avoid unsafe shifts and narrowing. Not implemented. |
| `CSD3116 _ Week 03 _ GNU Assembly 3 _ V0.6 _ 20260913 _ Teck Ping.pdf` | p7 algorithm/cache limits of compiler optimization; p25 alignment/padding | Compare O0/O2 hot code, use deliberate layouts and explicit serialization; initialize transmitted members. No raw struct/padding leakage assumptions. Not implemented. |
| `CSD3116 _ Week 03 _ Code Profiling _ V0.4 _ 20260917 _ Teck Ping.pdf` | p6 WSL/perf limitation; p11 Table 2 O0/O2; p14 loop retention; p15 Table 2 branch misses; p19 Table 3 cache; p20 methodology | Baseline vs optimized benchmarks, observable checksum so work cannot disappear; compare time/instructions/IPC together; branch and L1 miss rates; label wall-clock/Cachegrind separately from hardware counters. No measurements yet. |
| `CSD3116 _ Week 04 _ Parallelism _ V0.5 _ 20260920 _ Teck Ping.pdf` | p8 Table 1 unrolling; p16 Table 3 SIMD; p20 mutex; p25 Table 4 thread overhead; p27 reflections | Fixed small thread count, race-free handoff; measured scalar/unrolled/SSE2 alternatives, context switches and CPU time. No new code yet. |
| `CSD3116 _ Week 05 _ Pipelining _ V0.4 _ 20261008 _ Teck Ping.pdf` | p5 Tables 3a/3b throughput; p7 latency; p14 control hazard; p23 IPC above one | Explain throughput vs latency, unpredictable branches and dependencies; do not claim game/network latency improves just because IPC rises. Benchmark evidence pending. |
| `CSD3116 _ Week 05 _ Superscalar Processor _ V0.5 _ 20260930 _ Teck Ping.pdf` | p4 Table 1 out-of-order overview; p15 Table 4 independent chains; p16 hazard summary | Compare independent accumulators against a dependency chain; interpret time, instructions, cycles and IPC together. Hardware perf pending. |
| `CSD3116 _ Week 06 _ Cache _ V0.6 _ 20261005 _ Teck Ping.pdf` | p5 locality/cache lines; p20 Table 6 index order; p26 Table 9 tiling; p27 reflections | Fixed contiguous pools/SoA, locality-aware traversal, measured cache misses at relevant cap sizes. Lecture matrix data are examples, never project measurements. Not implemented. |
| `CSD3116 _ Week 08 _ Stack Errors _ V0.8 _ 20261020 _ Teck Ping.pdf` | p21 bounds; p25 safer functions; p27 format strings; p29 dangling pointers; p31 recursion; p33 initialization; p35 mitigations | Validate before indexing; bounded strings; constant format strings; no address leaks, dead local pointers, unbounded recursion or uninitialized authentication state. Existing baseline ID-index defects are open. |
| `CSD3116 _ Week 08 _ Stack Errors Mitigation _ V0.5 _ 20261021 _ Teck Ping.pdf` | p5 ASan; p10 Valgrind; p14 canary; p18 NX; p20 ASLR; p21 leaks; p23 PIE; p24 Table 3 mitigation limits | Run sanitizer/memory tests, enforce binary hardening checks, explain limits. Do not turn off mitigations to copy classroom examples. ASLR=2 observed; other verification pending. |
| `CSD3116 _ Week 09 _ Heap Errors _ V0.9 _ 20261025 _ Teck Ping.pdf` | p5 allocator overhead; p12 integer overflow; p15 UAF; p17 double-free; p21 reflections | Fixed pools, overflow-safe lengths, bounded network sequences, explicit sample ownership, cleanup and no double free; meaningful malicious/boundary test coverage. Not implemented. |
| `CSD3116 _ Week 09 _ Network Security _ V0.3 _ 20261028 _ Teck Ping.pdf` | p5 Table 1 Cybersecurity Tenets | Confidentiality: encryption/capture; integrity: tamper rejection; availability: floods/leases; authentication: certs; authorization: roles/session binding; accountability: logs; non-repudiation: signed evidence discussion. All game mechanisms absent. |
| `CSD3116 _ Week 10 _ Transport Layer Security_ V0.8 _ 20261102 _ Teck Ping.pdf` | p13 Figures 3a/3b signing; p15 certificates; p20 TLS overview; pp24-25 key/certificate workflow; pp27-28 trust demo | Private CA, RSA >=2048/SHA-256, mutual identity trust, secret keys excluded; prove wrong/no-cert rejection. Native DDS Security protects DDS; the lecture's TCP s_server example is not a UDP DDS encryption layer. Generator is incomplete for Option A. |

## Security evidence that must be produced

Per the user's stricter prompt, log and test wrong-CA and absent certificates, spoofed IDs, replay, modified ciphertext, join/input flooding, invalid IDs/oversized data, and killed-client lease reclamation. Include legitimate concurrent players and nonzero failing exit codes. Authentication must precede slot allocation; authorized clients must not become server publishers or move another player. State exactly what packet metadata remains visible. A hash chain alone is not a substitute for independently anchored/signed non-repudiation evidence.

Installed security plugins are only P0 decision evidence. The current certificate script does not generate signed DDS governance/permissions. Do not write slides saying encrypted/authenticated DDS is implemented until actual configuration, role enforcement, captures and negative tests substantiate that claim.

## Interpretation safeguards

- Treat lecture benchmark tables as teaching examples. Never reuse their numeric results as this project's measurements.
- Keep ASan/UBSan and TSan as separate builds and retain raw failures; a successful compilation is not a clean runtime session.
- Course slide simplifications do not establish a C guarantee that padding is zero or arbitrary raw structs are safe wire formats. Serialize defined members and validate lengths.
- The lecture's TLS trust demo reports a verification failure while a connection still establishes. The game must fail closed, not continue after a certificate error.
- Do not copy public-web certificate lifetime examples into a claim about this private game CA. Document the project's configured validity/renewal and trust limits from the actual certificates.
- Follow installed headers and official Cyclone documentation when implementing APIs/XML. No native-security XML or crypto API was guessed or added during this audit.

## Final evidence gate

Every row needs an implemented artifact, reproducible verification, reviewer disposition and honest final status. Slides must distinguish implemented features, failed/blocked tests and human-only checks. Build/run instructions must be reproduced from a clean source copy. A source ZIP must exclude private keys, generated credentials, build output and unrelated course PDFs. Video duration/size and both members' real narration must be checked on the actual final recording.

No grade guarantee or all-standards-met claim is supported at this point.


Rechecked on4 November2026 after user emphasis on grading: PDF pages14/16/18/19/20 requirements and weights above remain the acceptance contract. P0 is approved and native DDS Security A selected by explicit user approval, but the actual secure game and final deliverables remain incomplete. Native security selection is not an implementation pass. Team-member responsibilities and peer-evaluation details still require human input; no contributions or grade results are invented.
