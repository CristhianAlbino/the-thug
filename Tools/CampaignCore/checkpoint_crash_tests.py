"""Kill actual writer processes, then verify whole-checkpoint recovery and locks.

These are process-crash tests. They do not emulate storage-controller power loss.
"""
from pathlib import Path
import subprocess
import sys
import tempfile
import time

driver = str(Path(sys.argv[1]).resolve())
checks = 0


def check(value, message):
    global checks
    checks += 1
    if not value:
        raise AssertionError(message)


def call(command, slot, *args, expected=0):
    result = subprocess.run([driver, command, str(slot), *map(str, args)], capture_output=True, text=True, timeout=15)
    check(result.returncode == expected, f"{command} process result: {result.returncode}: {result.stderr}")
    return result.stdout


def load(slot):
    status, clock = call("load", slot).split()
    return int(status), float(clock)


for stage in range(6):
    with tempfile.TemporaryDirectory(prefix="thug-crash-") as directory:
        slot = Path(directory) / "slot.sav"
        call("seed", slot)
        call("crash", slot, stage, expected=73)
        check(load(slot) == (0, 9 if stage >= 4 else 1), "Crash exposed a partial or incorrect primary")
        if stage >= 3:
            slot.write_bytes(b"damaged-primary")
            check(load(slot) == (1, 1), "Crash recovery did not explicitly select coherent backup")
        call("save", slot)
        check(not list(Path(directory).glob("*.thug-tmp.*")), "Restart did not clean abandoned reserved temporaries")

with tempfile.TemporaryDirectory(prefix="thug-lock-") as directory:
    slot = Path(directory) / "slot.sav"
    call("seed", slot)
    child = subprocess.Popen([driver, "hold", str(slot)], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    try:
        deadline = time.monotonic() + 10
        marker = Path(str(slot) + ".ready")
        while not marker.exists() and time.monotonic() < deadline and child.poll() is None:
            time.sleep(0.02)
        check(marker.exists(), "Writer never acquired slot lock")
        check(load(slot) == (6, 0), "Concurrent reader failed to return Busy without changing campaign")
        check(subprocess.run([driver, "save", str(slot)], timeout=15).returncode == 4, "Concurrent writer overwrote locked slot")
    finally:
        child.kill()
        child.communicate(timeout=15)
    # Windows can release abandoned byte-range locks after process termination.
    deadline = time.monotonic() + 5
    recovered = load(slot)
    while recovered[0] == 6 and time.monotonic() < deadline:
        time.sleep(0.02)
        recovered = load(slot)
    check(recovered == (0, 1), "Terminated writer left the slot inaccessible or changed old progress")
    call("save", slot)
    check(not list(Path(directory).glob("*.thug-tmp.*")), "Terminated lock owner left unresolved reserved temporary")

print(f"Checkpoint process crashes: {checks} checks passed")
