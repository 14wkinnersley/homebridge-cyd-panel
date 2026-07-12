# Homebridge CYD Panel — a Cheap Yellow Display touch panel for Homebridge

![Homebridge](https://img.shields.io/badge/Homebridge-Panel-blue)
![ESPHome](https://img.shields.io/badge/ESPHome-Compatible-blue)
![LVGL](https://img.shields.io/badge/LVGL-UI-green)
![Cheap Yellow Display](https://img.shields.io/badge/CYD-ST7789%20%2F%20ILI9341-yellow)
![Flash in browser](https://img.shields.io/badge/Flash-in%20your%20browser-brightgreen)

Turn a **Cheap Yellow Display** (ESP32-2432S028) into a slick **Homebridge** wall
panel — an iOS-Home-app-style grid of tiles that control your lights, switches,
fans, covers, scenes and thermostats. It talks to Homebridge over the Homebridge
UI REST API, so **no Home Assistant and no MQTT** are involved.

The whole point of this fork is that it's **stupid easy**:

### 👉 [Flash it in your browser at flash.wrkintegrated.com](https://flash.wrkintegrated.com)

No ESPHome, no compiling, no YAML, no `secrets.yaml`. Plug in the display, click
**Install**, type your Wi-Fi, done.

> **Prefer Home Assistant?** This fork is Homebridge-only. For the original
> Home-Assistant version (and the standalone ILI9341 + external-ESP32 wiring
> variant), use the upstream project it's based on:
> **[akuehlewind/ESPHome-touch-display-mount](https://github.com/akuehlewind/ESPHome-touch-display-mount)**.

<img src="images/display-home-like.png" width="49%"> <img src="images/display-home-like-overlay.png" width="49%">

---

## Flash it (about 3 minutes)

1. Get a **Cheap Yellow Display** (ESP32-2432S028) and a USB cable that carries data.
2. On a **desktop computer** running **Chrome, Edge, or Opera**, open
   **[flash.wrkintegrated.com](https://flash.wrkintegrated.com)**.
3. Plug the display into USB, click **Install**, and pick the serial port
   (often shown as `CP210x`, `CH340`, or `USB Serial`).
4. When flashing finishes, the browser asks for your **Wi-Fi name and password** —
   enter them and the panel joins your network.
5. Click **Visit Device** to open `http://<panel-ip>/setup` and point it at Homebridge.

> ⚠️ **Browser flashing works on desktop Chrome / Edge / Opera only.** It uses
> Web Serial, which isn't available in Safari, Firefox, or on phones. (Once
> flashed, you configure and use the panel from any browser, including your phone.)

### Which display do I have? (ST7789 vs ILI9341)

Most CYDs use the **ST7789** controller — that's the default build, so start there.
The display controller **can't be detected before flashing**, so instead the panel
runs a quick **self-test on first boot**: it shows **red / green / blue bars** and
the build name. If those bars look torn, shifted, or the colors are wrong, your
board is the other type — just go back to the flasher and install the **ILI9341**
build. Re-flashing is safe and takes about a minute.

---

## Configure it (`http://<panel-ip>/setup`)

Open `http://<panel-ip>/setup` from any phone or computer on the same network:

1. Enter your **Homebridge URL** (LAN address, e.g. `http://192.168.1.50:8581`),
   **username**, **password**, and a **room name**. Tap **Save**.
2. Tap **Load devices from Homebridge** — the panel pulls your accessory list.
3. For each of the six tiles: pick a **device**, set a **title** and **type**
   (light / switch / fan / cover / climate / scene / script), tick **Show this tile**.
4. Tap **Save**. Changes apply **live** — no reflash, no reboot.

The config is stored in the panel's flash and survives reboots. The same firmware
works for any room or set of accessories.

---

## Requirements

- A **Cheap Yellow Display** (ESP32-2432S028) + a data USB cable.
- A **desktop with Chrome / Edge / Opera** for the one-time flash.
- **Homebridge** on your network with **Accessory Control ("insecure mode")** enabled:
  Homebridge UI → menu → **Settings → Homebridge Settings → Enable accessory control**
  → on, then restart Homebridge. Without it, `/api/accessories` has nothing to control.

> ⚠️ **Security:** Accessory control means anyone on your network (with the port +
> PIN) can control your accessories. Keep the panel and Homebridge on a **trusted
> LAN**, and don't expose the Homebridge UI to the internet. See
> [`esphome/HOMEBRIDGE.md`](esphome/HOMEBRIDGE.md) for details.

For everything about how the panel talks to Homebridge — the REST API, finding a
device's `uniqueId` manually, the HomeKit characteristic mapping, and the advanced
"build it yourself" path — see **[esphome/HOMEBRIDGE.md](esphome/HOMEBRIDGE.md)**.

---

## 📦 Hardware

### ESP32-2432S028 (Cheap Yellow Display / CYD)

| Part                          | Price | Comment                                          |
|-------------------------------|-------|--------------------------------------------------|
| ESP32-2432S028                | ~$15  | 2.8" 240×320 touch display with integrated ESP32 |
| 2× M4 3.5×16 screw (flathead) | ~$0.10| Connect the mount case to the base               |

The board integrates the ESP32, the SPI display (ST7789V or ILI9341 depending on
the batch), the XPT2046 touch controller, and the backlight on one board — a single
USB cable, no wiring. It's widely sold as the "Cheap Yellow Display."

Pinout (for reference — you don't need to wire anything):

| Function      | Pin    | Notes                         |
|---------------|--------|-------------------------------|
| LCD clk       | GPIO14 | SPI display clock             |
| LCD mosi      | GPIO13 | SPI display MOSI              |
| LCD cs        | GPIO15 | Display chip-select           |
| LCD dc        | GPIO2  | Display data/command          |
| Touch clk     | GPIO25 | SPI touch clock               |
| Touch mosi    | GPIO32 | SPI touch MOSI                |
| Touch miso    | GPIO39 | SPI touch MISO                |
| Touch cs      | GPIO33 | Touch chip-select             |
| Touch irq     | GPIO36 | Touch interrupt               |
| Backlight     | GPIO21 | LEDC PWM                      |

### Power connection

For the enclosure the side USB connector is **not used**. The included power cable's
**red (5V) and black (GND)** wires can be soldered to an old USB-A cable and routed
through the mount.

⚠️ The CYD has multiple identical JST connectors that do **not** share a pinout. Use
the one labeled **VIN / 5V**. Connecting 5V to a 3.3V connector can damage the board.

<img src="images/power-connect.jpeg" width="40%">

---

## 🖨 3D-printed mounts

See [`3d_print/`](3d_print/) for STL files and Fusion 360 sources. Four mounts share
the same adjustable-tilt enclosure:

- **Desk mount** – tabletop
- **Under-desk mount** – under a desk or shelf
- **Wall mount** – on a wall
- **Flush mount** – in-wall (EU electrical box compatible)

Designed for clean cable routing, a ±35° adjustable viewing angle, and a minimal
footprint. Assembly guide: [`3d_print/ESP32-2432S028/ASSEMBLY.md`](3d_print/ESP32-2432S028/ASSEMBLY.md).

<img src="images/desk-mount2.jpeg" width="32%"> <img src="images/tilt.jpeg" width="32%"> <img src="images/wall-mount.jpeg" width="32%">
<img src="images/wide-body-flush-mount.jpeg" width="32%"> <img src="images/flush-mount.jpeg" width="32%"> <img src="images/side.jpeg" width="32%">

---

## Advanced: build it yourself

You don't need this — the browser flasher is the recommended path — but if you want
to change tile defaults, pins, orientation, or the display model at compile time, you
can build the firmware with standalone ESPHome:

```bash
pip install esphome
esphome run esphome/homebridge/cyd-2432s028/home-like.yaml
```

The firmware is **secret-free** (Wi-Fi is provisioned at flash time or via the
captive-portal fallback AP `SmartDisplay Setup`), so no `secrets.yaml` is required.
To build the ILI9341 variant, override the display model:

```bash
esphome -s DISPLAY_MODEL ILI9341 -s INVERT_COLORS true compile esphome/homebridge/cyd-2432s028/home-like.yaml
```

Repo layout:

- [`esphome/homebridge/cyd-2432s028/home-like.yaml`](esphome/homebridge/cyd-2432s028/home-like.yaml) — the panel firmware (LVGL UI).
- [`esphome/homebridge/cyd-2432s028/hb_engine.yaml`](esphome/homebridge/cyd-2432s028/hb_engine.yaml) — the Homebridge REST engine (login, polling, commands), included as a package.
- [`esphome/homebridge/cyd-2432s028/components/hb_config/`](esphome/homebridge/cyd-2432s028/components/hb_config/) — the custom component that serves `/setup` and stores config in flash.
- [`site/`](site/) — the browser flasher (ESP Web Tools), deployed to Cloudflare Pages.
- [`esphome/HOMEBRIDGE.md`](esphome/HOMEBRIDGE.md) — full Homebridge reference + the optional `link-test.yaml` validator.

---

## Troubleshooting

**Garbled / corrupted display, or wrong colors.** You flashed the wrong controller
build. Re-flash the **other** variant from the flasher (ST7789 ↔ ILI9341). The
first-boot R/G/B self-test makes this obvious.

**Touch is offset or mirrored.** Touch calibration is device-specific. The
`TOUCH_*` and `TOUCH_CAL_*` substitutions at the top of `home-like.yaml` can be
tuned; see the ORIENTATION presets. (This requires the advanced build path.)

**"Load devices" times out on `/setup`.** Save your Homebridge URL + login first,
confirm Accessory Control is on in Homebridge, then load again. Very large
Homebridge installs may list only a subset — paste a device's `uniqueId` under a
tile's **Advanced · uniqueId** field (see [`HOMEBRIDGE.md`](esphome/HOMEBRIDGE.md)).

**The flasher button does nothing.** Use desktop Chrome / Edge / Opera over HTTPS.
Safari, Firefox, and phones don't support Web Serial.

---

## Credits & license

This is a Homebridge-focused fork of
**[akuehlewind/ESPHome-touch-display-mount](https://github.com/akuehlewind/ESPHome-touch-display-mount)**
by Adrian Kuehlewind — the original ESPHome/LVGL panel, the 3D-printed enclosures,
and the Home Assistant build all come from there. Huge thanks to that project and
its contributors.

Licensed under the [MIT License](LICENSE). Material Design Icons font is Apache 2.0
([Templarian/MaterialDesign-Webfont](https://github.com/Templarian/MaterialDesign-Webfont)).

This is a hobby project and a work in progress — fork it, remix it, improve it.
