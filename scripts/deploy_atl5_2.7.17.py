"""Deploy KTPAMXX 2.7.17 to ATL5 + activate KTPMatchHandler 0.10.133.

Sequence:
1. SCP ktpamx_i386.so + dodx_ktp_i386.so to ATL5 as *.so.new
2. Player-count check
3. Stop dodserver5
4. Swap both .so.new -> live
5. Restore KTPMatchHandler.amxx from .bad-0.10.133-needs-ktpamx-2.7.17
6. Start dodserver5
7. Verify all three load: KTPAMXX 2.7.17 + DODX (built from 2.7.17 tree) + KTPMatchHandler 0.10.133

The auto-swap script only fires at 3am cron; manual restarts bypass it.
That's why this script does the swap inline.
"""
import paramiko
import sys
import os
import time

HOST = "74.91.121.9"
USER = "dodserver"
PASS = "ktp"
PORT = "27019"
INSTANCE = "dodserver5"

LOCAL_KTPAMXX = r"N:\Nein_\KTP Git Projects\KTP DoD Server\serverfiles\dod\addons\ktpamx\dlls\ktpamx_i386.so"
LOCAL_DODX   = r"N:\Nein_\KTP Git Projects\KTP DoD Server\serverfiles\dod\addons\ktpamx\modules\dodx_ktp_i386.so"

REMOTE_BASE = f"/home/dodserver/dod-{PORT}/serverfiles/dod/addons/ktpamx"
REMOTE_KTPAMXX_LIVE = f"{REMOTE_BASE}/dlls/ktpamx_i386.so"
REMOTE_KTPAMXX_NEW  = f"{REMOTE_BASE}/dlls/ktpamx_i386.so.new"
REMOTE_DODX_LIVE    = f"{REMOTE_BASE}/modules/dodx_ktp_i386.so"
REMOTE_DODX_NEW     = f"{REMOTE_BASE}/modules/dodx_ktp_i386.so.new"
REMOTE_PLUGIN_LIVE  = f"{REMOTE_BASE}/plugins/KTPMatchHandler.amxx"
REMOTE_PLUGIN_BAK   = f"{REMOTE_BASE}/plugins/KTPMatchHandler.amxx.bad-0.10.133-needs-ktpamx-2.7.17"


def run(ssh, cmd, timeout=30, label=None):
    if label:
        print(f"\n=== {label} ===")
    print(f"$ {cmd}")
    stdin, stdout, stderr = ssh.exec_command(cmd, timeout=timeout)
    out = stdout.read().decode(errors="replace").strip()
    err = stderr.read().decode(errors="replace").strip()
    if out:
        print(out)
    if err and "Pseudo-terminal" not in err:
        print(f"[stderr] {err}", file=sys.stderr)
    return out, err


def main():
    for path in (LOCAL_KTPAMXX, LOCAL_DODX):
        if not os.path.exists(path):
            print(f"Local binary missing: {path}", file=sys.stderr)
            return 1

    ktpamx_size = os.path.getsize(LOCAL_KTPAMXX)
    dodx_size = os.path.getsize(LOCAL_DODX)
    print(f"Local ktpamx_i386.so:    {LOCAL_KTPAMXX} ({ktpamx_size:,} bytes)")
    print(f"Local dodx_ktp_i386.so:  {LOCAL_DODX} ({dodx_size:,} bytes)")

    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(HOST, username=USER, password=PASS, timeout=30)

    # Pre-check player count + current state.
    run(ssh, f"~/dod-{PORT}/{INSTANCE} details 2>&1 | grep -E 'Players|Status' | head -3",
        timeout=15, label="Pre-deploy state")
    run(ssh, f"ls -la {REMOTE_KTPAMXX_LIVE} {REMOTE_DODX_LIVE} {REMOTE_PLUGIN_BAK} 2>&1",
        timeout=15, label="Current binaries + backup")
    run(ssh, f"md5sum {REMOTE_KTPAMXX_LIVE} {REMOTE_DODX_LIVE} {REMOTE_PLUGIN_BAK} 2>&1",
        timeout=15, label="Current md5s")

    # SCP both binaries as .new.
    print("\n=== SCP ktpamx_i386.so -> .so.new ===")
    sftp = ssh.open_sftp()
    sftp.put(LOCAL_KTPAMXX, REMOTE_KTPAMXX_NEW)
    print("  ktpamx_i386.so.new uploaded")
    sftp.put(LOCAL_DODX, REMOTE_DODX_NEW)
    print("  dodx_ktp_i386.so.new uploaded")
    sftp.close()

    run(ssh, f"ls -la {REMOTE_KTPAMXX_NEW} {REMOTE_DODX_NEW} 2>&1; md5sum {REMOTE_KTPAMXX_NEW} {REMOTE_DODX_NEW}",
        timeout=15, label="Staged .new files")

    # Stop dodserver5 — operator authorized this.
    print("\n=== Stopping dodserver5 ===")
    run(ssh, f"~/dod-{PORT}/{INSTANCE} stop 2>&1 | tail -10", timeout=60)
    time.sleep(3)

    # Manual swap.
    run(ssh, f"mv {REMOTE_KTPAMXX_NEW} {REMOTE_KTPAMXX_LIVE} && echo OK", timeout=15, label="Swap ktpamx_i386.so")
    run(ssh, f"mv {REMOTE_DODX_NEW} {REMOTE_DODX_LIVE} && echo OK", timeout=15, label="Swap dodx_ktp_i386.so")

    # Restore correct 0.10.133 plugin (currently rolled back to 0.10.121).
    run(ssh, f"cp -v {REMOTE_PLUGIN_BAK} {REMOTE_PLUGIN_LIVE}", timeout=15, label="Restore KTPMatchHandler 0.10.133")
    run(ssh, f"md5sum {REMOTE_KTPAMXX_LIVE} {REMOTE_DODX_LIVE} {REMOTE_PLUGIN_LIVE}",
        timeout=15, label="Post-swap md5s")

    # Start dodserver5.
    print("\n=== Starting dodserver5 ===")
    run(ssh, f"~/dod-{PORT}/{INSTANCE} start 2>&1 | tail -15", timeout=120)

    # Belt-and-suspenders: recreate the monitoring lockfile in case it was missed.
    # The scheduled-restart script does this; manual restart paths may not.
    run(ssh, f"[ ! -f ~/dod-{PORT}/lgsm/lock/{INSTANCE}-monitoring.lock ] && "
             f"date +%s > ~/dod-{PORT}/lgsm/lock/{INSTANCE}-monitoring.lock && "
             f"echo 'lockfile recreated' || echo 'lockfile already present'",
        timeout=10, label="Lockfile check")

    # Give the engine ~10s to load plugins.
    time.sleep(10)

    # Verify the new versions are live.
    print("\n=== Verifying plugin load ===")
    run(ssh, f"~/dod-{PORT}/{INSTANCE} send 'amx version' 2>/dev/null; sleep 1.5; "
             f"tail -30 ~/dod-{PORT}/log/console/console.log | grep -iE 'amxx|version|KTPMatchHandler' | tail -10",
        timeout=20, label="amx version")

    run(ssh, f"~/dod-{PORT}/{INSTANCE} send 'amx plugins' 2>/dev/null; sleep 2; "
             f"tail -80 ~/dod-{PORT}/log/console/console.log | grep -E 'MatchHandler|debug|stopped|bad load' | tail -10",
        timeout=20, label="amx plugins KTPMatchHandler row")

    run(ssh, f"~/dod-{PORT}/{INSTANCE} send 'amx modules' 2>/dev/null; sleep 1.5; "
             f"tail -50 ~/dod-{PORT}/log/console/console.log | grep -iE 'dodx|amxx|reapi|curl' | tail -10",
        timeout=20, label="amx modules")

    ssh.close()
    print("\n=== Deploy complete ===")
    print("\nExpected results above:")
    print("  - amx version: ktpamx version 2.7.17")
    print("  - amx plugins: KTPMatchHandler.amxx ... debug ... 0.10.133")
    print("  - amx modules: KTP DODX (with the score-persistence natives)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
