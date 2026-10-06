# XMC1100 device headers + CMSIS (vendored)

Vendored (trimmed) subset of the official **Infineon XMC1000_DFP v2.12.0** and
**Arm CMSIS v6.3.0** packs, copied from:

```
D:/Arm/Packs/Infineon/XMC1000_DFP/2.12.0/Device/XMC1100_series/Include
D:/Arm/Packs/ARM/CMSIS/6.3.0/CMSIS/Core/Include
```

Only what the projects in this repo compile against is included (~723 KB of the
136 MB full pack):

| Path                                                     | What it is                                       | License        |
| -------------------------------------------------------- | ------------------------------------------------ | -------------- |
| `CMSIS/Include/Device/Infineon/XMC1100_series/Include`   | XMC1100 device headers (`XMC1100.h`, `system_XMC1100.h`, `XMC1000_RomFunctionTable.h`) | Boost-1.0 (Infineon) |
| `CMSIS/Include/core_cm0.h`, `core_cm0plus.h`, `tz_context.h` | CMSIS 6 core headers (Cortex-M0/M0+)       | Apache-2.0 (Arm) |
| `CMSIS/Include/cmsis_compiler.h`, `cmsis_version.h`      | CMSIS compiler dispatch + version              | Apache-2.0 (Arm) |
| `CMSIS/Include/cmsis_gcc.h`, `cmsis_armclang.h`, `cmsis_clang.h` | CMSIS compiler-abstraction headers (front ends) | Apache-2.0 (Arm) |
| `CMSIS/Include/m-profile/cmsis_*_m.h`                   | M-profile halves of the above (M0/M0+/M3/M4/M7/M33) | Apache-2.0 (Arm) |

Not vendored: the DFP's XMClib driver sources/headers, RTE `Config` directory,
SVD files, Flash algorithms, board support packages, pack documentation (81 MB of
PDFs) and examples.

`XMC1000_RomFunctionTable.h` is not compiled by the current projects; it is kept
because the pack's XMClib `RTE_Device.h` configs include it.

To build against the **full** official packs (e.g. to pull in XMClib) instead,
configure with the pack roots — see the root `README.md`:

```bash
cmake -G Ninja -DXMC_DFP_ROOT="D:/Arm/Packs/Infineon/XMC1000_DFP/2.12.0" \
                   -DCMSIS_ROOT="D:/Arm/Packs/ARM/CMSIS/6.3.0" ..
```

Packs are installed with Keil MDK's *Pack Installer* (`D:\Arm\Packs`) or the
`cpackget` CLI (`cpackget update index; cpackget install Infineon::XMC1000_DFP`).

## Licensing

Both licenses are permissive and allow redistribution with the notice kept
intact — every vendored file still carries its original copyright and license
header:

* `Device/Infineon/...` — Copyright (c) 2012-2020 Infineon Technologies AG,
  **Boost Software License 1.0**.
* CMSIS core/compiler headers — Copyright (c) 2009-2024 Arm Limited,
  **Apache-2.0** (`SPDX-License-Identifier: Apache-2.0`).

The DFP pack's own `License/CMSIS_END_USER_LICENSE_AGREEMENT.txt` covers the CMSIS
pack install, not the Boost-1.0 Infineon device headers; it is **not** vendored
here and is not required for the files above.