# Offline build and local game

On Windows: extract the entire ZIP, then double-click **RunGame.cmd**. Do not double-click run.sh; it is the Ubuntu entry point. Keep the launcher terminal open while playing. The first compile can take about a minute and prints server/client build progress. If a command fails, the Windows launcher keeps the error visible. The default WSL distribution must be Ubuntu24.04 with its C compiler already available.


Target: Ubuntu 24.04 x86-64 under WSL2 with WSLg. An existing GCC C toolchain, including standard C development headers and binutils, is required. The launcher performs no package installation or downloads. This bundled SDK does not target other Linux releases, macOS or native Windows.

Extract the complete ZIP into your Ubuntu home directory for faster compilation. Open an Ubuntu terminal in the extracted DDS_Asteroids directory and run:

```bash
bash run.sh
```

The script compiles the C sources, generates private local certificates on first use, starts the server, and opens two client login windows. Register a different account in each window, then log in. Select Remember Me to retain the account token on this machine. Passwords, scores and remembered sessions stay in private Linux user storage outside the submission. Use arrow keys to steer/thrust and Space to shoot. Ctrl+C in the launcher terminal stops its processes.

Build only: `bash tools/build_portable.sh`.
Temporary-account headless smoke: `bash run.sh --headless --duration 12`.

No make, pkg-config or idlc is needed. Generated DDS C files are hash-bound to the included IDL. GLFW, OpenGL software-renderer dependencies, Cyclone DDS with native security plugins, OpenSSL, SQLite, headers and helper Python are under portable/. Retain portable/linux-x86_64/licenses when distributing.

This launcher demonstrates two players on one machine. Separate-machine deployment needs separately provisioned participant credentials and network configuration. Never package generated private keys, account databases, saved profiles, packet captures, build logs or .git. Private credentials are generated locally and are absent from the SDK.

Crash recovery: a disconnected player has30seconds from the last input to reauthenticate with the same account and certificate identity. The game pauses during the reservation. After30seconds the whole round ends and clients close after the final display. Quitting the launcher stops all of its processes.

## Reopen a closed player window
Keep the original launcher terminal and the other player open. Double-click ReconnectPlayer1.cmd for the closed P1 window, or ReconnectPlayer2.cmd for P2. These reopen only that client and do not compile or start another server. In Ubuntu, run bash tools/rejoin.sh 1 (or2) from the game folder.

Sign in with the SAME account used by the closed window. Remember Me may sign in automatically. Return within30seconds of the last input to keep the ship/score. Play pauses while a joined player is absent. If30seconds expires, the whole round ends; start RunGame.cmd again for a new round. Before two players have joined, the game waits for the second player. Use reconnect for an existing session; use RunGame.cmd for a new session.

Passwords need at least6characters (maximum128). No uppercase, number or symbol is required. Existing accounts keep their current passwords. Enter signs in; Shift+Enter creates a new account.

Six-digit passwords are valid. Each player window can register multiple accounts with different usernames, without an artificial account-count limit. Press Enter to sign in or Shift+Enter to create another account. Usernames are unique across the server. Existing passwords and scores stay unchanged; each account remains bound to the player identity where it was created. The server upgrades older account databases transactionally on the next restart.
