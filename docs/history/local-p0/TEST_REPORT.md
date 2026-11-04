# Test report

Purpose: preserve actual P0 evidence and missing verification. CSD3116 team with Codex assistance, 4 November 2026 (Singapore).

## Initial audit scope and results (historical; resumed results below)

These are tests of the starter baseline and environment, not of an implemented asteroid game. Real builder and tester subagents independently ran the checks below. No server/client process was started.

| Check | Result | Evidence / limit |
|---|---|---|
| Forced baseline build | PASS with warnings | `make -B baseline`, exit 0 |
| Native DDS Security plugins | PRESENT | Three plugin paths below; not configured |
| Display prerequisites | PRESENT | DISPLAY/WAYLAND and WSLg directory only |
| ASLR setting | PASS | `2` |
| Tracked key/pem path check | PASS | Empty output, not a full content/history secret scan |
| Required GUI/capture tools | FAIL | Missing executables and package query below |
| P0 two-window movement | NOT RUN | Environment stop rule |
| P1-P4 production builds/unit/integration/security tests | NOT RUN | No production implementation exists |
| ASan/UBSan/TSan/Valgrind five-client soak | NOT RUN | No finite headless implementation exists |
| Hardening verification | NOT RUN | Current target prints properties without enforcing them |
| Gameplay assertions/slot timeout | NOT RUN | Not implemented |
| Ciphertext packet capture | NOT RUN | Tool missing; security not configured |
| GUI smoke/screenshots | NOT RUN | Tool missing |
| Clean-clone README reproduction | NOT RUN | Starter instructions include unfinished targets |

## Reproduction context

Run commands in Ubuntu:

```bash
cd /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116
make -B baseline
pkg-config --modversion CycloneDDS glfw3 openssl
find /usr/lib /usr/local/lib -name 'libdds_security_*' 2>/dev/null
printf '%s\n' "$DISPLAY" "$WAYLAND_DISPLAY"
test -d /mnt/wslg
cat /proc/sys/kernel/randomize_va_space
git status --short
git ls-files -- '*.key' '*.pem'
```

The shell command for this inventory completed with exit 0. Individual missing-tool failures are separately preserved below.

## Raw independent tester output

```text
CycloneDDS=0.10.4
glfw3=3.3.10
openssl=3.0.13
/usr/lib/x86_64-linux-gnu/libddsc0debian/libdds_security_ac.so
/usr/lib/x86_64-linux-gnu/libddsc0debian/libdds_security_auth.so
/usr/lib/x86_64-linux-gnu/libddsc0debian/libdds_security_crypto.so
DISPLAY=:0
WAYLAND_DISPLAY=wayland-0
WSLg=0
ASLR=2
git_status
 M baseline/client.c
 M baseline/server.c
tracked_key_pem_paths
```

`WSLg=0` is the exit status of `test -d /mnt/wslg`: the directory exists. It is not a GUI test result.

Tool checks used `command -v TOOL`, with explicit reporting from the failure branch:

```text
/usr/bin/gcc
/usr/bin/make
/usr/bin/idlc
/usr/bin/valgrind
xvfb-run_MISSING_EXIT_1
import_MISSING_EXIT_1
tcpdump_MISSING_EXIT_1
tshark_MISSING_EXIT_1
xdotool_MISSING_EXIT_1
dpkg-query: no packages found matching xvfb
dpkg-query: no packages found matching imagemagick
dpkg-query: no packages found matching tcpdump
dpkg-query: no packages found matching tshark
dpkg-query: no packages found matching xdotool
xauth  1:1.1.2-1build1
```

The package query exited 1. `xauth` is installed. `tshark` is an alternative to tcpdump, not an additional requirement.

## Raw builder output

```text
$ make baseline
make: Nothing to be done for 'baseline'.
exit 0

$ make -B baseline
mkdir -p build/gen
cd build/gen && idlc -l c ../../idl/messages.idl
No default extensibility provided. For one or more of the aggregated types in the IDL the extensibility is not explicitly set. Currently the default extensibility for these types is 'final', but this may change to 'appendable' in a future release because that is the default in the DDS XTypes specification.
mkdir -p build/release
cc -std=gnu11 -O2 -g -Wall -Wextra -Wformat=2 -Wformat-security -fstack-protector-strong -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE  -Isrc -Ibuild/gen -Ithird_party  baseline/server.c build/gen/messages.c -o build/release/baseline-server -pie -Wl,-z,relro,-z,now  -lddsc  -pthread
baseline/server.c: In function 'worker':
baseline/server.c:92:20: warning: unused parameter 'arg' [-Wunused-parameter]
   92 | void *worker(void *arg)
      |              ~~~~~~^~~
mkdir -p build/release
cc -std=gnu11 -O2 -g -Wall -Wextra -Wformat=2 -Wformat-security -fstack-protector-strong -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE  -Isrc -Ibuild/gen -Ithird_party   baseline/client.c build/gen/messages.c -o build/release/baseline-client -pie -Wl,-z,relro,-z,now  -lddsc  -lglfw  -lGL -lm -pthread
In file included from baseline/client.c:18:
third_party/stb_easy_font.h: In function 'stb_easy_font_print':
third_party/stb_easy_font.h:205:29: warning: missing braces around initializer [-Wmissing-braces]
  205 |     stb_easy_font_color c = { 255,255,255,255 }; // use structure copying to avoid needing depending on memcpy()
      |                             ^
      |                               {               }
baseline/client.c: In function 'worker':
baseline/client.c:77:20: warning: unused parameter 'arg' [-Wunused-parameter]
   77 | void *worker(void *arg)
      |              ~~~~~~^~~
third_party/stb_easy_font.h: At top level:
third_party/stb_easy_font.h:248:12: warning: 'stb_easy_font_height' defined but not used [-Wunused-function]
  248 | static int stb_easy_font_height(char *text)
      |            ^~~~~~~~~~~~~~~~~~~~
third_party/stb_easy_font.h:230:12: warning: 'stb_easy_font_width' defined but not used [-Wunused-function]
  230 | static int stb_easy_font_width(char *text)
      |            ^~~~~~~~~~~~~~~~~~~
third_party/stb_easy_font.h:194:13: warning: 'stb_easy_font_spacing' defined but not used [-Wunused-function]
  194 | static void stb_easy_font_spacing(float spacing)
      |             ^~~~~~~~~~~~~~~~~~~~~
exit 0
```

Diagnostic quotation marks above are normalized to ASCII; message content is unchanged. This output was produced by the real builder subagent, not invented as an expected result.

## Environment action

The user prompt explicitly prohibits agent sudo and says to stop if a required tool is missing. No install was attempted. Minimal human command:

```bash
sudo apt update && sudo apt install -y xvfb imagemagick tcpdump
```

After installation, recheck tools and capture permissions, then run P0 runtime checks. Do not mark later acceptance tests PASS because tools install successfully.

## Eventual human playtest checklist (not yet executed)

- Start the server, then two clients on different machines. Confirm unique identities and matching world state.
- Check turning, thrust, inertia, firing, wrap-around, hits, splitting, scoring, lives, respawn, waves and restart.
- Check readable HUD state and latency, smooth motion and controls, and clear rejection/disconnect messages.
- Kill a client; verify its slot expires and a replacement joins while the other players continue.
- Inspect a real packet capture; confirm game payloads are encrypted, and document visible metadata separately.
- Have both team members explain the security decisions, locking, validation, benchmarks and accepted AI output.


## Resumed preflight after user installation update

The user restored GUI smoke and packet-capture requirements and added explicit phase/security/IDL approval gates. The earlier missing-tool inventory is retained as historical output.

Actual primary preflight output (command completed exit 0; each tool was queried independently, not fail-fast):

```text
/usr/bin/xvfb-run
/usr/bin/Xvfb
/usr/bin/import
/usr/bin/tcpdump
/usr/bin/xauth
0.10.4
3.3.10
3.0.13
/usr/lib/x86_64-linux-gnu/libddsc0debian/libdds_security_ac.so
/usr/lib/x86_64-linux-gnu/libddsc0debian/libdds_security_auth.so
/usr/lib/x86_64-linux-gnu/libddsc0debian/libdds_security_crypto.so
:0
wayland-0
```

`command -v xdotool` returned no path. Follow-up `dpkg-query -W xdotool` output:

```text
dpkg-query: no packages found matching xdotool
```

Independent tester preflight exited 1 because xdotool was missing. It confirmed installed package versions xvfb `2:21.1.12-1ubuntu1.9`, imagemagick `8:6.9.12.98+dfsg1-5.2build2`, tcpdump `4.99.4-3ubuntu4.24.04.1`. GUI movement test remains NOT RUN. No game runtime or packet-capture evidence is implied by this inventory.

Restored tester requirements: isolated `xvfb-run`, `LIBGL_ALWAYS_SOFTWARE=1`, sequential server/client startup, two clients for at least 10 seconds, xdotool keyboard movement, import before/after screenshots, inspect both windows and compare actual state changes. After phase checks pass, also open and confirm two WSLg desktop windows for the user manual gate.

Capture workflow: `tools/capture_demo.sh` is run with sudo by the human. The agent does not invoke sudo. Analyse the resulting `.pcap` with an unprivileged tcpdump reader. Baseline capture can establish plaintext reference behavior only; encryption verification belongs to the approved security implementation.


## P0 script checks: actual builder output

```text

$ bash -n tools/play.sh tools/stop.sh tools/capture_demo.sh
exit=0 expected=0

$ python3 -B -c import\ ast\,pathlib\;\ ast.parse\(pathlib.Path\(\"tools/p0_session.py\"\).read_text\(\)\)
exit=0 expected=0

$ env -u DISPLAY bash tools/play.sh
P0: DISPLAY is unset. Open a WSLg terminal and retry.
exit=1 expected=1

$ bash tools/capture_demo.sh 0
Usage: bash tools/capture_demo.sh [seconds 1..120] [basename.pcap] [interface=lo]
exit=1 expected=1

$ bash tools/capture_demo.sh 1 ../escape.pcap
Output must be a plain .pcap basename; no directory components.
exit=1 expected=1

$ bash tools/capture_demo.sh 1 safe.pcap any
Select one existing interface, for example lo or eth0; any is disallowed.
exit=1 expected=1

$ bash tools/stop.sh
No recorded P0 session.
exit=0 expected=0

CHECK_DRIVER_EXIT=0

```

## Harness safe checks: actual tester output

```text
AST syntax: PASS
Synthetic square detector: PASS (blank rejected; square found at 100,100)
Synthetic deleted-executable guard: PASS (running rebuilt baseline rejected)
Existing-output protection: PASS (exit 1; sentinel unchanged; no runtime called)
GUI/runtime tests: NOT RUN (xdotool absent; awaiting dependency gate)

```
These are syntax/synthetic/guard checks, not live gameplay or a managed-session shutdown test.

## Actual GUI verification and retained failure

```text
P0 independent GUI smoke verification - 2026-11-04

Run 1 command (repository root in WSL Ubuntu):
xvfb-run -a env LIBGL_ALWAYS_SOFTWARE=1 python3 -B tests/p0_gui_smoke.py --output build/evidence/p0-gui-run1
Exit status: 1
Raw combined output: build/evidence/p0-gui-run1-output.txt
Failure: No square borders detected in build/evidence/p0-gui-run1/before-client1.png
Visual inspection: single-client1 screenshot rendered a labelled Player 1 square; before-client1 PNG was entirely black. Both client logs continued receiving positions. Likely cause was occluded OpenGL window capture on Xvfb without preserved backing contents.
Harness-only correction: capture() now raises the target X window and allows 0.2 seconds for repaint before ImageMagick import. Baseline source untouched. Run 1 failure retained.

Run 2 command (repository root in WSL Ubuntu):
xvfb-run -a env LIBGL_ALWAYS_SOFTWARE=1 python3 -B tests/p0_gui_smoke.py --output build/evidence/p0-gui-run2
Exit status: 0
Raw combined output: build/evidence/p0-gui-run2-output.txt
Summary: build/evidence/p0-gui-run2/summary.json
Measured two-client lifetime: 11.087 seconds
Measured total lifetime: 15.564 seconds
Client 1 input Left: square (684,148) -> (635,148), 49 pixels
Client 2 input Left: square (739,397) -> (689,397), 50 pixels
Automated assertions: two squares shown identically in both clients before/after each movement; each input moves intended player while preserving other player's position; both clients and server remain alive for at least ten seconds; final positions stable.

Independent PNG visual inspection using view_image:
before-client1.png: 800x600 white arena, red Player 1 header, square labels 1 and 2.
after-player1-client1.png: square 1 moved left, square 2 unchanged.
after-player2-client2.png: Player 2 header; square 2 moved left, square 1 at its moved location.
final-client1.png and final-client2.png: distinct Player 1 / Player 2 headers; same two labelled square positions in both windows.
This is the insecure baseline square demo. It does not show asteroid gameplay or secure traffic, and the baseline has no Connected HUD.
Xvfb isolates the display only. DDS used unchanged default domain 0. No two-machine, security, packet-capture, sanitizer or game-feel pass is claimed.

Post-cleanup command:
command -v xdotool; dpkg-query -W xdotool; ps -C baseline-server -C baseline-client -C Xvfb -o pid=,stat=,comm=,args=; true
Command exit status: 0
Raw output:
/usr/bin/xdotool
xdotool 1:3.20160805.1-5build1
No process rows followed. No baseline-server, baseline-client, or Xvfb process remained at this check.

```

Run1 actual output:

```json
{
  "status": "FAIL",
  "error": "No square borders detected in build/evidence/p0-gui-run1/before-client1.png",
  "scope": "P0 insecure baseline squares only"
}

```

Run2 actual output (tester observed exit 0):

```json
{
  "status": "PASS",
  "scope": "P0 insecure baseline squares only",
  "windows": [
    "4194311",
    "2097159"
  ],
  "client1_movement": {
    "direction": "Left",
    "from": [
      684,
      148
    ],
    "to": [
      635,
      148
    ],
    "distance_pixels": 49
  },
  "client2_movement": {
    "direction": "Left",
    "from": [
      739,
      397
    ],
    "to": [
      689,
      397
    ],
    "distance_pixels": 50
  },
  "two_client_seconds": 11.087,
  "total_seconds": 15.564
}

```

## WSLg desktop launch: actual primary output

```text
$ wsl -d Ubuntu --cd /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116 -- bash tools/play.sh
P0 baseline launched: server and two clients report protocol activity.
Traffic is PLAINTEXT; this is the reference demo, not the secure game.
Visually check two windows, distinct Player 1/2 labels and movement.
Logs: /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/p0-play/20261104-111516-d9deba26
Recovery: cd '/mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116' && bash tools/stop.sh
Observed exit code: 0
DISPLAY=:0
server: live expected executable
client1: live expected executable
client2: live expected executable
WSLg_VISIBLE_WINDOWS=2
```

Actual window details from the unprivileged desktop verifier:

```json
[
  {
    "name": "client1",
    "pid": 3788,
    "window": "6291463",
    "title": "DDS Client (Click within the window to control)",
    "image": "/mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/evidence/p0-desktop-d9deba26/client1.png",
    "geometry": "WINDOW=6291463\nX=64\nY=146\nWIDTH=800\nHEIGHT=600\nSCREEN=0"
  },
  {
    "name": "client2",
    "pid": 3862,
    "window": "8388615",
    "title": "DDS Client (Click within the window to control)",
    "image": "/mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/evidence/p0-desktop-d9deba26/client2.png",
    "geometry": "WINDOW=8388615\nX=884\nY=186\nWIDTH=800\nHEIGHT=600\nSCREEN=0"
  }
]

```

Primary inspected both desktop PNGs. Both show the baseline world and numbered square outlines; window2 shows the full Player2 header, while window1's captured header is partly rendered in this snapshot. Unique player headings and complete shared state were verified in the automated run2 images. Desktop visibility is also confirmed by X11 visible-window enumeration on actual WSLg display :0 and PID mapping, not inferred from log messages alone. The manual view/feel check remains for the user.

Current verdict: P0 automated scope PASS with inherited baseline compiler warnings and security/threading defects explicitly retained. Desktop launch/visible windows PASS at observation time. Manual approval PENDING. Capture NOT RUN, secure gameplay NOT IMPLEMENTED, later-phase tests NOT RUN. Managed live-session stop and relaunch were subsequently tested successfully; see the lifecycle section below. The reopened desktop session is left open for the user. No sudo was run.


## Live managed-session lifecycle and final desktop state

```text
$ bash tools/play.sh  # duplicate managed session
P0: P0 already running; use bash tools/stop.sh first.
Observed exit code: 1 (expected refusal)

$ bash tools/stop.sh
client2: stopped owned P0 process
client1: stopped owned P0 process
server: stopped owned P0 process
Observed exit code: 0

$ ps -C baseline-server -C baseline-client -o pid=,stat=,comm=
$ xdotool search --onlyvisible --name "^DDS Client"
[no output from either command; combined final exit code 1]

$ bash tools/play.sh
P0 baseline launched: server and two clients report protocol activity.
Traffic is PLAINTEXT; this is the reference demo, not the secure game.
Visually check two windows, distinct Player 1/2 labels and movement.
Logs: /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/p0-play/20261104-111852-cecf1d22
Recovery: cd '/mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116' && bash tools/stop.sh
Observed exit code: 0

DISPLAY=:0
server: live expected executable
client1: live expected executable
client2: live expected executable
WSLg_VISIBLE_WINDOWS=2
Desktop verifier observed exit code: 0

```

Final reopened visible-window details:

```json
[
  {
    "name": "client1",
    "pid": 4145,
    "window": "6291463",
    "title": "DDS Client (Click within the window to control)",
    "image": "/mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/evidence/p0-desktop-cecf1d22/client1.png",
    "geometry": "WINDOW=6291463\nX=64\nY=146\nWIDTH=800\nHEIGHT=600\nSCREEN=0"
  },
  {
    "name": "client2",
    "pid": 4219,
    "window": "8388615",
    "title": "DDS Client (Click within the window to control)",
    "image": "/mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/evidence/p0-desktop-cecf1d22/client2.png",
    "geometry": "WINDOW=8388615\nX=884\nY=186\nWIDTH=800\nHEIGHT=600\nSCREEN=0"
  }
]

```

The earlier d9deba26 desktop session was intentionally stopped during lifecycle verification. The final session is cecf1d22, left open. The raw no-process/no-window observation was between stop and relaunch, not a claim about current state. No packet capture, sudo command or next-phase work was performed. STOP pending the user manual P0 approval.


## Human P0 capture attempt after approval

User supplied actual terminal output:

```text
P0 PLAINTEXT capture: lo, UDP ports 7400-7600, 30s.
Bound: 10000 packets, 256 captured bytes per packet (P0-only, truncated).
Default DDS domain 0 only; other traffic in that DDS port range can be captured.
Run play.sh in another terminal. Capturing packets does not prove encryption.
tcpdump: listening on lo, link-type EN10MB (Ethernet), snapshot length 256 bytes
0 packets captured
0 packets received by filter
0 packets dropped by kernel
No packets captured; P0 may use another interface. No evidence produced.
```

Verdict: NO EVIDENCE. No pcap was produced to analyse. Primary ss output showed actual baseline UDP sockets on ephemeral ports outside7400-7600; eth0 address172.24.167.12 and default routeeth0. A fixed domain port range is insufficient with the observed participant configuration. Interface alone is not established as the cause. The capture filter is being corrected without agent sudo; actual traffic evidence still requires human retry.


## Corrected capture filter: actual offline tester output

```text
CASE: 101 RTPS outside old port range
CASE: 102 RTPS inside old port range
CASE: 103 non-RTPS inside old port range
CASE: 104 non-RTPS outside old port range
CASE: 105 too-short UDP payload
CASE: 106 TCP payload starts RTPS
CASE: 107 RTPS magic at wrong payload offset
CASE: 108 magic-only heuristic limitation
COMMAND: ['tcpdump', '-nn', '-r', 'build/evidence/p0-filter-run1/synthetic-rtps-filter.pcap', '-w', 'build/evidence/p0-filter-run1/old-filter.pcap', 'udp and portrange 7400-7600']
EXIT: 0
STDERR: reading from file build/evidence/p0-filter-run1/synthetic-rtps-filter.pcap, link-type EN10MB (Ethernet), snapshot length 65535
COMMAND: ['tcpdump', '-nn', '-r', 'build/evidence/p0-filter-run1/synthetic-rtps-filter.pcap', '-w', 'build/evidence/p0-filter-run1/new-filter.pcap', 'ip and udp and udp[8:4] = 0x52545053']
EXIT: 0
STDERR: reading from file build/evidence/p0-filter-run1/synthetic-rtps-filter.pcap, link-type EN10MB (Ethernet), snapshot length 65535
OLD SELECTED IDs: [102, 103, 105]
NEW SELECTED IDs: [101, 102, 108]
PASS: ephemeral-port RTPS selected; old-range non-RTPS, short UDP, TCP, wrong-offset controls rejected.
LIMIT: magic-prefix capture heuristic also selects crafted RTPS-prefixed data; not an authentication/protocol-validity check.
LIMIT: synthetic offline IPv4 fixture only; no actual human capture, IPv6, fragment reassembly, or encryption proof.

```
Observed test-driver exit0. This is synthetic offline IPv4 filter validation only, not a real capture pass. Installed Cyclone q_protocol.h defines RTPS payload magic; installed pcap-filter documentation supports UDP header-relative arithmetic for IPv4. The fixed filter no longer assumes domain-assigned UDP ports.

During builder guard checks, a reused driver mistakenly invoked stop.sh and closed the owned desktop session. That action was outside the capture-only instruction; its actual output remains in build/evidence/p0-capture-filter-checks.txt. Primary disclosed this to the user and restored play.sh (exit0). The desktop verifier then exited0: DISPLAY=:0, two visible800x600 clients, PIDs359/434, and all three expected executables live. Final restored screenshot/geometry evidence is build/evidence/p0-desktop-7daeed4a/. No agent sudo or live capture was run.
