# Homebridge integration (no Home Assistant, no MQTT)

This project was originally built for **Home Assistant**, which ESPHome talks to
natively over its encrypted API. Homebridge has no such native ESPHome link, so
the panel talks to Homebridge a different way: the **Homebridge UI REST API**
(`homebridge-config-ui-x` — the same service you reach at your Homebridge URL).

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

- **Control (out):** `PUT /api/accessories/{uniqueId}` with
  `{"characteristicType":"On","value":true}`
- **State (in):** `GET /api/accessories/{uniqueId}` → a `values` map of the
  accessory's HomeKit characteristics, polled on an interval
- **Auth:** `POST /api/auth/login` → a bearer token used on every request

> ⚠️ **Security:** Accessory control requires Homebridge **insecure mode**.
> Anyone with network access to the Homebridge port + PIN can control your
> accessories. Keep this on a trusted LAN. Do **not** expose the Homebridge UI
> to the internet without a reverse proxy you trust, and prefer the panel to
> reach Homebridge over the **LAN address**, not a public hostname.

---

## Start here: validate the link before porting the UI

Porting the full tile UI is a large change, and it can't be flash-tested for
you. So the first step is a tiny, self-contained config that proves the ESP32 ↔
Homebridge round-trip on your hardware and network:

**[homebridge/link-test.yaml](homebridge/link-test.yaml)**

It logs in, reads one accessory, and toggles it — showing the result on the
onboard RGB LED and in the ESPHome logs. Once this works, the full port is
mostly mechanical. Steps below get you there.

---

## Step 1 — Enable Accessory Control in Homebridge

In the Homebridge UI: top-right menu → **Settings** → toggle
**Homebridge Settings → Enable accessory control (insecure mode)** on, then
restart Homebridge. (Equivalent to starting Homebridge with the `-I` flag.)

Without this, `/api/accessories` returns nothing to control.

## Step 2 — Install standalone ESPHome

You don't have Home Assistant, so you won't have the ESPHome add-on. Install
ESPHome on any machine (your choice):

```bash
# pip
pip install esphome

# or Docker
docker run --rm -v "${PWD}:/config" -it ghcr.io/esphome/esphome
```

Then work from the `esphome/` folder of this repo. Copy the example secrets:

```bash
cp secrets.yaml.example secrets.yaml   # then edit secrets.yaml
```

## Step 3 — Get a token and find your accessories' `uniqueId`

Every accessory is addressed by a stable `uniqueId` (a hash), **not** by name.
Grab a token, then list accessories:

```bash
# 1) Log in (default creds are admin/admin — use yours)
TOKEN=$(curl -s -X POST http://192.168.1.50:8581/api/auth/login \
  -H 'Content-Type: application/json' \
  -d '{"username":"admin","password":"YOUR_PASSWORD"}' | jq -r .access_token)

# 2) List accessories with their uniqueId, type, and current values
curl -s http://192.168.1.50:8581/api/accessories \
  -H "Authorization: Bearer $TOKEN" \
  | jq '.[] | {uniqueId, type, name: .serviceName, values}'
```

Each entry looks like:

```json
{
  "uniqueId": "e2a3f1c9b8...",
  "type": "Lightbulb",
  "name": "Desk Lamp",
  "values": { "On": 1, "Brightness": 80 }
}
```

Copy the `uniqueId` of a light or switch you can watch into
`HB_TEST_ACCESSORY_ID` in [link-test.yaml](homebridge/link-test.yaml).

> Tip: you can also browse the whole API interactively at
> `http://<homebridge-ip>:8581/swagger`.

## Step 4 — Set your URL and credentials

- **Base URL** → `HB_BASE_URL` substitution at the top of the config.
  Prefer the **LAN** address (`http://192.168.1.50:8581`). The public
  `https://hb.example.com` works too, but TLS is heavy on the ESP32 and a wall
  panel shouldn't depend on your reverse proxy. This value lives only in your
  local copy — leave the committed placeholder as-is.
- **Credentials** → `hb_username` / `hb_password` in `secrets.yaml`
  (gitignored). These are used only to fetch the bearer token.

## Step 5 — Flash and interpret the result

```bash
esphome run homebridge/link-test.yaml
```

Watch the onboard LED and the logs:

| LED    | Meaning                                                            |
|--------|-------------------------------------------------------------------|
| RED    | No token yet, or the last request failed (see logs for HTTP code) |
| BLUE   | Logged in, and the test accessory reports **OFF**                 |
| GREEN  | Logged in, and the test accessory reports **ON**                  |

Then, from the ESPHome dashboard/log view, press **"Homebridge: Toggle test
accessory"** — the real device should switch, and the LED should follow it
within a poll cycle. Toggle the device from the Home app instead, and the LED
should still track it. That's the full read + write loop confirmed.

Common failures:
- `login FAILED (http 401)` — wrong `hb_username`/`hb_password`.
- `poll failed (http 401)` right after login — token/session issue; check the
  Homebridge UI session timeout.
- `poll failed (http 404)` — wrong `HB_TEST_ACCESSORY_ID`, or insecure mode off.
- Everything times out — wrong `HB_BASE_URL`, firewall, or (for `https://`)
  set `verify_ssl: false` under `http_request:`.

---

## Authentication options

- **`HB_AUTH: "form"` (default):** the panel logs in with your UI credentials
  and refreshes the token every 6h (the UI session default is 8h). Robust and
  the recommended choice.
- **`HB_AUTH: "none"`:** if you disable Homebridge UI authentication
  (UI → Settings → *Authentication: None*), the panel calls `/api/auth/noauth`
  instead. Simpler, but only acceptable on a fully trusted LAN — combined with
  insecure mode, anyone on the network can control your home.

---

## Home Assistant → HomeKit characteristic reference

When we port the tiles, HA `entity_id` + service calls become HomeKit
`uniqueId` + characteristics. The mapping:

| Tile type | HA (old)                        | HomeKit characteristic(s) via this API                       |
|-----------|---------------------------------|--------------------------------------------------------------|
| light     | `light.toggle` / brightness 0-255 | `On` (bool); `Brightness` **0-100**                        |
| light color | rgb / color_temp              | `Hue` 0-360 + `Saturation` 0-100; `ColorTemperature` (mireds)|
| switch / outlet | `switch.toggle`           | `On` (bool)                                                  |
| scene / script | `*.turn_on`                | usually a stateless `On` switch accessory                    |
| cover     | `cover.set_cover_position` 0-100  | `TargetPosition` / `CurrentPosition` **0-100** (100 = open) |
| fan       | on/off + percentage / preset      | `On` or `Active`; `RotationSpeed` 0-100 (no "preset" concept)|
| climate   | `climate.set_temperature` / hvac  | `TargetTemperature` / `CurrentTemperature`; `TargetHeatingCoolingState` 0=off 1=heat 2=cool 3=auto |

Two mapping gaps worth knowing up front: HomeKit fans have **no preset_mode**
(the current `fan_toggle_preset` tap becomes a speed set), and light **brightness
is 0-100** here versus HA's 0-255 (the poll engine will scale it so the existing
UI math keeps working).

---

## Roadmap (what's built vs. next)

- [x] `secrets.yaml.example` — Homebridge credentials
- [x] `HB_BASE_URL` substitution pattern (keeps your URL out of the repo)
- [x] **link-test.yaml** — validates login + control + state read
- [ ] Full **home-like** tile UI on the CYD, driven by the Homebridge API
      (login/token layer, per-tile state polling → existing `ui_refresh`,
      tap/long-press → `PUT` characteristics)
- [ ] External ILI9341 + ESP32 variant

Once link-test.yaml is confirmed on your setup, the full UI port is the next
step — the same primitives, wired into the existing LVGL screen.

---

## Sources

- [Homebridge UI — API Reference](https://github.com/homebridge/homebridge-config-ui-x/wiki/API-Reference)
- [Homebridge UI — Enabling Accessory Control](https://github.com/homebridge/homebridge-config-ui-x/wiki/Enabling-Accessory-Control)
- [ESPHome — HTTP Request component](https://esphome.io/components/http_request.html)
- [ESPHome — JSON parsing](https://esphome.io/components/json.html)
