# Source Code - ESP32-S3 ChatAI Display 2.0"

Kumpulan sketch Arduino untuk testing hardware ESP32-S3 dengan display ST7789 2.0" (240x320), microphone INMP441, speaker MAX98357A, dan WS2812B LED.

Sketch diurutkan dari komponen paling sederhana ke fitur lengkap — bisa dipakai untuk verifikasi setiap modul sebelum integrasi penuh.

## 📋 Daftar Sketch

| No | Sketch | Komponen | Deskripsi |
|----|--------|----------|-----------|
| 00 | `00-esp32S3-WS2812BTest` | LED only | Test RGB LED on-board (GPIO 48) |
| 01 | `01-esp32S3-SpeakerTest` | Speaker | Test MAX98357A I2S — generate beep |
| 02 | `02-esp32-S3-display-test-display2.0` | Display | Hello world ST7789 2.0" |
| 03 | `03-esp32S3-MicTest` | Mic | INMP441 dengan kalibrasi baseline noise (UART only) |
| 04 | `04-ESP32-S3-VoiceBeep-display2.0` | Display + Speaker + Mic | Hardware test semua komponen + baseline calibration |
| 05 | `05-ESP32-S3_voiceBeep-withWS2812-display2.0` | + WS2812B | Code 04 + LED VU meter (4 modes) |
| 06 | `06-ESP32-S3-VoiceMonitoring-display2.0` | Voice recorder | Rekam 5 detik + playback (heap memory) |
| 07 | `07-ESP32S3-VoiceRecorder-withPSRAM-display2.0` | + PSRAM | Voice recorder pakai PSRAM (bisa rekam lebih panjang) |

## ⚙️ Konfigurasi Pin (Umum untuk Semua Sketch)

### Display ST7789 2.0" (SPI)
| Pin Display | ESP32-S3 GPIO |
|-------------|---------------|
| SCLK / SCL  | 21 |
| MOSI / SDA  | 47 |
| CS          | 41 |
| DC          | 40 |
| RST         | 45 |
| BL (backlight) | 42 |
| VCC         | 3.3V |
| GND         | GND |

### Audio I2S
| Komponen | Sinyal | GPIO |
|----------|--------|------|
| Speaker (MAX98357A) | DOUT/DIN | 7 |
| Speaker (MAX98357A) | BCLK     | 15 |
| Speaker (MAX98357A) | LRC      | 16 |
| Microphone (INMP441) | WS      | 4 |
| Microphone (INMP441) | SCK     | 5 |
| Microphone (INMP441) | SD      | 6 |

### Lain-lain
| Komponen | GPIO |
|----------|------|
| WS2812B LED (NeoPixel) | 48 |
| BOOT Button | 0 |

**Penting INMP441:** sambungkan pin **L/R ke GND** supaya channel left aktif. Jika dibiarkan floating, baseline noise akan terlalu tinggi.

## 🚀 Hardware SPI (Penting!)

Semua sketch yang pakai display ST7789 (02, 04, 05, 06, 07) sudah **migrasi dari software SPI ke hardware SPI 40MHz**. Ini wajib karena:

- **Software SPI** (~1-2 MHz): tiap `fillRect` besar bisa blok 50-80 ms → display lag, scroll lambat dari atas ke bawah, suara playback putus-putus karena DMA audio overflow.
- **Hardware SPI** (40 MHz): `fillRect` turun ke ~1-2 ms → display responsif, audio mulus.

Pattern yang dipakai di semua sketch display:

```cpp
SPIClass spi(FSPI);
Adafruit_ST7789 tft = Adafruit_ST7789(&spi, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);

void initLCD() {
  spi.begin(PIN_TFT_SCLK, -1, PIN_TFT_MOSI);  // -1 = MISO not used
  tft.init(TFT_WIDTH, TFT_HEIGHT);
  tft.setSPISpeed(40000000);
  // ...
}
```

**Hindari** constructor 5-arg: `Adafruit_ST7789(CS, DC, MOSI, SCLK, RST)` — ini software SPI bit-banging, sangat lambat.

## 🎤 Baseline Noise Calibration (Sketch 04 & 05)

Sketch 04 dan 05 menambahkan kalibrasi baseline noise saat startup:

1. Tunggu 500 ms setelah beep test agar bunyi fisik mereda
2. `i2s_zero_dma_buffer()` → buang sample ekor beep yang masih di DMA
3. 20 warmup reads → buang sample yang masih kontaminasi
4. Kumpulkan 30 buffer averages → sort → pakai rata-rata 30% terendah sebagai baseline

Pendekatan persentil ini **tahan terhadap transient noise** (mic glitch, suara sesaat) dibanding rata-rata sederhana. `mic_level` yang dibaca selalu **di atas baseline**, sehingga ambient noise tidak trigger LED/bar.

## 📦 Library yang Dibutuhkan

Instal via Arduino IDE → Tools → Manage Libraries:

- **Adafruit GFX Library**
- **Adafruit ST7735 and ST7789 Library**
- **Adafruit NeoPixel** (sketch 00, 05, 06, 07)

I2S driver dan PSRAM sudah termasuk di ESP32 Arduino core.

## 🛠️ Arduino IDE Settings

Pakai konfigurasi yang sama untuk semua sketch:

| Setting | Value |
|---------|-------|
| Board | ESP32S3 Dev Module |
| USB CDC On Boot | Enabled |
| USB Mode | Hardware CDC and JTAG |
| Flash Size | 16MB (128Mb) |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| **PSRAM** | **OPI PSRAM** ← wajib untuk sketch 07 |
| Upload Speed | 921600 |

FQBN untuk `arduino-cli`:
```
esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=default
```

## 🧪 Urutan Testing yang Disarankan

1. **00** → pastikan LED WS2812B nyala (R-G-B-W cycle)
2. **01** → pastikan speaker mengeluarkan beep
3. **02** → pastikan display nyala dan tampil "Hello ESP32-S3!"
4. **03** → pastikan mic INMP441 mendeteksi suara (monitor via UART)
5. **04** → semua komponen + baseline calibration
6. **05** → tambah LED VU meter
7. **06** → rekam-playback 5 detik (heap memory)
8. **07** → rekam-playback dengan PSRAM

Kalau salah satu gagal, fix dulu sebelum lanjut. Sketch berikutnya selalu cumulative dari yang sebelumnya.

## 🐛 Troubleshooting Singkat

| Gejala | Kemungkinan Penyebab |
|--------|---------------------|
| Display lambat, scroll dari atas | Masih pakai software SPI — cek constructor |
| Audio playback putus-putus | Display ops blokir DMA — pastikan hardware SPI |
| Mic baseline >2000 | L/R pin INMP441 floating, atau noise elektrik dari kabel |
| Mic baseline <30 | L/R pin tidak grounded, atau mic tidak dapat power |
| Sketch 07 alokasi gagal | PSRAM tidak enabled di Arduino IDE |
| LCD tampil garbage | Kabel SPI panjang/loose — turunkan `setSPISpeed` ke 20-26 MHz |
