#!/usr/bin/env python3
"""P0 baseline GUI evidence only; CSD3116 team, AI-assisted, 2026-11-04.

Run from repository root, after make baseline, on an isolated Xvfb display:
  xvfb-run -a env LIBGL_ALWAYS_SOFTWARE=1 python3 tests/p0_gui_smoke.py --output build/evidence/p0-gui

This verifies labelled square movement, not asteroid gameplay or security.
No baseline source is changed. Only processes created here are stopped.
Xvfb isolates the display only; baseline DDS still uses its default domain.
"""
import argparse
import fcntl
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

WIDTH, HEIGHT, SIDE = 800, 600, 50
TITLE = "^DDS Client"


def run(*args):
    """Run argv synchronously; return stdout or raise on tool failure."""
    return subprocess.check_output(args, stderr=subprocess.STDOUT, text=True).strip()


def require_tools():
    """Fail before creating a session when a required executable is absent."""
    for tool in ("xdotool", "import", "convert", "identify", "stdbuf"):
        if shutil.which(tool) is None:
            raise RuntimeError("Missing prerequisite: " + tool)
    if not os.environ.get("DISPLAY"):
        raise RuntimeError("Run this harness inside xvfb-run -a")
    if os.environ.get("LIBGL_ALWAYS_SOFTWARE") != "1":
        raise RuntimeError("Set LIBGL_ALWAYS_SOFTWARE=1 for this smoke test")
    if os.environ.get("CYCLONEDDS_URI"):
        raise RuntimeError("P0 expects default DDS domain 0; unset CYCLONEDDS_URI first")


def acquire_session_lock():
    """Hold the launcher's same lock throughout this test and its owned cleanup."""
    root = Path(__file__).resolve().parent.parent
    work = root / "build" / "p0-play"
    work.mkdir(mode=0o700, parents=True, exist_ok=True)
    if work.resolve() != work or (work / "lock").is_symlink():
        raise RuntimeError("Refusing a symlinked P0 runtime directory or lock")
    lock = (work / "lock").open("a")
    try:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        lock.close()
        raise RuntimeError("Another P0 launch, stop, or smoke test is in progress")
    return lock


def require_no_local_baseline(server, client):
    """Reject exact locally running baseline binaries; remote DDS peers remain possible."""
    binaries = {server.resolve(), client.resolve()}
    if not all(binary.is_file() for binary in binaries):
        raise RuntimeError("Build both baseline binaries before the GUI smoke test")
    for process_dir in Path("/proc").iterdir():
        if not process_dir.name.isdigit():
            continue
        try:
            executable = Path(os.readlink(process_dir / "exe").removesuffix(" (deleted)"))
        except (FileNotFoundError, PermissionError, ProcessLookupError):
            continue
        if executable in binaries:
            raise RuntimeError("A baseline executable is already running locally; stop its owned session first")


def windows():
    """Return matching X window IDs; an empty search is expected before launch."""
    result = subprocess.run(["xdotool", "search", "--name", TITLE], capture_output=True, text=True)
    if result.returncode not in (0, 1):
        raise RuntimeError("xdotool search failed: " + result.stderr)
    return set(result.stdout.split())


def wait_for(predicate, description, timeout=12):
    """Wait at most timeout seconds for a true predicate; return its value."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        value = predicate()
        if value:
            return value
        time.sleep(0.1)
    raise RuntimeError("Timed out waiting for " + description)


def start_process(binary, log_path, processes, handles):
    """Start one owned process, recording its output and cleanup handle."""
    handle = log_path.open("w")
    handles.append(handle)
    process = subprocess.Popen(["stdbuf", "-oL", "-eL", str(binary)], stdout=handle, stderr=subprocess.STDOUT)
    processes.append(process)
    return process


def start_client(binary, out_dir, number, processes, handles):
    """Start a sequential joiner and return its newly created X window ID."""
    previous = windows()
    log_path = out_dir / ("client%d.log" % number)
    process = start_process(binary, log_path, processes, handles)
    window = wait_for(lambda: windows() - previous, "client window").pop()
    wait_for(lambda: "Join response received" in log_path.read_text(), "client join response")
    if process.poll() is not None:
        raise RuntimeError("Client exited during join")
    time.sleep(0.5)
    return window


def square_positions(rgb):
    """Find 50-pixel red square borders in an 800x600 RGB image; return x,y pairs."""
    if len(rgb) != WIDTH * HEIGHT * 3:
        raise RuntimeError("Unexpected client image dimensions")
    red = bytearray(WIDTH * HEIGHT)
    for pixel in range(WIDTH * HEIGHT):
        offset = pixel * 3
        red[pixel] = rgb[offset] > 150 and rgb[offset + 1] < 100 and rgb[offset + 2] < 100
    candidates = []
    for y in range(HEIGHT - SIDE):
        x = 0
        while x < WIDTH:
            if not red[y * WIDTH + x]:
                x += 1
                continue
            start = x
            while x < WIDTH and red[y * WIDTH + x]:
                x += 1
            if SIDE - 1 <= x - start <= SIDE + 2:
                candidates.append((start, y))
    squares = set()
    for x, y in candidates:
        # Text and HUD strokes cannot satisfy both the lower edge and vertical borders.
        lower = sum(red[(y + SIDE) * WIDTH + xx] for xx in range(x, min(x + SIDE, WIDTH)))
        edge_counts = [max(sum(red[yy * WIDTH + xx] for yy in range(y, y + SIDE))
                           for xx in range(max(0, edge - 1), min(WIDTH, edge + 2)))
                       for edge in (x, min(x + SIDE, WIDTH - 1))]
        if lower >= SIDE - 2 and min(edge_counts) >= SIDE - 2:
            squares.add((x, y))
    return squares


def capture(window, path):
    """Save a PNG screenshot and return detected square positions from its pixels."""
    # Xvfb has no compositor/backing store for a fully obscured OpenGL window.
    run("xdotool", "windowraise", window)
    time.sleep(0.2)
    run("import", "-window", window, str(path))
    dimensions = run("identify", "-format", "%wx%h", str(path))
    if dimensions != "800x600":
        raise RuntimeError("Unexpected window dimensions: " + dimensions)
    rgb = subprocess.check_output(["convert", str(path), "-depth", "8", "rgb:-"])
    positions = square_positions(rgb)
    if not positions:
        raise RuntimeError("No square borders detected in " + str(path))
    return positions


def capture_pair(window_ids, out_dir, label):
    """Capture both clients after state has settled; require identical square positions."""
    time.sleep(0.4)
    positions = [capture(window, out_dir / (label + "-client%d.png" % (index + 1)))
                 for index, window in enumerate(window_ids)]
    if len(positions[0]) != 2 or positions[0] != positions[1]:
        raise RuntimeError("Both clients must show the same two square positions: " + repr(positions))
    return positions[0]


def choose_direction(moving, fixed):
    """Choose a clear axis direction with at least 30 pixels of unobstructed travel."""
    x, y = moving
    fx, fy = fixed
    choices = [("Right", WIDTH - SIDE - x), ("Left", x), ("Down", HEIGHT - SIDE - y), ("Up", y)]
    for direction, room in sorted(choices, key=lambda item: item[1], reverse=True):
        if room < 35:
            continue
        tx = x + (30 if direction == "Right" else -30 if direction == "Left" else 0)
        ty = y + (30 if direction == "Down" else -30 if direction == "Up" else 0)
        if tx <= fx + SIDE and fx <= tx + SIDE and ty <= fy + SIDE and fy <= ty + SIDE:
            continue
        return direction
    raise RuntimeError("Cannot choose a safe keyboard movement from these spawn positions")


def move_and_check(window, window_ids, before, moving, fixed, out_dir, label):
    """Inject held keyboard input, then assert only the intended square moves on both clients."""
    direction = choose_direction(moving, fixed)
    run("xdotool", "windowfocus", "--sync", window)
    run("xdotool", "keydown", direction)
    try:
        time.sleep(0.8)
    finally:
        run("xdotool", "keyup", direction)
    after = capture_pair(window_ids, out_dir, label)
    if fixed not in after or after == before:
        raise RuntimeError("Input failed to move only the intended player")
    moved = (after - {fixed}).pop()
    distance = abs(moved[0] - moving[0]) + abs(moved[1] - moving[1])
    if distance < 2:
        raise RuntimeError("Movement was smaller than two pixels")
    return after, moved, {"direction": direction, "from": moving, "to": moved, "distance_pixels": distance}


def exercise(args, out_dir, processes, handles):
    """Run baseline server then two clients; return measured movement and lifetime evidence."""
    started = time.monotonic()
    server = start_process(args.server.resolve(), out_dir / "server.log", processes, handles)
    time.sleep(0.8)
    if server.poll() is not None:
        raise RuntimeError("Server exited before client launch")
    first = start_client(args.client.resolve(), out_dir, 1, processes, handles)
    one = capture(first, out_dir / "single-client1.png")
    if len(one) != 1:
        raise RuntimeError("First client must initially display one player")
    player1 = next(iter(one))
    second = start_client(args.client.resolve(), out_dir, 2, processes, handles)
    second_started = time.monotonic()
    ids = [first, second]
    before = capture_pair(ids, out_dir, "before")
    if player1 not in before:
        raise RuntimeError("First player moved before keyboard input")
    player2 = (before - {player1}).pop()
    after1, player1, evidence1 = move_and_check(first, ids, before, player1, player2, out_dir, "after-player1")
    after2, player2, evidence2 = move_and_check(second, ids, after1, player2, player1, out_dir, "after-player2")
    remaining = 10.0 - (time.monotonic() - second_started)
    if remaining > 0:
        time.sleep(remaining)
    final = capture_pair(ids, out_dir, "final")
    if final != after2 or any(process.poll() is not None for process in processes):
        raise RuntimeError("Session did not remain alive and stable for ten seconds")
    return {"status": "PASS", "scope": "P0 insecure baseline squares only", "windows": ids,
            "client1_movement": evidence1, "client2_movement": evidence2,
            "two_client_seconds": round(time.monotonic() - second_started, 3),
            "total_seconds": round(time.monotonic() - started, 3)}


def stop_owned(processes, handles):
    """Terminate and reap only this harness's children; close owned output handles."""
    for process in reversed(processes):
        if process.poll() is None:
            process.terminate()
    for process in reversed(processes):
        try:
            process.wait(timeout=3)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=3)
    for handle in handles:
        handle.close()


def main():
    """Parse paths, enforce preflight, record measured PASS or actual FAIL, return exit code."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--server", type=Path, default=Path("build/release/baseline-server"))
    parser.add_argument("--client", type=Path, default=Path("build/release/baseline-client"))
    args = parser.parse_args()
    processes, handles, created_output, session_lock = [], [], False, None
    try:
        require_tools()
        session_lock = acquire_session_lock()
        require_no_local_baseline(args.server, args.client)
        args.output.mkdir(parents=True, exist_ok=False)
        created_output = True
        if windows():
            raise RuntimeError("Refusing to share a display with an existing baseline client")
        result = exercise(args, args.output, processes, handles)
    except Exception as error:
        result = {"status": "FAIL", "error": str(error), "scope": "P0 insecure baseline squares only"}
    finally:
        stop_owned(processes, handles)
        if session_lock is not None:
            session_lock.close()
    payload = json.dumps(result, indent=2)
    print(payload)
    if created_output:
        (args.output / "summary.json").write_text(payload + "\n")
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    sys.exit(main())
