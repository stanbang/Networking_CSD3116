"""Native-secured account, score restart and resume regression. Team + AI, 2026-11-05."""
import argparse
import os
from pathlib import Path
import re
import secrets
import sqlite3
import subprocess

from account_fixture import AccountFixture
from p1_network import stop, wait_line


class Run:
    def __init__(self, args):
        self.out = args.output_dir
        self.out.mkdir(parents=True, exist_ok=False)
        self.security = args.security_dir
        self.bin = args.bin_dir.resolve()
        self.fixture = AccountFixture()
        self.children = []

    def launch(self, name, kind, *extra, **kwargs):
        path = self.out / f"{name}.log"
        command = [self.bin / kind, "--security-dir", self.security, *extra]
        with path.open("x") as stream:
            if kind == "account-join-probe":
                child = subprocess.Popen(list(map(str, command)), stdout=stream, stderr=subprocess.STDOUT)
            else:
                child = self.fixture.spawn(command, stdout=stream, stderr=subprocess.STDOUT, **kwargs)
        self.children.append(child)
        return child, path

    def server(self, name):
        child, path = self.launch(name, "server", "--duration", 120, "--audit-file", self.out / f"{name}-audit.log")
        wait_line(path, r"SERVER_READY ", child, timeout=10)
        return child, path

    def client(self, name, identity, *extra, **kwargs):
        return self.launch(name, "client", "--identity", identity, "--headless", "--duration", 25, *extra, **kwargs)

    def expect(self, name, identity, pattern, *extra, success=True, **kwargs):
        child, path = self.client(name, identity, *extra, **kwargs)
        result = child.wait(timeout=30)
        text = path.read_text()
        assert (result == 0) == success and re.search(pattern, text), f"{name} exit={result}; inspect {path}"
        return text

    def close(self):
        for child in reversed(self.children):
            if child.poll() is None:
                stop(child)
        self.fixture.storage.cleanup()


def persisted_score(run):
    db_path = run.fixture.database / "accounts.sqlite3"
    with sqlite3.connect(db_path.as_uri() + "?mode=ro", uri=True) as db:
        row = db.execute("SELECT best_score FROM best_scores JOIN accounts USING(account_id) "
                         "WHERE identity_channel=1").fetchone()
        assert row and row[0] > 0, "Server did not persist a real score"
        return row[0]


def play_and_save(run):
    server, path = run.server("server-before")
    first, first_log = run.client("first", 1, "--remember", "--bot", "hunter")
    wait_line(first_log, r"CLIENT_JOIN .*status=accepted", first, timeout=12)
    second, second_log = run.client("second", 2, "--bot", "hunter")
    wait_line(second_log, r"CLIENT_JOIN .*status=accepted", second, timeout=12)
    probe, log = run.launch("certificate-only", "account-join-probe", "--identity", 6)
    assert probe.wait(timeout=16) == 0, f"Certificate-only probe failed. See {log}"
    for line in log.read_text().splitlines():
        if line.startswith("PASS "):
            print(line, flush=True)
    duplicate_dir = run.fixture.root / "duplicate-profile"
    duplicate_dir.mkdir(mode=0o700)
    run.expect("duplicate", 1, r"CLIENT_ACCOUNT operation=hello status=3", "--account-only",
               success=False, register=False, profile_dir=duplicate_dir)
    wait_line(first_log, r"CLIENT_SNAPSHOT .*score=[1-9]\d*", first, timeout=19)
    remembered, = run.fixture.profiles.glob("p1-*.profile")
    old_dir = run.fixture.root / "old-profile"
    old_dir.mkdir(mode=0o700)
    with os.fdopen(os.open(old_dir / remembered.name, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600), "wb") as copy:
        copy.write(remembered.read_bytes())
    assert stop(first) == 0 and stop(second) == 0 and stop(server) == 0
    earned = max(int(value) for value in re.findall(r"\bscore=(\d+)", first_log.read_text()))
    best = persisted_score(run)
    assert best >= earned > 0
    assert re.search(r"SERVER_SUMMARY joins=2 .*peak=2 ", path.read_text())
    print(f"PASS two accounts played and server saved an earned best of {best}", flush=True)
    return old_dir, best


def verify_restart(run, old_dir, best):
    server, path = run.server("server-after")
    result = run.expect("resume", 1, r"CLIENT_ACCOUNT operation=resume status=0",
                        "--account-action", "leaderboard", credentials=False)
    assert re.search(rf"CLIENT_LEADERBOARD count=\d+ own_best={best}\b", result)
    run.expect("old-token", 1, r"CLIENT_ACCOUNT operation=resume status=2", "--account-only",
               credentials=False, profile_dir=old_dir, success=False)
    wrong = ("player_1", secrets.token_hex(20))
    run.expect("wrong-password", 1, r"CLIENT_ACCOUNT operation=login status=2", "--account-only",
               register=False, name_password=wrong, success=False)
    run.expect("wrong-identity", 2, r"CLIENT_ACCOUNT operation=login status=2", "--account-only",
               register=False, name_password=run.fixture.credentials(1), success=False)
    run.expect("logout", 1, r"CLIENT_ACCOUNT operation=logout status=0 persisted=1",
               "--account-action", "logout", credentials=False)
    assert not list(run.fixture.profiles.glob("p1-*.profile"))
    run.expect("remember-again", 1, r"CLIENT_PROFILE saved=1", "--account-only", "--remember",
               register=False)
    run.expect("forget", 1, r"CLIENT_ACCOUNT operation=forget status=0 persisted=1",
               "--account-action", "forget", credentials=False)
    assert not list(run.fixture.profiles.glob("p1-*.profile"))
    assert stop(server) == 0
    assert "SERVER_SUMMARY joins=0 " in path.read_text()
    with sqlite3.connect((run.fixture.database / "accounts.sqlite3").as_uri() + "?mode=ro", uri=True) as db:
        assert db.execute("SELECT count(*) FROM resume_tokens").fetchone() == (0,)
    print("PASS persisted leaderboard, token rotation and replay rejection, credential binding, logout and forget revocation")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--security-dir", required=True)
    parser.add_argument("--bin-dir", type=Path, default=Path("build/release"))
    parser.add_argument("--output-dir", required=True, type=Path)
    args = parser.parse_args()
    run = Run(args)
    try:
        verify_restart(run, *play_and_save(run))
    finally:
        run.close()


if __name__ == "__main__":
    main()
