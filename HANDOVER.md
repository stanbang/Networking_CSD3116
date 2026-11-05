# Teammate handover: account game and offline source ZIP

Primary runtime: C:/Users/stanl/Documents/GitHub/Networking_CSD3116.
Full development worktree: C:/Users/stanl/Documents/GitHub/Networking_CSD3116_accounts_c50af1e.
Evidence/backup/ZIP: sibling Networking_CSD3116_integration_20261105_c50af1e.

Ubuntu24.04 x86_64 WSL2/WSLg with GCC C development toolchain: cd into runtime and run bash run.sh. The launcher builds offline, provisions local private credentials and starts server plus two windows. Tab changes login field, Enter signs in, Shift+Enter registers, F2 toggles Remember Me. Use two different accounts. Arrows steer/thrust, Space fires, R restarts ordinary game-over. B shows leaderboard, L logs out, F3 forgets the remembered token. Ctrl+C in the launcher terminal stops its processes.

Stanley integration preserves same-account/certificate ship state for30seconds after the last input. After3seconds without input the server pauses and requires reauthentication; after30seconds absence the whole round ends. Returning requires the same account and participant identity. Rejoining another account on the reserved identity is intentionally denied. Protocol4 changes the snapshot schema; old clients cannot mix with the final game.

Jordain contribution retained: password login, SQLite store and worker, certificate-bound admission, authoritative best scores/leaderboard, Remember Me token rotation/revocation, login UI, participant export and capture tools in the full worktree. Stanley contribution: existing secure simulation, lifecycle integration,30second reservation/round end, offline library/header packaging and final verification. Team should adjust individual responsibility claims to actual authorship before submission.

No further security or IDL approval gates are pending under the user's assume-approved instruction. Final runtime tests passed as recorded below. Human acceptance, external packet capture and two-machine evidence remain outstanding; no clean sanitizer/performance claim is justified. Slides/video/peer evaluation remain separate team work. Full development docs/tests are kept outside the runtime ZIP. Do not zip the repository parent or any private Linux state directory.

Packet capture (human only, no agent sudo): in the full development worktree run sudo bash tools/capture_demo.sh --secure 45 secure-final.pcap lo while bash run.sh is actively playing from the runtime in another terminal. The helper saves under the invoking user's private Linux state capture directory and prints the path. Tell the agent that path so it can inspect without sudo. Zero packets is no encryption evidence. Default loopback config uses DDS domain42; native packet ports may be ephemeral, so do not revert to the old7400-7600 filter.

Git: base remote collaborator main c50af1e; these integration/package changes are local and have not been committed/pushed. Original root .git and all earlier files are preserved. Never force-push or silently discard either worktree. SDK is278MiB uncompressed and ZIP about90MiB; do not blindly git-add binary dependencies. The source ZIP, not a GitHub checkout, is the tested offline distribution.

# Final runtime integration,2026-11-05

The selected account/password/SQLite implementation is integrated with the30second crash/rejoin reservation. The verified runtime is now in Networking_CSD3116; every previous root file except preserved .git was moved into root-before-account-promotion in this archive. Full development source, tests and docs remain in Networking_CSD3116_accounts_c50af1e. The tested ZIP contains runtime source, launch/build/provision scripts, generated DDS source, public dependency headers/libraries, required license records and RUN_OFFLINE.md. No private keys, profiles, databases, captures, logs or Git history are in the ZIP.

Ubuntu24.04 x86_64 under WSL2/WSLg with an existing GCC C toolchain is required. Run bash run.sh from the extracted DDS_Asteroids directory. No packages or downloads are invoked. Build-only: bash tools/build_portable.sh.

Actual final outputs (offline-final-verification.txt, exit0):
PASS bundled Python modules sqlite3/ssl/ctypes/fcntl/json
PASS fresh ZIP extraction: offline compile, fresh certificate provisioning, authenticated two-client game and exit0
PASS fresh ZIP bundled software OpenGL: two visible GLFW windows, registration, authenticated joins, keyboard input, screenshot, exit0
PASS offline extracted two-minute two-client soak: exit0 joins2 timeouts0 invalid0

The fresh-extraction test blocked apt,apt-get,sudo,make,pkg-config,idlc. New screenshot gui-offline.png inspected once: both players connected, separate player1/player2 HUD, asteroids, ships, firing and distinct account names.

Native account regression and hardening: account-final-regression.txt, exit0. PIE/NX/RELRO/BIND_NOW/canary references for both binaries; protocol/sim/event/account-store unit checks pass. Certificate-only admission, forged account session, unknown account and rapid retry rejected. Server-earned score420 persisted. Leaderboard, remembered-token rotation/replay rejection/certificate binding/logout/forget all passed.

Reconnect: account-reconnect-test-02.txt exit0. Same-account/certificate crash reclaim kept player1. Score260->660, lives3, cumulative distance166663->306794. On second crash, entire round/server/surviving client ended with exit0 after31.981seconds including final display. Protocol version4 prevents silent compatibility with older game messages. Countdown boundary30 accepted/31 rejected (countdown-boundary-test.txt exit0).

Known limitations remain: external packet capture missing (previous human capture had0packets), no two-machine run, no controlled idleCPU before/after or full before/after performance measurements. Historical TSan/Valgrind dependency findings remain open, not rerun or cleared. Leave ACK warning fallback remains previously approved. During the terminal round-end interval, already in-flight inputs can produce session-drop audit records. WSLg desktop human acceptance has not been substituted by Xvfb automation. Slides/video/peer evaluation remain team deliverables. These are not claims of full rubric completion.

No new commit or push was performed. The final ZIP is local, not a published clean Git revision.

## Windows double-click launcher fix
The reported failure was opening run.sh directly from Windows File Explorer. The working-folder offline build and full run.sh headless session both exited0. Added RunGame.cmd, which explicitly invokes the default Ubuntu WSL distribution in the script directory and pauses after errors. Added visible server/client compilation progress. No C, IDL, account, security or SDK library changes.

Actual Windows entry-point test: RunGame.cmd --headless --duration8, WINDOWS_LAUNCH_EXIT=0:
Starting DDS Asteroids in WSL. The first compilation can take about a minute.
[1/2] Compiling server...
[2/2] Compiling client...
Offline source build complete. No packages installed.
Local session started. Logs: build/local-session.DJjRorpS server=9485 client1=9500 client2=9527

Updated distribution: DDS_Asteroids_offline_source_windows_launcher.zip,94314441bytes,1102 unique files, SHA2563060b8f082fcf858cbcec252e69e8374c898bfebe918700825eb73a926451484. Every packaged file verified against current runtime. No private keys/databases/profiles/captures included. The old ZIP is retained as historical evidence; distribute the Windows-launcher ZIP. Windows usage: extract the entire ZIP and double-click RunGame.cmd, not run.sh. Ubuntu terminal usage remains bash run.sh. Test mode above is headless; the earlier graphical registration/input evidence remains in offline-final-verification.txt. External packet-capture evidence remains pending.

## Password minimum and clearer wording,2026-11-05
User requested minimum6letters and simpler wording. Passwords now accept6-128 printable characters with no uppercase/number/symbol requirement. Both network/client validation and the server store enforce the same lower bound. Existing password hashes, accounts, tokens, database schema, TLS/DDS security and wire format unchanged.

Changed account form to Sign in or create an account, Create account and Stay signed in. Removed internal jargon about certificates, authentication and password bytes from the login form. Field-specific username/password format errors and plain generic sign-in failure messages preserve account-enumeration protection.

Actual outputs:
PASSWORD_TEST_EXIT=0
PASS password minimum: five rejected, six/seven letters accepted
PASS six-letter password registration/login; five-letter password rejected
PASS existing store score/token/binding/path safety regressions
PASSWORD_BUILD_EXIT=0 (bundled offline build, no packages installed)
PASSWORD_GUI_EXIT=0
PASS GUI: five-letter password blocked; adding sixth letter registers player1; six-letter player2 registration accepted
PASS both graphical clients joined; password text remains masked; screenshots saved for visual inspection

password-six-validation.png and password-six-connected.png inspected once: clear, fitting form text, five masked password characters and format error, second player's six-character hint; final screenshot both authenticated players in game. No full-suite/soak or sanitizer rerun. Distribute DDS_Asteroids_offline_source_password6.zip; it retains Windows and reconnect launchers. Restart with RunGame.cmd to load the change. External packet capture remains pending.

## Multiple accounts per player,2026-11-05
User explicitly approved allowing any number of registered accounts per player window. Removed the per-identity unique constraint and global six-account capacity trigger. Usernames remain globally unique. Accounts/tokens stay certificate-identity bound; one active client per participant identity and the gameplay player cap remain unchanged. Passwords still allow six digits with no composition requirement. Registration errors now suggest a different username, not an account-per-window limit.

Schema version2 is created for new stores. Existing version1 stores are validated under the exclusive lock and upgraded transactionally: create/copy the accounts table preserving all IDs/columns, remove old restrictions, validate integrity/foreign keys, commit or roll back, restore foreign-key enforcement. Password hashing, existing password values, score provenance and tokens are preserved.

MULTI_STORE_EXIT=0 (multi-account-store-test.txt):
PASS legacy migration preserves every account/score/token field byte-for-byte
PASS twelve accounts on one player beyond old global cap; login/reopen/unique usernames and certificate binding
PASS existing six-character, score persistence, verifier material, token rotation/expiry/revocation, unsafe-path and exclusive-lock checks
MULTI_BUILD_EXIT=0 (multi-account-build.txt): bundled offline build of client/server in development worktree; no packages installed.
REAL_MIGRATION_EXIT=0 (real-account-migration-test.txt): migrated a private copy of the user's exact original-schema database1->2; both accounts and every stored account/password/score/token value identical; original and backup untouched.

Private consistent backup: /home/stanley/.local/state/dds-asteroids/backups/accounts-before-multi-3y42hbou.sqlite3 (mode0600 under mode0700 backups). No private data is included in the source ZIP. The live user's old server was left running to protect their session; the primary runtime contains updated source. Close the old launcher terminal, then double-click RunGame.cmd to compile/restart and upgrade the live store. Do not run the older game against the upgraded schema. Historical artifacts are retained, not the current distribution.

Distribute DDS_Asteroids_offline_source_multi_accounts.zip. No new DDS wire/security mechanism change, full-suite/soak/sanitizer rerun or live-session interference. Account registration/login and migration were verified through actual store APIs; no new concurrent DDS network test was run alongside the user's live server. External packet capture remains pending.
