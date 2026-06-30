# prboom-tdeck

Doom for the LilyGo T-Deck Plus, built on [PrBoom](https://prboom.sourceforge.net/) via Espressif's [esp32-doom](https://github.com/espressif/esp32-doom) port.

The shareware WAD (`doom1-cut.wad`) is embedded directly in the firmware binary — no separate flash step, no SD card. Switch firmwares with the [bmorcelli M5Launcher](https://github.com/bmorcelli/M5Stick-Launcher) as normal.

**No sound.** The T-Deck Plus MAX98357A amplifier is not wired up; audio is stubbed out.

---

## Hardware

- LilyGo T-Deck Plus (ESP32-S3, ST7789 320×240, BBQ10 keyboard, optical trackball, 16 MB flash, 8 MB OPI PSRAM)

---

## Controls

| Key | Action |
|-----|--------|
| W / S | Move forward / backward |
| A / D | Strafe left / right |
| Space | Use / open door |
| P | Fire |
| Trackball roll | Move / turn |
| Trackball press | Fire |
| Enter | Menu confirm |
| Esc / Backspace / U | Menu / back |
| M | Automap |
| $ | Cycle weapon |
| 1 – 7 | Select weapon slot |

---

## WAD files

Only the **shareware** episode (`doom1-cut.wad`, trimmed by Espressif) is bundled. The full commercial DOOM1.WAD or DOOM.WAD is not supported by this build; the embedded filename is hard-coded to `DOOM1.WAD`.

---

## Building

### Prerequisites

- [PlatformIO](https://platformio.org/) (CLI or IDE)
- The shareware WAD must be present at the repo root as `doom1-cut.wad`:

```sh
curl -L -o doom1-cut.wad https://dl.espressif.com/dl/doom1-cut.wad
```

### Build

```sh
pio run -e t-deck
```

Output: `.pio/build/t-deck/firmware.bin`

### Flash (OTA slot only — for use with M5Launcher)

```sh
pio run -e t-deck -t upload
```

Or manually:

```sh
esptool.py --chip esp32s3 --port /dev/ttyUSB0 \
  write_flash 0x10000 .pio/build/t-deck/firmware.bin
```

### First-time / merged flash

```sh
esptool.py --chip esp32s3 --port /dev/ttyUSB0 \
  write_flash 0x0 prboom-tdeck-merged.bin
```

---

## Partition layout

| Partition | Offset | Size | Notes |
|-----------|--------|------|-------|
| nvs | 0x9000 | 16 KB | Shared NVS — launcher and other firmwares are safe |
| otadata | 0xD000 | 8 KB | OTA boot record |
| phy_init | 0xF000 | 4 KB | RF calibration |
| ota_0 | 0x10000 | 7 MB | Primary app slot (firmware + embedded WAD) |
| ota_1 | 0x710000 | 7 MB | Secondary OTA slot |
| coredump | 0xE10000 | 2 MB | Crash dump |

NVS, otadata and phy_init are at the same offsets the bmorcelli launcher protects — flashing this firmware will never corrupt NVS data from other apps.

---

## Credits

- [PrBoom](https://prboom.sourceforge.net/) — Colin Phipps, Florian Schulze et al., GNU GPL v2
- [esp32-doom](https://github.com/espressif/esp32-doom) — Espressif Systems, Apache 2.0
- [openmeshos-868](https://github.com/OpenMeshOS/openmeshos-868) — hardware drivers and board shims (Board.cpp, Display.cpp)
- id Software — DOOM engine, shareware WAD
