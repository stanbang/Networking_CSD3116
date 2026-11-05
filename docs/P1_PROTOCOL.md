# Current merged-protocol note

The account layer adds fixed-size AstAccount request and response types from `idl/accounts.idl` on six identity-specific channel pairs. The native signed permissions grant clients write access only to their own account request topic and read access only to their own account response topic. The gameplay wire format is the same. Join reuses its client nonce to prove the live account binding, checked by the server against the identity associated with the reader. HELLO, epoch/challenge, message correlation, retry limits and worker state are described in [the account design](ACCOUNT_FEATURE_DESIGN.md).

The merged implementation now enables native security by default and requires --security-dir with a valid private Linux bundle. Historical pre-security P1 text below describes development history, not permission for plaintext production use. Use current README/INTEGRATION.md for exact verification status; secure GUI acceptance currently fails on this host.

---
# P1 protocol and admission

Purpose: explain the intermediate protocol and its limits. Author: <team>, with Codex assistance. Date: 2026-11-03.

P1 is an **insecure intermediate implementation**. It is suitable only for controlled local testing. Authentication, encryption, signed access permissions, rate limiting and tamper-evident auditing belong to P2. The graphical client currently displays connection state; Asteroids simulation and snapshots arrive in P3.

## Identity and addressing

Six provisioned identity channels allow a sixth client to test the five-player cap. For identity N, exact topic names are `DDSAsteroids_Join_N`, `DDSAsteroids_Input_N`, `DDSAsteroids_Leave_N`, and `DDSAsteroids_Response_N`. P2 must sign permissions limiting each client certificate to its own Join/Input/Leave publications, its own Response subscription, and server Snapshot subscriptions. Only the server may publish Response/Snapshot. Topic separation alone does not authenticate a peer in P1.

Server handlers derive the channel from their local reader, then verify that the session and claimed player id match that channel's admitted slot. Every join response echoes a random 16-byte nonce; clients ignore unrelated responses. Active retries with the same nonce repeat the same assignment. New nonces cannot take over an active identity. Session identifiers come from OpenSSL `RAND_bytes`; they are never printed.

The server retains up to 64 admitted nonces per identity for its process lifetime, preventing a retired join from reopening a slot. At history exhaustion it rejects further new joins for that identity. Restarting the server clears this history. P2's native DDS security adds authenticated middleware replay protection; the bounded application history is an additional admission defence, with this explicit availability limit.

## Lifetimes and threading

The server main thread runs at 60 Hz and expires leases after three seconds without valid input. A single worker owns DDS entities, admission, and received inputs. Slot access uses a checked mutex; no DDS operation or sleep spans that lock. Clients have one DDS worker and a main thread for GLFW or scripted input. Shared client flags and input state are atomic. Inputs carry state rather than position; P3 will consume the latest state once per simulation tick.

Join/Response/Leave use reliable bounded KEEP_ALL history (32 samples). Input uses best-effort KEEP_LAST(1). Each queue permits one instance and has an explicit resource bound. Read conditions and waitsets block at most 50 ms between timer checks. A valid input refreshes the application lease; automatic DDS liveliness is also set to three seconds. Invalid ids, wrong sessions, future tick acknowledgements and stale sequences do not refresh the lease.

Main requests shutdown atomically, joins its worker, then reads summaries or destroys shared state. Graceful clients send a reliable Leave and wait up to one second for acknowledgements before deleting the participant tree. SIGKILL is detected by lease expiry instead. All DDS loans are returned. No per-tick application allocation is used.

## Reproduce

Run in Ubuntu from `/mnt/c/Users/jorda/Desktop/Networking_CSD3116`:

```bash
make -j2
make test
make baseline
./build/release/server --duration 30
# Separate terminals, start server first:
./build/release/client --identity 1 --headless --bot circle --duration 10
./build/release/client --identity 2 --headless --bot idle --duration 10
# Independent acceptance harness:
python3 tests/p1_network.py
```

Flags reject unknown arguments and nonfinite/out-of-range durations. Maximum duration is 86400 seconds. The server also accepts `--ticks N`. Client fault modes `spoof`, `session`, `id-zero`, `id-high`, and `replay` deliberately test application rejection. They do not prove cryptographic rejection. Non-headless mode opens the connection-status view.

The original lecturer IDL is copied to `idl/baseline.idl` and generated separately under `build/baseline-gen`, preserving `make baseline` while v2 evolves. `baseline/` and `third_party/` remain unmodified. New builds treat external headers as system includes; the lecturer's own unused-parameter warnings remain visible in baseline builds. No warning suppression is applied to new application code.
