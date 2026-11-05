# Jordain feature proposal — approved

5 November 2026. Jordain confirmed Stanley approved the design and SQLite dependency before implementation. Jordain installed the dependency in Ubuntu, where `pkg-config --modversion sqlite3` reported `3.45.1`. The table preserves the J0 decisions. Implementation and verification status are in [PROGRESS.md](PROGRESS.md).

| Decision | Recommended first version |
|---|---|
| Login | Real server-validated username/password after mandatory DDS certificate authentication; login/register/masked password/logout in the existing GLFW window |
| Account scope | One account per provisioned certificate identity, up to six accounts and five concurrent players. Each player uses a distinct identity; sharing its private certificate is sharing the principal |
| Username/password | Username: 3–24 lowercase ASCII letters/digits/underscore. Password: 12–128 printable ASCII bytes, no trimming/case changes |
| Password storage | Existing OpenSSL scrypt, independent random salts, checked memory/work limits; one added bounded account/storage worker |
| Persistence | Add SQLite for account records, authoritative best scores and hashed resume tokens |
| Scores | Only server-earned scores; save before Leave/lease/reset and on game-over/graceful shutdown. Lower/duplicate scores cannot replace a best. Show own best and a bounded top-ten board |
| Remember me | Explicit opt-in, seven-day random token, rotated on resume. Password never saved. Private Linux profile; logout/online forget revoke, ordinary close keeps the remembered token |
| Network change | Add bounded account request/response topics and exact signed permissions. Existing gameplay structs stay unchanged, but mandatory login and new permissions require coordinated client/bundle/test updates |
| Session safety | Fresh server challenge, bounded account lease/heartbeat before Join, existing gameplay lease, no stale login result after logout, generic credential errors and bounded flood handling |

The proposed implementation uses the current C/OpenGL/GLFW/Cyclone DDS/OpenSSL stack. It adds SQLite and one server-side worker; it does not add a new game/UI framework.

Evidence prepared during J0:

- Draft IDL compiles with the installed Cyclone `idlc`; its proposed patch passes `git apply --check` and has **not** been applied.
- Draft schema/conditional score updates passed in-memory SQL checks, including rollback, identity binding, duplicate/lower scores, account bounds and token-rotation constraints. These are feasibility checks, not a C storage implementation.
- Existing OpenSSL scrypt candidate took about 0.34–0.43 seconds/check and about 133 MiB process peak RSS in a small local probe; the worker bounds concurrency. Gameplay/flood tests remain implementation acceptance work.
- SQLite C headers/pkg-config support are absent. After dependency approval, the human runs `sudo apt update` and `sudo apt install -y libsqlite3-dev` inside Ubuntu. No installation is performed by this proposal.

Reviewable details:

- [Full account, storage, token and acceptance design](ACCOUNT_FEATURE_DESIGN.md)
- [Concrete bounded IDL](proposals/accounts_v1.idl) and [unapplied proposed patch](proposals/accounts_v1.patch)
- [Draft SQL schema](proposals/accounts_schema_v1.sql)

Stanley's handover J0 says: “Obtain Stanley's approval BEFORE applying a breaking IDL/topic/policy change.”

The approval covers the policy defaults, SQLite dependency, and linked account protocol and permissions change. Stanley's manual acceptance of the resulting game is a separate step.
