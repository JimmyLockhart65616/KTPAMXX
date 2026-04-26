/**
 * ktp_version_reporter — shared KTP plugin version registry + rcon command.
 *
 * Each KTP plugin should `#include <ktp_version_reporter>` and call
 * `KTP_RegisterVersion(PLUGIN_NAME, PLUGIN_VERSION)` from `plugin_init()`.
 *
 * The first plugin in plugins.ini load order to call this registers the
 * `amx_ktp_versions` rcon command (ADMIN_RCON). When invoked, the command
 * prints a table of every KTP plugin loaded with version + git SHA +
 * build time.
 *
 * Storage uses engine localinfo, which persists across map changes but
 * clears on server restart — so the registry is fresh each restart.
 *
 * Build-time SHA + time are injected via a generated `build_info.inc` in
 * each plugin's compile.sh (see compile.sh template below). If absent,
 * "unknown" is used as a graceful fallback so the include works even
 * outside the canonical toolchain.
 *
 * Example compile.sh additions to write build_info.inc into the temp
 * build dir before invoking amxxpc:
 *
 *   GIT_SHA=$(git -C "$SCRIPT_DIR" rev-parse --short HEAD 2>/dev/null || echo "unknown")
 *   BUILD_TIME=$(date -u +%Y-%m-%dT%H:%MZ)
 *   cat > "$TEMP_BUILD/build_info.inc" <<EOF
 *   #define KTP_BUILD_SHA "$GIT_SHA"
 *   #define KTP_BUILD_TIME "$BUILD_TIME"
 *   EOF
 *
 * Cross-plugin coordination notes:
 * - All plugins call `register_concmd` for the same command name. AMXX
 *   allows this; multiple handlers fire in plugins.ini load order. The
 *   first handler returns PLUGIN_HANDLED to short-circuit redundant
 *   prints from the rest.
 * - The localinfo flag `_ktp_v_reg` distinguishes "first call this run"
 *   from "subsequent call this run" so the data accumulator is cleared
 *   exactly once per server start.
 *
 * Rolled out 2026-04-25 as the first concrete deliverable for the test
 * infrastructure plan; also unblocks the Tier 2 deploy-verification
 * integration in `KTPInfrastructure/TEST_INFRASTRUCTURE_PLAN.md`.
 */

#if defined _ktp_version_reporter_included
    #endinput
#endif
#define _ktp_version_reporter_included

#include <amxmodx>

#tryinclude "build_info"
#if !defined KTP_BUILD_SHA
    #define KTP_BUILD_SHA "unknown"
#endif
#if !defined KTP_BUILD_TIME
    #define KTP_BUILD_TIME "unknown"
#endif

// Localinfo keys (engine-shared state)
#define _KTP_VR_KEY_DATA "_ktp_v_data"
#define _KTP_VR_KEY_FLAG "_ktp_v_reg"

// Tunables
#define _KTP_VR_DATA_MAX 2048   // ~25 plugins worth
#define _KTP_VR_LINE_MAX 256
#define _KTP_VR_NAME_MAX 64
#define _KTP_VR_VER_MAX  32
#define _KTP_VR_SHA_MAX  16
#define _KTP_VR_TIME_MAX 32

stock KTP_RegisterVersion(const plName[], const plVersion[]) {
    new flag[8];
    get_localinfo(_KTP_VR_KEY_FLAG, flag, charsmax(flag));

    if (flag[0] == 0) {
        // First registration in this server run — reset accumulator + register
        // the rcon command. Subsequent plugins skip this branch.
        set_localinfo(_KTP_VR_KEY_DATA, "");
        set_localinfo(_KTP_VR_KEY_FLAG, "1");
        register_concmd(
            "amx_ktp_versions",
            "_ktp_v_dump_handler",
            ADMIN_RCON,
            "List loaded KTP plugin versions (name, version, git SHA, build time)"
        );
    }

    // Append our line to the registry. Format: name|version|sha|time;
    new line[_KTP_VR_LINE_MAX];
    formatex(line, charsmax(line), "%s|%s|%s|%s;",
             plName, plVersion, KTP_BUILD_SHA, KTP_BUILD_TIME);

    new data[_KTP_VR_DATA_MAX];
    get_localinfo(_KTP_VR_KEY_DATA, data, charsmax(data));
    add(data, charsmax(data), line);
    set_localinfo(_KTP_VR_KEY_DATA, data);
}

public _ktp_v_dump_handler(id, level, cid) {
    if (!cmd_access(id, level, cid, 1)) return PLUGIN_HANDLED;

    new data[_KTP_VR_DATA_MAX];
    get_localinfo(_KTP_VR_KEY_DATA, data, charsmax(data));

    console_print(id, "^nKTP plugin versions:");
    console_print(id, "  %-32s %-14s %-10s %s",
                  "Name", "Version", "SHA", "Build Time (UTC)");
    console_print(id, "  %-32s %-14s %-10s %s",
                  "--------------------------------",
                  "--------------",
                  "----------",
                  "------------------");

    new count = 0;
    new line[_KTP_VR_LINE_MAX];
    new rest[_KTP_VR_DATA_MAX];
    copy(rest, charsmax(rest), data);

    while (rest[0] != 0) {
        // Split off next entry by ';'
        new tail[_KTP_VR_DATA_MAX];
        strtok(rest, line, charsmax(line), tail, charsmax(tail), ';', false);
        copy(rest, charsmax(rest), tail);
        if (line[0] == 0) continue;

        // Split entry by '|' into 4 fields
        new name[_KTP_VR_NAME_MAX];
        new ver[_KTP_VR_VER_MAX];
        new sha[_KTP_VR_SHA_MAX];
        new buildTime[_KTP_VR_TIME_MAX];
        new lineRest[_KTP_VR_LINE_MAX];

        strtok(line,     name, charsmax(name), lineRest, charsmax(lineRest), '|', false);
        strtok(lineRest, ver,  charsmax(ver),  lineRest, charsmax(lineRest), '|', false);
        strtok(lineRest, sha,  charsmax(sha),  lineRest, charsmax(lineRest), '|', false);
        copy(buildTime, charsmax(buildTime), lineRest);

        console_print(id, "  %-32s %-14s %-10s %s",
                      name, ver, sha, buildTime);
        count++;
    }

    console_print(id, "^n  Total: %d KTP plugin(s) loaded", count);
    return PLUGIN_HANDLED;
}
