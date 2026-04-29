# Vendored Metamod headers

This directory contains a minimal, build-time-only header subset from
[`alliedmodders/metamod-hl1`](https://github.com/alliedmodders/metamod-hl1)
("metamod-am" in KTP local convention).

## Why these headers are here

KTPAMXX builds against Metamod's public interface (`meta_api.h`, `mreg.h`,
plugin signature structs, etc.) for compile-time symbols, even though the
KTP fleet runs in **extension mode** and Metamod is never loaded at
runtime. Before vendoring (2026-04-29), the build chain checked out
`metamod-hl1` as a sibling repo and pointed AMBuild at it via the
`--metamod` flag or the `METAMOD` env var. That added an external dep,
extra CI checkout time, and recurring confusion ("we don't use Metamod,
why does the build need it?"). Upstream is essentially abandoned (last
meaningful commit ~2018), so vendoring is safe and won't drift.

## What's vendored

Just the transitive header closure from KTPAMXX's two direct includes
(`<meta_api.h>` and `<sdk_util.h>`):

| Header             | Pulled in by                  | Bytes  |
|--------------------|-------------------------------|--------|
| `dllapi.h`         | meta_api.h                    | 11,023 |
| `engine_api.h`     | meta_api.h                    | 26,982 |
| `enginecallbacks.h`| sdk_util.h                    |  3,253 |
| `log_meta.h`       | mutil.h                       |  4,129 |
| `meta_api.h`       | KTPAMXX direct                | 10,676 |
| `meta_eiface.h`    | engine_api.h                  | 24,681 |
| `mhook.h`          | mutil.h                       |  5,682 |
| `mreg.h`           | mutil.h                       |  5,903 |
| `mutil.h`          | meta_api.h                    |  7,178 |
| `osdep.h`          | meta_api.h, sdk_util.h        | 15,860 |
| `plinfo.h`         | meta_api.h                    |  3,259 |
| `sdk_util.h`       | KTPAMXX direct                |  4,039 |
| `types_meta.h`     | meta_eiface.h                 |  3,164 |

13 files, ~126 KB total. The other ~22 headers in upstream
`metamod-hl1/metamod/` are not part of KTPAMXX's compile surface and
were not vendored — adding any of them later requires copying from
upstream and updating the table above.

## License

Metamod is GPLv2-or-later with a special linking exception. See
`LICENSE.txt` in this directory for the full text. Original copyright
notices remain intact at the top of each header file. KTPAMXX itself is
GPLv3 — no license incompatibility (GPLv2-or-later → GPLv3 is fine).

## How the build picks these up

`AMBuildScript` adds `third_party/metamod` to the C++ include path
unconditionally; KTPAMXX `#include <meta_api.h>` resolves here. There
is no longer an `--metamod` configure flag or `METAMOD` env var
recognized by the build; both have been removed.

## Updating from upstream (rare)

If you need a fix from a future upstream release:

1. `git clone https://github.com/alliedmodders/metamod-hl1/tmp/mm`
2. For each file in this directory listed above, diff against
   `/tmp/mm/metamod/<file>`. Apply patches selectively or copy whole.
3. Re-vendor any new transitive deps (run the include-closure walk in
   `KTPInfrastructure` if there's still a script for it; otherwise
   manual). Update the table above.
4. Verify build with `bash build_linux.sh` and Tier 1 smoke.

In practice this hasn't been needed since upstream went dormant.
