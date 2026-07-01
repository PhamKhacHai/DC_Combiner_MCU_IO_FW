# CURRENT_STATUS

## Firmware status

- `bsp_gpio`, `feedback`, `contactor`, `app`, `can_comm`, and `diagnostic` are implemented.
- Firmware supports:
  - GPIO output control for 6 groups.
  - Feedback debounce.
  - Contactor state machine and safe fault handling.
  - CAN command `0x500 + NodeID`.
  - CAN status `0x510 + NodeID`.
  - Heartbeat `0x520 + NodeID`.
  - Clear Fault through command `Data[1] bit0`.
  - Lost CAN timeout fail-safe.
  - Runtime diagnostic counters through request `0x530 + NodeID` and response `0x540 + NodeID`.

## Diagnostic counter protocol

- Runtime counters are RAM-only.
- Counters reset on MCU reset/power loss.
- Counters are `uint32_t` and saturate at `UINT32_MAX`.
- No Flash/EEPROM persistence has been implemented.
- Diagnostic request does not refresh Lost CAN timeout and does not touch output/contactors.
- Protocol details are documented in `docs/diagnostic_protocol.md`.

## Build status

- Firmware build command: `cmake --build build\Debug`.
- Firmware build result: OK.
- Current memory use from Debug build:
  - RAM: 2136 B / 20 KB.
  - FLASH: 18580 B / 64 KB.

## Test status

- PC test source has been updated for diagnostic counters.
- Object compile checks passed for the changed PC test sources and firmware modules.
- PC test execution is currently blocked by local MSYS2 linker policy:
  - `C:\msys64\ucrt64\bin\ld.exe` is blocked by Windows Application Control.
  - CMake/GCC can compile objects but cannot link test executables in this session.
