# Homebridge integration (no Home Assistant, no MQTT)

This panel controls your home through the **Homebridge UI REST API**
(`homebridge-config-ui-x` — the same service you reach at your Homebridge URL).
There's no Home Assistant and no MQTT broker involved. (ESPHome normally talks to
Home Assistant over its native encrypted API; Homebridge has no such link, so this
build uses the REST API instead.)

With Homebridge's **Accessory Control** ("insecure") mode enabled, that API can
read and control **any** accessory Homebridge bridges — Hue, LIFX, Nest, etc. —
regardless of which plugin exposes it. No MQTT broker, no duplicate accessories,
no Home-app automation glue.

```
  ┌──────────────┐  HTTP (LAN)     ┌────────────────────────┐   plugins   ┌───────────┐
  │ CYD panel    │ ──────────────► │ Homebridge UI REST API │ ──────────► │ Hue/LIFX/ │
  │ (ESPHome)    │  GET  state     │ /api/accessories/{id}  │             │ Nest/...  │
  │              │ ◄────────────── │  (insecure mode on)    │ ◄────────── │           │
  └──────────────┘  PUT  commands  └────────────────────────┘   state     └───────────┘
```

The panel is **configured entirely from a web page it serves** — you flash the
firmware once, then set your Homebridge URL, login, room name and tiles at
`http://<panel-ip>/setup`. Nothing device-specific lives in the YAML, so the same
firmware works for any room or set of accessories, and the repo configs contain
only placeholders.

> ⚠️ **Security:** Accessory control requires Homebridge **insecure mode**.
> Anyone with network access to the Homebridge port + PIN can control your
> accessories. Keep this on a trusted LAN. Do **not** expose the Homebridge UI
> to the internet without a reverse proxy you trust, and prefer the panel to
> reach Homebridge over the **LAN address**, not a public hostname.

---

## Quick start

### 1. Enable Accessory Control in Homebridge

Homebridge UI → top-right menu → **Settings** → **Homebridge Settings → Enable
accessory control (insecure mode)** → on, then restart Homebridge. (Equivalent to
starting Homebridge with the `-I` flag.) Without this, `/api/accessories` returns
nothing to control.

### 2. Flash the panel (once)

The easy way — **[flash.wrkintegrated.com](https://flash.wrkintegrated.com)** — needs
no ESPHome, no compiling, and no secrets. Plug the display into a desktop running
Chrome / Edge / Opera, click **Install**, and enter your Wi-Fi when prompted. The
firmware is secret-free: Wi-Fi is provisioned in the browser (Improv), nothing
sensitive is baked in, and your Homebridge details are entered later on the panel's
own `/setup` page.

> **Display controller:** most CYDs are **ST7789V** (the default build). Some are
> **ILI9341** — the flasher offers both, and the panel's first-boot **self-test
> screen** (red/green/blue bars + build name) tells you if you picked the wrong one.
> If it looks garbled or the colors are wrong, just re-flash the other build.

<details><summary><b>Advanced: build and flash it yourself</b></summary>

You don't need this, but if you want to change compile-time defaults (pins,
orientation, tile defaults, display model):

```bash
pip install esphome
esphome run esphome/homebridge/cyd-2432s028/home-like.yaml          # pick your serial port
# ILI9341 variant:
esphome -s DISPLAY_MODEL ILI9341 -s INVERT_COLORS true run esphome/homebridge/cyd-2432s028/home-like.yaml
```

No `secrets.yaml` is required — Wi-Fi is set at flash time (Improv) or via the
captive-portal fallback AP `SmartDisplay Setup`. (The only file that still uses
secrets is the optional `link-test.yaml` validator described below.)

</details>

### 3. Configure at `http://<panel-ip>/setup`

The panel's IP is printed in the ESPHome logs on boot (and in your router). Open
`http://<panel-ip>/setup` from any phone or computer on the same network:

1. Enter your **Homebridge URL** (LAN address, e.g. `http://192.168.1.50:8581`),
   **username**, **password**, and a **room name**, then tap **Save**.
2. Tap **Load devices from Homebridge**. The panel pulls your accessory list and
   fills a dropdown for each tile.
3. For each tile: pick a **device**, set a **title** and **type**
   (light / switch / fan / cover / climate / scene / script), and tick
   **Show this tile**.
4. Tap **Save**. Changes apply **live** — the panel updates within about a second,
   no reflash and no reboot.

That's it. The config (URL, credentials, room, tiles) is stored in the panel's
flash and survives reboots.

> **Large Homebridge installs:** the panel fetches the lightweight
> `/api/accessories/layout` list, but a no-PSRAM ESP32 has limited RAM. If you run
> **many** accessories, the dropdown may show only a subset — any device that
> isn't listed can still be added by pasting its `uniqueId` under a tile's
> **Advanced · uniqueId** expander (see *Finding a uniqueId* below).

---

## Finding a `uniqueId` manually

Every accessory is addressed by a stable `uniqueId` (a hash), not by name. The
device picker resolves these for you, but for a device that isn't listed:

```bash
# 1) Log in (use your UI credentials)
TOKEN=$(curl -s -X POST http://192.168.1.50:8581/api/auth/login \
  -H 'Content-Type: application/json' \
  -d '{"username":"admin","password":"YOUR_PASSWORD"}' | jq -r .access_token)

# 2) List accessories with their uniqueId, type, and name
curl -s http://192.168.1.50:8581/api/accessories \
  -H "Authorization: Bearer $TOKEN" \
  | jq '.[] | {uniqueId, type, name: .serviceName}'
```

Paste the `uniqueId` into the tile's **Advanced · uniqueId** field on `/setup`.
You can also browse the API interactively at `http://<homebridge-ip>:8581/swagger`.

---

## Optional: validate the raw link first

If nothing works and you want to isolate the problem, flash the tiny
**[homebridge/link-test.yaml](homebridge/link-test.yaml)** config. It logs in,
reads one accessory and toggles it, showing the result on the onboard RGB LED and
in the ESPHome logs — proving the ESP32 ↔ Homebridge round-trip on your hardware
before you worry about the UI. Set `HB_BASE_URL` + `HB_TEST_ACCESSORY_ID` in that
file and `hb_username`/`hb_password` in `secrets.yaml`.

| LED   | Meaning                                              |
|-------|------------------------------------------------------|
| RED   | No token yet, or the last request failed (see logs)  |
| BLUE  | Logged in; the test accessory reports **OFF**        |
| GREEN | Logged in; the test accessory reports **ON**         |

Common failures: `login FAILED (http 401)` = wrong credentials; `http 404` on a
read = wrong `uniqueId` or insecure mode off; everything times out = wrong URL,
firewall, or (for `https://`) needs `verify_ssl: false`.

---

## Authentication options

- **Form login (default):** the panel logs in with your UI credentials and
  refreshes the token every 6h. Robust and recommended.
- **No auth:** if you disable Homebridge UI authentication (UI → Settings →
  *Authentication: None*), the API is reachable without a token. Simpler, but only
  acceptable on a fully trusted LAN — combined with insecure mode, anyone on the
  network can control your home.

---

## How it maps to HomeKit

The panel translates each tile action/state to HomeKit characteristics via the API:

- **Control (out):** `PUT /api/accessories/{uniqueId}` with
  `{"characteristicType":"On","value":true}`
- **State (in):** `GET /api/accessories/{uniqueId}` → a `values` map, polled every
  few seconds
- **Auth:** `POST /api/auth/login` → a bearer token used on every request

| Tile type | HomeKit characteristic(s)                                                             |
|-----------|---------------------------------------------------------------------------------------|
| light     | `On` (bool); `Brightness` **0-100**                                                    |
| light color | `Hue` 0-360 + `Saturation` 0-100; `ColorTemperature` (mireds)                        |
| switch / outlet | `On` (bool)                                                                     |
| scene / script | usually a stateless `On` switch accessory                                        |
| cover     | `TargetPosition` / `CurrentPosition` **0-100** (100 = open)                            |
| fan       | `On` / `Active`; `RotationSpeed` 0-100 (HomeKit has no fan "preset")                   |
| climate   | `TargetTemperature` / `CurrentTemperature`; `TargetHeatingCoolingState` 0=off 1=heat 2=cool 3=auto |

Note: HomeKit light **brightness is 0-100** (HA used 0-255); the poll engine scales
it so the existing UI math keeps working.

---

## Required ESPHome `http_request` settings (validated on hardware)

Talking to the Homebridge UI API from an ESP32 needs several non-obvious settings.
Confirmed on a live CYD + Homebridge (2026-07-11); already baked into
[hb_engine.yaml](homebridge/cyd-2432s028/hb_engine.yaml) and
[link-test.yaml](homebridge/link-test.yaml):

- **Accept any 2xx, not just 200.** `POST /api/auth/login` returns **201 Created**.
- **Send `Content-Type: application/json` on the login POST** — ESPHome's `json:`
  action doesn't, and Homebridge rejects the body with **415** without it.
- **Enlarge both HTTP buffers on `http_request:`** — `buffer_size_tx: 4096` (the
  `Authorization: Bearer <token>` header is ~360 B, over the default 512 B) and
  `buffer_size_rx: 4096` (response headers total ~900 B).
- **Raise `max_response_buffer_size` per parsed GET** (default is 1 kB, which
  truncates the accessory JSON). It's an *action* option next to `capture_response`.
  Note it allocates a scratch buffer **plus** the response string at once (~2× the
  size), so on a no-PSRAM ESP32 keep it modest — the device list fetch is capped to
  stay within contiguous RAM.
- **Header-value lambdas must return `const char*`** (keep the bearer string in a
  persistent global and return `.c_str()`), and `Accept-Encoding: identity` on GETs
  guards against compressed replies the device can't inflate.

---

## What's in the repo

- **[homebridge/cyd-2432s028/home-like.yaml](homebridge/cyd-2432s028/home-like.yaml)**
  — the panel firmware (LVGL home-like UI). Flash this.
- **[homebridge/cyd-2432s028/hb_engine.yaml](homebridge/cyd-2432s028/hb_engine.yaml)**
  — the Homebridge engine (login, polling, commands, device-list fetch), included
  as a package. Reads everything from the on-device config at runtime.
- **[homebridge/cyd-2432s028/components/hb_config/](homebridge/cyd-2432s028/components/hb_config/)**
  — the custom component that serves `/setup` + `/hbdevices` and stores the config
  in flash.
- **[homebridge/link-test.yaml](homebridge/link-test.yaml)** — the optional
  link validator above.

## Roadmap

- [x] **link-test.yaml** — validates login + control + state read (confirmed on hardware)
- [x] Full **home-like** tile UI on the CYD, driven by the Homebridge API (landscape,
      live state, tap-to-toggle, brightness/color)
- [x] **On-device web config** — set URL/credentials/room + a device picker at
      `/setup`, stored in flash, applied live. One firmware for any room/devices.
- [x] **Browser flasher** (ESP Web Tools) with in-browser Wi-Fi (Improv) and a
      secret-free factory image — no ESPHome install, no compiling
- [x] **ST7789V + ILI9341 builds** with a first-boot display self-test
- [ ] External ILI9341 + ESP32 (Homebridge) variant
- [ ] Streaming device-list fetch (list *all* accessories on very large installs)

---

## Sources

- [Homebridge UI — API Reference](https://github.com/homebridge/homebridge-config-ui-x/wiki/API-Reference)
- [Homebridge UI — Enabling Accessory Control](https://github.com/homebridge/homebridge-config-ui-x/wiki/Enabling-Accessory-Control)
- [ESPHome — HTTP Request component](https://esphome.io/components/http_request.html)
- [ESPHome — JSON parsing](https://esphome.io/components/json.html)
