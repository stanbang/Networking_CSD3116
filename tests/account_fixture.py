"""Private account state and pipe credentials for real game tests. Team + AI, 2026-11-05."""
import os
from pathlib import Path
import secrets
import subprocess
import tempfile
import time


class AccountFixture:
    def __init__(self):
        self.storage = tempfile.TemporaryDirectory(prefix="asteroids-accounts-")
        self.root = Path(self.storage.name)
        self.database = self.root / "database"
        self.profiles = self.root / "profiles"
        self.database.mkdir(mode=0o700)
        self.profiles.mkdir(mode=0o700)
        self.passwords = {i: secrets.token_hex(20) for i in range(1, 7)}
        self.started = {}

    def credentials(self, identity):
        return f"player_{identity}", self.passwords[identity]

    def spawn(self, command, *, credentials=True, register=None, name_password=None,
              profile_dir=None, **kwargs):
        """Keep generated passwords out of command arguments, logs and evidence."""
        command = list(map(str, command))
        role = next(Path(value).name for value in command if Path(value).name in ("server", "client"))
        if role == "server":
            return subprocess.Popen([*command, "--account-dir", str(self.database)], **kwargs)
        command += ["--profile-dir", str(profile_dir or self.profiles)]
        fault = command[command.index("--fault") + 1] if "--fault" in command else ""
        if not credentials or fault in ("wrong-ca", "no-cert", "rogue-server"):
            return subprocess.Popen(command, **kwargs)
        identity = int(command[command.index("--identity") + 1]) if "--identity" in command else 1
        if register is None:
            register = identity not in self.started
        if identity in self.started:
            # Space repeated password attempts to respect the production KDF limit.
            time.sleep(max(0, self.started[identity] + 2.4 - time.monotonic()))
        self.started[identity] = time.monotonic()
        username, password = name_password or self.credentials(identity)
        read_fd, write_fd = os.pipe()
        try:
            try:
                os.write(write_fd, f"{username}\n{password}\n".encode("ascii"))
            finally:
                os.close(write_fd)
            command += ["--credentials-fd", str(read_fd)]
            if register:
                command += ["--register"]
            return subprocess.Popen(command, pass_fds=(read_fd,), **kwargs)
        finally:
            os.close(read_fd)
