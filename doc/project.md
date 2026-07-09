# family-feud-podium

## Epics
<!-- AUTO-MAINTAINED — do not edit manually, ask Wren to update -->
- [family-feud](../../Epics/family-feud.md)
<!-- END AUTO-MAINTAINED -->

## Links
- **GitHub:** https://github.com/karlman/family-feud-podium

## Overview
Arduino firmware for two custom physical Family Feud podiums. Each podium has NeoPixel LED strips, MOSFET-controlled accent lighting, and a buzz-in button. Communicates with the game server (`family-feud-game`) via USB serial.

## Hardware
- **Board:** Arduino Uno R4 Minima
- **LEDs:** Adafruit NeoPixel strips (one per podium, 6-column serpentine matrix)
- **Lighting:** MOSFET-controlled top, front, and button illumination per podium
- **Input:** Tactile buttons (one per podium) with INPUT_PULLUP

## Platform
- Built with **PlatformIO** (`renesas-ra` platform)
- Framework: Arduino
- Library: `adafruit/Adafruit NeoPixel`

## Serial Protocol
Commands sent from the Pi game server → Arduino:

| Command | Effect |
|---------|--------|
| `RESET` | Clear state, show royal blue on both strips |
| `BUZZIN` | Enable buzz-in, pulsing blue animation |
| `ACTIVE:1` / `ACTIVE:2` | Illuminate winning podium white, dim the other |
| `STRIKE:N` | Flash red X (N = number of strikes), then reset |
| `WIN:1` / `WIN:2` | Rainbow animation on winning podium |
| `CLEAR` | Turn everything off |

Arduino replies to the Pi:

| Response | Meaning |
|----------|---------|
| `RINGER:1` / `RINGER:2` | Podium 1 or 2 button pressed during buzz-in |
| `READY` | Startup complete |
| `DEBUG:BTN*_NO_BUZZIN` | Button pressed when buzz-in not active |

## Animations
- **Buzz-in:** Pulsing blue on both strips
- **Active:** Winning strip white (or strikes drawn), losing strip dim blue
- **Strike:** Bold red X pattern, 3-row serpentine, up to 3 strikes
- **Win:** Rainbow cycle on winning strip, other strip off
- **Startup:** Double blink + color cycle through orange/green/purple/blue/white

## Key Files
- `src/main.cpp` — Main loop, command processor, animations
- `include/podium.h` — Pin definitions, constants, game state enum
- `include/test_modes.h` — Test mode helpers
- `platformio.ini` — Build config (uploads to COM6)

## Related Repos
- `family-feud-game` — Game server that sends commands and receives buzz-in events
