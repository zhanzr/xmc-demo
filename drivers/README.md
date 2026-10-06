# Vendored device drivers / CMSIS

Container for the **vendored device headers + CMSIS core headers** used by the
boards in this repo. There is one folder per XMC series, so boards of different
series share a root but never mix headers:

| Folder | Series | Device headers                         | CMSIS core profile | License                    |
| ------ | ------ | -------------------------------------- | ------------------ | -------------------------- |
| [`xmc1/`](xmc1) | XMC1000 | `XMC1100.h`, `system_XMC1100.h`, … | Cortex-M0/M0+      | Boost-1.0 (Infineon), Apache-2.0 (Arm) |

Each series folder documents its own contents, licensing and how to build against
the full official pack instead.

A board's `cmake/<board>.cmake` picks its folder with:

* `-DDRIVERS_ROOT=…` — the repo-root `drivers/` container (defaults to the repo
  root, so no absolute paths are needed),
* `-DXMC_SERIES=xmc1` — which `drivers/<series>/` folder to build against.

Nothing outside its own `drivers/<series>/` folder is used, so adding an XMC4000
or XMC7000 board later only means adding the matching folder and pointing the new
board's CMake at it.