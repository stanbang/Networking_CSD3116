# Build protections and runtime checking

Purpose: explain P4 build assertions and reproduce checks. Author: <team>, with Codex assistance. Date: 2026-11-04.

```bash
make -j2 all test
make check-hardening
make -j2 SAN=asan all test
make SAN=asan check-hardening
make -j2 SAN=tsan all
make SAN=tsan check-hardening
```

Release executables are under `build/release`; AddressSanitizer plus UBSan executables under `build/asan`; ThreadSanitizer executables under `build/tsan`. ASan and TSan are separate builds. Unknown `SAN` values are rejected instead of quietly creating an unsanitized directory. Makefile changes invalidate application/test executables. Changes to arbitrary command-line compiler flags require an explicit rebuild; do not infer a binary's settings from a new make command that says nothing needs rebuilding.

`tools/check_hardening.sh` examines the actual ELF files with GNU `readelf`, returning nonzero if any required protection is absent or the binary/tool is missing. It requires executable PIE metadata (`DYN` and the dynamic `PIE` flag), a nonexecutable `GNU_STACK`, a `GNU_RELRO` segment, immediate binding (`NOW`/`BIND_NOW`), and an undefined `__stack_chk_fail` dependency. The latter demonstrates a canary-check dependency, not that every function contains a canary. The command prints the actual matching ELF metadata. A missing assertion cannot be hidden by a final successful `echo` or `|| true`.

| Protection | What it helps prevent | Practical limit |
|---|---|---|
| Stack protector | Detects some stack-buffer overwrites before a protected function returns | Does not prevent all out-of-bounds access, heap corruption, or data-only attacks |
| NX stack | Blocks execution of injected code on the stack | Does not prevent return-oriented attacks or corruption of writable data |
| PIE with host ASLR | Lets the loader randomize executable addresses | Information leaks can undermine randomization; PIE alone does not prove ASLR is enabled |
| Full RELRO and immediate binding | Makes relocation areas read-only after startup and avoids lazy symbol binding | Does not remove every writable function pointer or memory bug |
| Fortify with optimization | Adds compiler/libc checks where object sizes are known | Cannot check every pointer or dynamically sized operation |

The build adds `-z noexecstack` explicitly, along with existing PIE, RELRO, immediate binding, stack protection and Fortify flags. Sanitizer builds retain these flags and add frame pointers for diagnostics. No host ASLR setting is changed. Inspect the actual host value separately with `cat /proc/sys/kernel/randomize_va_space`.

Build success, ELF checks and unit tests are distinct from runtime sanitizer/Valgrind evidence. In particular, a sanitizer runtime that cannot initialize on the host is not a clean game run. TEST_REPORT.md records actual five-client/churn runs, tool outputs, minimal environment reproducers and any unresolved limits. Native DDS/OpenSSL/GL dependencies come from the installed distribution; these builds do not automatically rebuild those libraries with sanitizer instrumentation.
