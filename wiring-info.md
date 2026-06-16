# Wiring Info — Goouuu ESP32-S3 ChatAI Display 2.0 Inch

Microcontroller: **ESP32-S3-WROOM N16R8** (16MB Flash, 8MB OPI PSRAM)

> Dokumen ini di-generate dari seluruh kode di folder `Source-code/` (project 00 s/d 07).
> Berisi: pin GPIO yang DIPAKAI tiap modul, peta pemakaian per project, dan daftar pin yang masih FREE / RESERVED.

---

## 1. WS2812B RGB LED (NeoPixel)

**Protokol:** 1-Wire (NeoPixel / single-wire digital)
**Source ref:** `00-esp32S3-WS2812BTest.ino`, `05-...`, `06-...`, `07-...`

| ESP32-S3 Pin | WS2812B Pin | Keterangan |
|---|---|---|
| GPIO 48 | DI / DIN | Data Input |
| 3V3 (atau 5V) | VCC / VDD | Power (3.3V untuk 1 LED) |
| GND | GND | Ground |

**Library:** Adafruit NeoPixel (direkomendasikan untuk ESP32-S3 dengan I2S)
> **Catatan:** Jangan gunakan FastLED jika memakai I2S (speaker/mic) karena conflict RMT channel → bootloop.
> File `00-esp32S3-WS2812BTest.ino` pakai FastLED hanya karena project itu **tidak** memakai I2S.

---

## 2. MAX98357A — Digital Amplifier / Speaker

**Protokol:** I2S (Inter-IC Sound) — TX (output audio), I2S port `I2S_NUM_0`
**Source ref:** `01-esp32S3-SpeakerTest.ino`, `04-...`, `05-...`, `06-...`, `07-...`

| ESP32-S3 Pin | MAX98357A Pin | Keterangan |
|---|---|---|
| GPIO 7  | DIN  | Digital Audio Data |
| GPIO 15 | BCLK | Bit Clock |
| GPIO 16 | LRC  | Left/Right Clock (Word Select) |
| 3V3 | Vin / VCC | Power 3.3V |
| GND | GND | Ground (short connect GAIN untuk mono) |
| — | Audio+ | Ke speaker (kabel merah) |
| — | Audio- | Ke speaker negatif |

**I2S Mode:** MASTER TX, 16-bit, sample-rate 16000 / 44100 Hz tergantung project, stereo (L+R interleaved).

---

## 3. INMP441 — Digital Microphone

**Protokol:** I2S (Inter-IC Sound) — RX (input audio), I2S port `I2S_NUM_1` (di project 04+) / `I2S_NUM_0` (di project 03)
**Source ref:** `03-esp32S3-MicTest.ino`, `04-...`, `05-...`, `06-...`, `07-...`

| ESP32-S3 Pin | INMP441 Pin | Keterangan |
|---|---|---|
| GPIO 4 | WS  | Word Select / Data Select |
| GPIO 5 | SCK | Serial Clock / Data Clock |
| GPIO 6 | SD  | Serial Data Output |
| 3V3 | VDD | Power 3.3V |
| GND | GND | Ground (short connect L/R untuk left channel) |

**I2S Mode:** MASTER RX, 16000 Hz, 32-bit, left channel only.

---

## 4. IPS Display ST7789 2.0" (240×320)

**Protokol:** Hardware SPI (FSPI bus, 40 MHz) — pin custom
**Source ref:** `02-esp32-S3-display-test-display2.0.ino`, `04-...`, `05-...`, `06-...`, `07-...`

| ESP32-S3 Pin | Display Pin | Keterangan |
|---|---|---|
| GPIO 21 | SCL | SPI Clock (SCLK) |
| GPIO 47 | SDA | SPI Data (MOSI) |
| GPIO 45 | RES | Reset Signal |
| GPIO 40 | DC  | Data/Command Select |
| GPIO 41 | CS  | Chip Select |
| GPIO 42 | BLK | Backlight Control (HIGH = ON) |
| 3V3 | VCC | Power 3.3V |
| GND | GND | Ground |

**Driver IC:** ST7789
**Library:** Adafruit ST7789 + Adafruit GFX
**Init:** `tft.init(240, 320)` + `tft.setRotation(2)`
**MISO:** tidak dipakai (`PIN_TFT_MISO = -1`)

---

## 5. BOOT Button (GPIO 0)

**Protokol:** Digital input (INPUT_PULLUP), aktif LOW
**Source ref:** `04-...`, `05-...`, `06-...`, `07-...`

| ESP32-S3 Pin | Fungsi | Keterangan |
|---|---|---|
| GPIO 0 | BOOT button | Trigger test-beep / interaksi user, di-pull-up internal |

> Tombol BOOT built-in pada modul ESP32-S3 — tidak butuh wiring eksternal.
> Strap pin: harus HIGH saat power-on untuk boot normal (jangan dipakai untuk beban yang menarik LOW).

---

## 6. USB Serial — Programming & Debug

**Protokol:** UART via USB

| Mode | Chip | Keterangan |
|---|---|---|
| USB Serial Converter (default) | CH343 | Switch hardware ke posisi Serial USB, USB CDC on Boot = Disabled |
| Native USB | ESP32-S3 built-in | Switch hardware ke posisi Native USB, USB CDC on Boot = Enabled |

**Baud rate upload:** 921600
**Baud rate serial monitor:** 115200 (untuk test code)
**Pin Native USB:** GPIO 19 (D-) & GPIO 20 (D+) — terpakai HANYA saat mode Native USB.
**Pin UART0 default:** GPIO 43 (U0TXD) & GPIO 44 (U0RXD) — dipakai untuk log Serial.

---

## Ringkasan GPIO Map (DIPAKAI)

| GPIO | Device | Fungsi | Protokol |
|---|---|---|---|
| 0  | BOOT button | Input tombol (pull-up) | GPIO IN |
| 4  | INMP441 Mic | WS (Word Select) | I2S RX |
| 5  | INMP441 Mic | SCK (Clock) | I2S RX |
| 6  | INMP441 Mic | SD (Data) | I2S RX |
| 7  | MAX98357A Speaker | DIN (Data) | I2S TX |
| 15 | MAX98357A Speaker | BCLK (Clock) | I2S TX |
| 16 | MAX98357A Speaker | LRC (Word Select) | I2S TX |
| 21 | ST7789 Display | SCL (SPI Clock) | SPI |
| 40 | ST7789 Display | DC (Data/Command) | SPI |
| 41 | ST7789 Display | CS (Chip Select) | SPI |
| 42 | ST7789 Display | BLK (Backlight) | GPIO OUT |
| 43 | UART0 / USB Serial | U0TXD (log Serial) | UART |
| 44 | UART0 / USB Serial | U0RXD (log Serial) | UART |
| 45 | ST7789 Display | RES (Reset) | GPIO OUT |
| 47 | ST7789 Display | SDA (MOSI) | SPI |
| 48 | WS2812B LED | DIN (Data) | 1-Wire NeoPixel |
| 19 | USB D− *(kondisional)* | dipakai bila Native USB aktif | USB |
| 20 | USB D+ *(kondisional)* | dipakai bila Native USB aktif | USB |

**Total GPIO terpakai aktif (selain UART/USB):** 14 pin (`0, 4, 5, 6, 7, 15, 16, 21, 40, 41, 42, 45, 47, 48`).

---

## Pemakaian per Project

| Project | LED 48 | Spk 7/15/16 | Mic 4/5/6 | Disp 21/40/41/42/45/47 | BOOT 0 |
|---|:---:|:---:|:---:|:---:|:---:|
| 00 — WS2812B Test          | ✅ | — | — | — | — |
| 01 — Speaker Test          | — | ✅ | — | — | — |
| 02 — Display Test          | — | — | — | ✅ | — |
| 03 — Mic Test              | — | — | ✅ | — | — |
| 04 — VoiceBeep + Display   | — | ✅ | ✅ | ✅ | ✅ |
| 05 — VoiceBeep + WS2812    | ✅ | ✅ | ✅ | ✅ | ✅ |
| 06 — VoiceMonitoring       | ✅ | ✅ | ✅ | ✅ | ✅ |
| 07 — VoiceRecorder + PSRAM | ✅ | ✅ | ✅ | ✅ | ✅ |

---

## GPIO FREE (Masih Tersedia untuk Ekspansi)

Diukur dari project paling berat (05 / 06 / 07: LED + Speaker + Mic + Display + BOOT semua aktif).

| GPIO | Status | Catatan / Strap / ADC |
|---|---|---|
| **1**  | FREE | ADC1_CH0 — bagus untuk analog input (mis. potensio, light sensor) |
| **2**  | FREE | ADC1_CH1 — strap-pin, jangan ditarik LOW saat boot |
| **3**  | FREE | ADC1_CH2 — strap-pin (JTAG sel), aman setelah boot |
| **8**  | FREE | ADC1_CH7 |
| **9**  | FREE | ADC1_CH8 |
| **10** | FREE | ADC1_CH9 |
| **11** | FREE | ADC2_CH0 — ADC2 tidak bisa dipakai bersamaan dengan WiFi aktif |
| **12** | RESERVED *(expansion)* | ADC2_CH1, RTC, FSPICLK, TOUCH12 — **Dipakai expansion board**, jangan dipakai ulang sebagai output saat expansion terpasang |
| **13** | FREE | ADC2_CH2 — sda. |
| **14** | FREE | ADC2_CH3 — sda. |
| **17** | FREE | ADC2_CH6 |
| **18** | FREE | ADC2_CH7 |
| **19** | FREE *(kondisional)* | USB D− jika Native USB dipakai → hindari |
| **20** | FREE *(kondisional)* | USB D+ jika Native USB dipakai → hindari |
| **38** | FREE | digital biasa, aman |
| **39** | FREE | digital biasa, juga jalur MTCK/JTAG (aman jika tidak debug JTAG) |
| **46** | FREE | strap-pin (boot mode), aman setelah boot — input/output OK |

**Total GPIO FREE praktis (aman + WiFi-friendly):** **13 pin** → `1, 2, 3, 8, 9, 10, 38, 39, 46` + pin ADC2 (`11, 13, 14, 17, 18`) hanya jika WiFi tidak butuh ADC.
> GPIO 12 dikeluarkan dari daftar FREE karena sudah dialokasikan untuk expansion board.

> Pin paling aman & paling lega: **GPIO 1, 2, 3, 8, 9, 10, 38, 39** — bisa langsung dipakai untuk tombol tambahan, sensor I²C (SDA/SCL), encoder, dll.

---

## GPIO RESERVED (Tidak Boleh Dipakai)

Modul ESP32-S3-WROOM **N16R8** memakai SPI Flash 16MB + OPI PSRAM 8MB → pin berikut diambil internal oleh PSRAM/Flash dan **TIDAK BOLEH** dipakai sebagai GPIO eksternal:

| GPIO | Fungsi Internal |
|---|---|
| 26 | SPI flash / PSRAM (SPICS1) |
| 27 | SPI flash / PSRAM |
| 28 | SPI flash / PSRAM |
| 29 | SPI flash / PSRAM |
| 30 | SPI flash / PSRAM |
| 31 | SPI flash / PSRAM |
| 32 | SPI flash / PSRAM |
| 33 | OPI PSRAM (R8) |
| 34 | OPI PSRAM (R8) |
| 35 | OPI PSRAM (R8) |
| 36 | OPI PSRAM (R8) |
| 37 | OPI PSRAM (R8) |

> Pada varian non-OPI (mis. N8R2 dengan QSPI PSRAM) GPIO 33–37 BISA dipakai. Modul ini **N16R8 (OPI)** → 33–37 tetap dilarang.

---

## Catatan Tambahan

- LED RGB (GPIO 48) berfungsi sebagai indikator status: biru kedip = WiFi config, hijau = terhubung, merah = rekam audio.
- BOOT button (GPIO 0) di project 04/05/06/07 dipakai untuk trigger test-beep secara manual.
- Pada beberapa board DIY, tombol "WiFi config" eksternal di-mount ke **GPIO 1** (tekan + RST untuk masuk WiFi config mode pada firmware XiaoZhi resmi). Di kode example folder `Source-code/` GPIO 1 tidak dipakai → masih bebas.
- Firmware XiaoZhi menggunakan koneksi **WiFi 2.4 GHz** untuk cloud AI.
- Hindari memakai GPIO 19/20 untuk sensor jika mode upload pakai Native USB.
- Untuk I²C tambahan (mis. sensor OLED/IMU), rekomendasi pin: **SDA=GPIO 8, SCL=GPIO 9** (free dan tidak ada konflik strap).
