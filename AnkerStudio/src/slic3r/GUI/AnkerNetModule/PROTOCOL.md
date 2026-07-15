# AnkerNet Native Protocol Reference

This document describes the network protocol this plugin (`AnkerNetNative`) uses to talk
to eufyMake/AnkerMake M5 printers and Anker's cloud, as reverse-engineered and reimplemented
from scratch for the native arm64 build. It covers four layers:

1. **HTTP API** — login, device list, P2P credentials (Anker's cloud REST API)
2. **MQTT** — live telemetry + remote control (Anker's cloud MQTT broker)
3. **PPPP (LAN)** — direct UDP protocol to the printer for file transfer and camera, on the local network
4. **PPPP (Remote/Relay)** — the same protocol extended to work over the internet via NAT traversal

None of this was obtained from Anker's own source or documentation. It was built by reading
the community reverse-engineering project [libflagship/ankermake-m5-protocol](https://github.com/Ankermgmt/ankermake-m5-protocol)
where it covers the LAN path, and by live packet capture and controlled experimentation
against real hardware (two M5 printers) for everything the community project doesn't cover
(remote/WAN transport, MQTT command semantics, print-state telemetry). Fields explicitly
marked **best guess** below are inferred from behavior, not confirmed against a spec.

Code lives in this directory: `AnkerNetNative.{hpp,cpp}` (orchestration + HTTP + MQTT dispatch),
`AnkerMqtt.{hpp,cpp}` (MQTT client + frame codec), `AnkerPppp.{hpp,cpp}` (PPPP client),
`DeviceObjectNative.hpp` (per-device state + outbound command builders).

---

## 1. HTTP API

Base URL is per-account: the login response's `domain` field (e.g. `make-app.ankermake.com`),
falling back to `make-app.ankermake.com` (or `make-app-eu.ankermake.com` if `ab_code == "EU"`)
if `domain` wasn't provided. All calls are plain HTTPS POST with a JSON body — **no
AES/gtoken encryption at the HTTP layer**, unlike MQTT payloads (see §2).

### Headers (every authenticated call)

```
X-Auth-Token: <auth_token from login>
Content-Type: application/json
App_name: anker_make
Model_type: PC
App_version: 14
Country: US
Language: en
Os_type: Mac
Os_version: 26
```

### Login

Login itself is **not** performed by this plugin — it's done by Anker's real login web
page, loaded in the app's embedded webview (`AnkerWebView`). This plugin only parses the
JSON the page posts back via `window.anker_msg.postMessage(...)`:

```json
{"functionName": "login" | "loginback", "data": { ... }}
```

`data` fields (all percent-decoded except `token_expires_at`):
`auth_token`, `user_id`, `email`, `nick_name`, `avatar`, `ab_code`, `token_expires_at`,
`domain`, and `server_secret_info.public_key`.

The web page also calls `functionName: "getHeaderList"` before every HTTP request it makes
itself, expecting the plugin to answer synchronously (it blocks up to 10s) with the same
header set shown above (plus `Accept`, `Openudid`) — if unanswered, the page's own requests
go out with empty headers and the backend rejects them.

These fields are cached locally to `<app-data-dir>/ankernet_native_login.json` so a relaunch
doesn't require re-login.

### Device list

`POST /v1/app/query_fdm_list`, body `{}`.

Response: `{"code": ..., "msg": ..., "data": [ {device}, ... ]}`. Per-device fields read:
`station_sn`, `station_name`, `station_model`, `ip_addr` (often empty/stale — see §3 LAN
discovery), `secret_key` (hex, the per-device MQTT AES key), `p2p_did` (PPPP device ID),
`p2p_conn` (encoded relay-host list, see §4), `station_id`.

### P2P credentials (dsk keys)

`POST /v1/app/equipment/get_dsk_keys`, body:
```json
{"station_sns": ["SN1", "SN2", ...], "invalid_dsks": {}}
```

Response: `{"code": ..., "data": {"dsk_keys": [ {"station_sn": ..., "dsk_key": ...}, ... ]}}`.

**`dsk_key` is a 20-character RAW string, not hex** — this tripped up an early implementation
attempt (hex-decoding it silently produces an empty/garbage key, which the relay accepts but
treats as unauthenticated). Used only in the remote/relay PPPP path (§5).

### Avatar download

A generic blocking `curl` download to a local file, no auth headers. Used for the user's
profile picture after login.

---

## 2. MQTT (live telemetry + remote control)

### Connection

- Broker: `make-mqtt.ankermake.com` (or `make-mqtt-eu.ankermake.com` for EU accounts), port **8789**, TLS.
- Certificate validation is **disabled** (`SSL_VERIFY_NONE`) — matches the community client's own `insecure` option; the broker's cert apparently isn't validated by any known client.
- MQTT 3.1.1. CONNECT: username = `"eufy_" + user_id`, password = **the user's plain email address** (not the auth token), client ID = `eufypc_<first 8 chars of user_id>_<hex(time^pid)>`, keepalive = 60s, clean session.
- Client sends a PING roughly every 30s to keep the connection alive.

### Topics

Subscribed per device SN:
```
/phone/maker/<sn>/notice          (unsolicited push telemetry)
/phone/maker/<sn>/command/reply   (ack for a command we sent)
/phone/maker/<sn>/query/reply     (ack for a status query)
```

Published:
```
/device/maker/<sn>/command   (control commands)
/device/maker/<sn>/query     (status queries, e.g. {"commandType":1027} on connect for a full snapshot)
```

**Note:** subscribing to the *device-bound* `/device/maker/<sn>/command` topic (to see what
other clients, e.g. the official iOS app, publish) was tried and the SUBSCRIBE succeeds
with no error, but the broker never actually routes other clients' publishes to us there —
so we can only see the *results* of another client's commands (via `/notice`), never the
command itself.

### Frame format ("MA" packet)

Every MQTT publish/notice payload is a binary "MA" frame: a 64-byte header, then an
AES-CBC-encrypted JSON payload, then a single trailing XOR-checksum byte.

Header (64 bytes), all fields fixed/constant except `size`:
| Offset | Size | Value |
|---|---|---|
| 0-1 | 2 | `"MA"` |
| 2-3 | 2 | `size`, u16 little-endian = `64 + len(ciphertext) + 1` |
| 4-8 | 5 | magic bytes `5, 1, 2, 5, 'F'` |
| 9 | 1 | packet type = `0xC0` ("Single") |
| 10-11 | 2 | packet_num, u16 LE, always 0 |
| 12-15 | 4 | time, u32 LE, always 0 |
| 16-52 | 37 | device GUID string (padded/truncated to 37 bytes) |
| 53-63 | 11 | zero padding |

Then the AES-CBC ciphertext, then one checksum byte such that XOR-folding **every byte of
the complete frame** (header + ciphertext + checksum byte itself) equals 0. Decode validates
this, and also checks header offset 6 == `2` (part of the fixed magic).

**AES parameters:** key = the device's `secret_key` (hex-decoded) — AES-128 if 16 bytes,
AES-256 if 32; mode CBC; IV = the fixed ASCII string `"3DPrintAnkerMake"` (16 bytes, used
for both directions); PKCS7 padding.

### Payload shape

Decrypted JSON is one of:
- A single object `{"commandType": N, ...}` (command replies, query replies)
- An array of such objects (unsolicited `/notice` pushes, and full-snapshot query replies)

### QoS is 0 — commands can be silently dropped

Our `publish()` sends a plain PUBLISH with the fixed header byte `0x30` (QoS 0, no DUP/RETAIN)
and no packet identifier — fire-and-forget, no ack, no retransmission at the protocol level.
**This is a real, observed reliability problem**: a Stop click's command was sent 3 times
across ~90 seconds of manual retries before one actually got a reply from the printer. As a
mitigation (not a fix — this is a protocol limitation, not a bug in our client), print-control
commands are sent 3 times over ~800ms from a background thread (`sendGcodeReliably`/
`sendCommandReliably` in `DeviceObjectNative.hpp`) as cheap redundancy.

### Command types (`commandType`)

The interface header's own `aknmt_command_type_e` enum only lists a handful of values
(1003, 1004, 1009, 1010, 1021, 1044, 1135) — it's an incomplete stub. The real, working
set discovered so far:

| commandType | Direction | Meaning | Fields |
|---|---|---|---|
| **1000** | inbound (`/notice`, `/query/reply`) | Device event. Only `subType:1` is meaningful — it's the print-event state machine. `value` maps **directly** onto the `aknmt_print_event_e` enum below. | `subType`, `value` |
| **1001** | inbound | Print job info. Schema changes once a job ends (see below). | `progress` (0–10000 basis points; **-1/absent once a job ends — treat as "unchanged", not 0**), `name` (filename), `filamentUsed`, `filamentUnit`, `time` (remaining seconds, counts down), `totalTime` (elapsed seconds, counts up), `realSpeed`, `task_id` |
| **1003** | both | Nozzle temperature. Telemetry values are **×100** (divide to get °C). | `currentTemp`, `targetTemp` |
| **1004** | both | Bed temperature. Same ×100 scaling. | `currentTemp`, `targetTemp` |
| **1021** | both | Z-offset. **Best guess**: value is mm × 100. | `value` |
| **1026** | inbound only (never confirmed as sendable) | Unknown — appeared twice correlating with a print being stopped via the iOS app, but sending it ourselves during a live print did **not** stop it. Likely a side-effect broadcast, not a command. | `value` (paired with a `1037` entry) |
| **1027** | outbound only | Request a full status snapshot. No reply schema of its own — triggers a `/query/reply` array covering everything below. | *(no body)* |
| **1043** | outbound (GCODE_COMMAND) | Run a raw Marlin gcode line on the printer directly. The one truly general-purpose lever — used for temperature (see gotcha below), extrude/retract, and Pause/Resume. | `cmdLen`, `cmdData` |
| **1052** | inbound | Layer progress. | `total_layer`, `real_print_layer` |
| **1057** | inbound only | Confirmed **unrelated routine housekeeping** — the identical sequence (`1008 → 1057 → 1098`) recurs on a completely idle, untouched printer. Not a command trigger. | — |
| **1068** | inbound only | Job-ended summary report, seen once alongside a stop. Has a `trigger` field (seen value `3` for a user-cancelled job) that may distinguish cause from natural completion — unconfirmed. | `name`, `img`, `totalTime`, `filamentUsed`, `filamentUnit`, `saveTime`, `trigger` |
| **1072** | inbound | Bed-leveling status. | `isLeveled` |
| **1081** | — | **Dead field.** An earlier implementation pass guessed this was the print-event/progress field; confirmed via a real print (heating through completion) that it never changes off `{"value":-1,"progress":0}`. Superseded by 1000+1001. | — |
| **1098** | inbound | Filament type. | `filamentType` (array) |

Commands we build and send (`DeviceObjectNative.hpp`):
- **1003 / 1004** (set target temp) — sent alongside the matching gcode (`M104 Sxx` / `M140 Sxx`) via 1043. **The bare commandType alone is acked but does not actually engage the heater** — the printer only responds to the Marlin gcode.
- **1021** (set z-offset) — value scaled ×100.
- **1043** (gcode) — used for temperature (above), extrude/retract (`M83` then `G1 E<len>`/`G1 E-<len>`), and print Pause/Resume: `M25` (pause) / `M24` (resume). Both are standard Marlin SD-print gcodes and this firmware is Marlin-derived (reports version strings like `V3.3.20_3.1.25`), but neither has been independently confirmed against a real print yet.
- **1026** (stop, unconfirmed) — see the Known Gaps section below. **This does not work.**

`aknmt_print_event_e` (the enum `1000`'s `value` field maps onto, confirmed live for the
values marked):
```
IDLE = 0                            ✓ confirmed
PRINTING = 1                        ✓ confirmed
PAUSED = 2
STOPPED = 3                         (marked "no use" in the header; never observed —
                                      a real stop/cancel goes straight to IDLE=0 instead)
COMPLETED = 4                       ✓ confirmed
LEVELING = 5
DOWNLOADING = 6                     (marked "no use")
LEVEL_HEATING = 7
PRINT_HEATING = 8                   ✓ confirmed
PREHEATING = 9
PRINT_DOWNLOADING = 10              (marked "no use")
CALIBRATION = 11
CALIBRATION_HEATING = 12
EXTRUSION_PREHEATING = 13           (marked "no use")
FEED_AND_RETURN_TO_THE_MATERIAL = 14 (marked "no use")
SLICING = 15                        (marked "no use")
LOAD_MATERIAL = 16
UNLOAD_MATERIAL = 17
```

---

## 3. PPPP — LAN (direct UDP file transfer + camera)

PPPP is Anker's own P2P/UDP protocol, used for two things once you have the printer's LAN
IP: uploading a gcode file to start a print, and streaming the live camera. It needs no
`dsk_key` on the LAN path (only the remote/relay path, §4, needs it). Printer listens on
UDP port **32108**.

### Discovery

The device list's `ip_addr` field is frequently empty or stale, so connection tries a
directed connect to that IP first (6s), then falls back to **LAN broadcast discovery**:
send `LAN_SEARCH` to `255.255.255.255:32108` and lock onto whichever reply's DUID
(prefix+serial) matches the target device.

### Wire format

Every packet: `0xF1` + `type`(1 byte) + `len`(u16 **big-endian**) + payload. Struct fields
inside payloads are little-endian unless noted.

**Packet types:**
```
0x00 HELLO           0x01 HELLO_ACK
0x12 DEV_LGN_CRC
0x20 P2P_REQ          0x21 P2P_REQ_ACK      0x26 P2P_REQ_DSK
0x30 LAN_SEARCH
0x40 PUNCH_TO         0x41 PUNCH_PKT
0x42 P2P_RDY          0x43 P2P_RDY_ACK
0xD0 DRW              0xD1 DRW_ACK
0xE0 ALIVE            0xE1 ALIVE_ACK
0xF0 CLOSE
0xF9 SESSION_READY    (REPORT_SESSION_READY — remote path only, see §4)
```

**Duid** (device identity, from `p2p_did` string `"prefix-serial-check"`, packed to 20 bytes):
`prefix`(8, zero-padded) + `serial`(u32 **big-endian**) + `check`(6, zero-padded) + `pad`(2, zero).

**Host** (16 bytes — a network address): `pad0`(1)=0 + `afam`(1)=2 (AF_INET) + `port`(u16 LE)
+ `addr`(4 bytes) + `pad1`(8, zero). **Byte order of `addr` is context-dependent** — see the
callout in §4; the LAN-only code path (`packPeerHost`) does not reverse it.

### Connection handshake (LAN)

1. Send `LAN_SEARCH` (empty payload) to the broadcast address.
2. On `PUNCH_PKT` (while connecting): reply `CLOSE` then `P2P_RDY(duid)`.
3. On `P2P_RDY` whose DUID matches: reply `P2P_RDY_ACK` (duid+host+8 zero pad), state → Connected.
4. Ongoing: `HELLO` → `HELLO_ACK(host)`; `ALIVE` → `ALIVE_ACK` (empty); `CLOSE` → reset.

### Reliable channel (DRW)

Writes are split into ≤1024-byte chunks, each sent as a `DRW` packet with payload
`0xD1`(signature) + `chan`(1) + `index`(u16 **big-endian**) + data. Every received `DRW`
is acked with `DRW_ACK` = `0xD1` + `chan` + `count=1`(u16 BE) + `acks[index]`(u16 BE).
Unacked packets retransmit every 500ms.

### File upload (once connected)

1. **Announce**: an `XZYH` frame on channel 0, `cmd = P2P_SEND_FILE (0x3a98)`, data = a
   16-character ID string (machine ID, padded/truncated).
2. **BEGIN**: an `AABB` frame on channel 1, `frametype = 0x00`, data = a metadata string
   `"0,<filename>,<size>,<md5-hex>,<user_name>,<user_id>,<machine_id>"` + trailing NUL.
   (Identity fields aren't strictly validated — placeholder values work.)
3. **DATA**: the file in 32KB chunks, each an `AABB` frame `frametype = 0x01` with the
   running byte offset as `pos`.
4. **END**: an `AABB` frame `frametype = 0x02`, empty payload. **Required** — omitting it
   means the printer acks every BEGIN/DATA frame OK but its own display shows "transfer
   failed"; the END frame is what actually finalizes the job and starts the print.

Every AABB frame is sent reliably (via the DRW channel) and its `REPLY` (`frametype = 0x80`,
1-byte payload `0x00` = OK) is awaited before proceeding.

**XZYH frame** (channel 0, used for file-transfer announce and camera commands): `"XZYH"`(4)
+ `cmd`(u16 LE) + `len`(u32 LE) + 6 reserved bytes (all 0) + data. 16-byte header total.

**AABB frame** (channel 1, used for file transfer): `0xAA 0xBB` + `frametype`(1) + `sn`(1)=0
+ `pos`(u32 LE) + `len`(u32 LE) + data + `crc16`(u16 LE). CRC is **CRC16/XMODEM** (poly
`0x1021`, init 0, no reflect, no xorout) computed over everything after the 2-byte magic.
Reply frametype is `BEGIN/DATA/END | 0x80`; 1-byte reply payload: `0x00`=OK,
`0xFC`=timeout, `0xFD`=frame-type error, `0xFE`=MD5 mismatch, `0xFF`=busy.

### Camera / live video

Uses the same PPPP connection. Send an `XZYH` frame on channel 0, `cmd = P2P_JSON_CMD
(0x06a4)`, data = JSON:
```json
{"commandType": 1000, "data": {"encryptkey": "x", "accountId": "y"}}
```
(`commandType: 1000` here means START_LIVE; the stream is **not actually encrypted** despite
the field name — it's a placeholder.) Stop with `{"commandType": 1001}` (CLOSE_LIVE).

Video frames arrive as `XZYH` packets on channel 1; each frame's payload is a chunk of raw
H.264 elementary-stream data — concatenating them is a playable stream. Confirmed: the M5
streams 1280×720 @ 25fps H.264. SPS/PPS are resent every GOP, so the decoder should only
rebuild its session when parameters actually change, not on every occurrence.

---

## 4. PPPP — Remote / Relay (works over the internet)

This is the hardest-won part of the protocol. It is **not** a relay/TURN-style forwarding
scheme (an earlier implementation attempt built and tested a full `RLY_*` packet flow —
`RLY_REQ`/`RLY_TO`/`RLY_PKT` — and it's a dead end: the relay authenticates the rendezvous
but never actually bridges traffic). The real mechanism is a **direct NAT hole-punch with
an ICE-style candidate exchange**, brokered through Anker's relay servers only to introduce
the two sides to each other.

### Sequence

1. **Resolve relay hosts**: the device's `p2p_conn` field is a shuffle-ciphered
   comma-separated list of relay hostnames (e.g. `p2p-mk-ohi.ankermake.com`, plus IPs).
   Decoded with a 54-byte shuffle-table cipher (`decodeInitString`) — unrelated to the
   session cipher in step 3.
2. **STUN + rendezvous**, to every relay host on UDP port **32100**, repeated every 500ms
   for up to 6s:
   - `HELLO` (empty) → `HELLO_ACK` returns a 16-byte **Host = our own public/WAN address**
     as seen by the relay. Save these 16 bytes **verbatim** — this becomes `addr_wan` below.
   - `P2P_REQ_DSK` (0x26), payload = `duid(20) + host(16, zero placeholder) + nat_type(1)=0
     + version(3)={1,0,0} + dsk(20, the raw — not hex — dsk_key) + pad(4)` = 64 bytes total.
     → `P2P_REQ_ACK` (mark=0 means accepted) followed by one or more `PUNCH_TO` packets,
     each a 16-byte Host giving one of the **device's** candidate addresses (its LAN IP and
     its WAN IP are both offered, ICE-style — collect all of them, don't overwrite).
3. **Build and send `REPORT_SESSION_READY` (type `0xF9`)** — this is the step that makes
   remote work at all; without it the device has no idea where to punch back to, and the
   community reference project's own client leaves this send commented out entirely.
   Plaintext (84 bytes), then run through the session cipher below:
   ```
   duid(20) + middle(16, fixed constant bytes) + addr_local(16) + addr_wan(16) + addr_relay(16)
   ```
   `addr_local` and `addr_relay` are built with byte-reversed address octets (see callout
   below); `addr_wan` is the `HELLO_ACK` bytes copied through unchanged. Send this to every
   relay host every ~300ms during the punch phase.
4. **Punch**: to every candidate from step 2, every ~300ms: `PUNCH_PKT(duid)` then
   `P2P_RDY(duid)`. The device, now knowing our address from step 3, punches back —
   typically from an **ephemeral port**, which the client must accept even before formally
   "locked" (the reply may not come from the exact candidate address). Once its `P2P_RDY`
   arrives, the peer locks and the connection is up; DRW/XZYH proceed exactly as in the LAN
   path, just addressed to whichever ephemeral port answered.
5. The official app visibly "fails once, retries, connects" in normal use — budget the
   whole sequence 20–30 seconds and keep resending, don't give up after one round.

### Byte-reversed addresses — the critical gotcha

**The 4-byte address field inside a `Host` struct is stored byte-reversed on the wire**
(i.e. little-endian of network order) in the STUN/ICE parts of this exchange — e.g.
`192.168.12.142` is transmitted as bytes `8e 0c a8 c0`, not `c0 a8 0c 8e`. This applies to
`PUNCH_TO` candidate addresses and to the `addr_local`/`addr_relay` fields built into
`REPORT_SESSION_READY`. It does **not** apply to the plain LAN-only `packPeerHost` helper
(§3), which is a different code path for a different purpose (acking a locked LAN peer,
not exchanging ICE candidates) — reading a remote candidate address forward, un-reversed,
silently produces a syntactically valid but wrong IP, and was the root cause of an early
remote-connect failure that looked like a NAT/CGNAT problem but wasn't.

### Session cipher (`simpleEncrypt`)

A stream cipher, seed = the fixed ASCII string `"SSD@cs2-network."`, using a 256-byte
shuffle table (`PPPP_SIMPLE_SHUFFLE`) distinct from the 54-byte one used for `p2p_conn`.
`simpleHash(seed)` folds the seed into a 4-int hash; `simpleLookup(h, b) = SHUFFLE[(h[b&3]+b)
mod 256]`. Encryption is cipher-feedback style: `out[0] = in[0] xor lookup(h,0)`, then
`out[i] = in[i] xor lookup(h, out[i-1])` for each subsequent byte — each output byte's XOR
key depends on the *previous output byte*, not the previous input byte (decryption mirrors
this using `in[i-1]` instead).

### What this achieves

Confirmed working: remote live camera (continuous 720p frames over the internet, off the
printer's LAN) and remote gcode send-to-printer, both over this same punched connection.
Remote MQTT monitoring and all MQTT-based controls (temperature, z-offset) already worked
over the internet regardless, since MQTT goes through Anker's cloud broker directly and was
never LAN-limited.

---

## Known gaps / unsolved

- **Stop print**: unsolved. `M524` (the standard Marlin cancel-SD-print gcode) is
  **explicitly rejected** by this firmware — it replies `"ok\n\n+ringbuf:...\nUnknown
  command"` (the leading "ok" is just Marlin's generic per-line acknowledgment and can
  mislead you into thinking the command succeeded if you don't check the full reply text).
  Vendor commandTypes `1057` and `1026` were both tried against a live, actively-printing
  job and neither stopped it (see the commandType table above). The real trigger is
  whatever the iOS/official app publishes to the device-bound `/device/maker/<sn>/command`
  topic, which this client cannot observe (see §2's topic note). Until solved, use the iOS
  app or the printer's own screen to cancel a print. Pause (`M25`)/Resume (`M24`) use the
  standard Marlin gcodes and are plausible but not yet independently confirmed against a
  real print.
- **Message Center** (the notification-bell feature in the official apps): entirely
  unimplemented — a separate authenticated HTTP API, comparable in scope to the login/
  device-list work, that hasn't been reverse-engineered. A local alternative (a
  `print_history.csv` log of start/finish/duration/filament/layers per print, independent
  of any cloud API) was built instead to cover the actual use case (tracking printer usage).
- **RLY_\* relay-forwarding packets**: implemented and tested, confirmed to be the *wrong*
  mechanism for this printer (see §4) — the code path exists in `AnkerPppp.cpp` but is
  dead/unused. Left in only as a historical artifact; safe to remove if it becomes a
  maintenance burden.
- Several fields are marked **best guess** above (z-offset scaling, the exact semantics of
  `1000`'s STOPPED=3 value which has never actually been observed on the wire) — treat them
  as working-but-unverified, not confirmed spec.
