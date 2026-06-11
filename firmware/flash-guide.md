# ESP32-S3 Flash Guide

Panduan flashing firmware untuk seri xiaozhi-ESP32-S3 chat-AI display 2.0 inch.
Semua binary di folder ini sudah berupa **merged image** (bootloader + partitions + boot_app0 + app dalam satu file), sehingga cukup di-flash ke offset `0x0`.

- Build date: **2026-06-11**
- Chip: **ESP32-S3** (16 MB flash, OPI PSRAM)
- FQBN: `esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=default`
- Flash settings: `dio` / `80 MHz` / `16 MB`

> Catatan: contoh di bawah memakai port `COM8`. Ganti sesuai port di komputermu (cek di Device Manager atau jalankan `arduino-cli board list`).

---

## Daftar binary yang berhasil di-build

| # | File firmware | Ukuran | Sumber project |
|---|---------------|-------:|----------------|
| 0 | `00-esp32S3-WS2812BTest.bin` | 442 KB | `Source-code/00-esp32S3-WS2812BTest` |
| 1 | `01-esp32S3-SpeakerTest.bin` | 374 KB | `Source-code/01-esp32S3-SpeakerTest` |
| 2 | `02-esp32-S3-display-test-display2.0.bin` | 407 KB | `Source-code/02-esp32-S3-display-test-display2.0` |
| 3 | `03-esp32S3-MicTest.bin` | 369 KB | `Source-code/03-esp32S3-MicTest` |
| 4 | `04-ESP32-S3-VoiceBeep-display2.0.bin` | 440 KB | `Source-code/04-ESP32-S3-VoiceBeep-display2.0` |
| 5 | `05-ESP32-S3_voiceBeep-withWS2812-display2.0.bin` | 443 KB | `Source-code/05-ESP32-S3_voiceBeep-withWS2812-display2.0` |
| 6 | `06-ESP32-S3-VoiceMonitoring-display2.0.bin` | 436 KB | `Source-code/06-ESP32-S3-VoiceMonitoring-display2.0` |
| 7 | `07-ESP32S3-VoiceRecorder-withPSRAM-display2.0.bin` | 438 KB | `Source-code/07-ESP32S3-VoiceRecorder-withPSRAM-display2.0` |

---

## Cara flashing (esptool v5.x — disarankan)

esptool versi baru memakai dash, bukan underscore.
Jalankan perintah ini dari folder root project (sehingga path `firmware/...` valid):

```bash
# 0) WS2812B LED test
esptool --chip esp32s3 --port COM8 --baud 921600 write-flash 0x0 firmware/00-esp32S3-WS2812BTest.bin

# 1) Speaker test
esptool --chip esp32s3 --port COM8 --baud 921600 write-flash 0x0 firmware/01-esp32S3-SpeakerTest.bin

# 2) Display 2.0 inch test
esptool --chip esp32s3 --port COM8 --baud 921600 write-flash 0x0 firmware/02-esp32-S3-display-test-display2.0.bin

# 3) Microphone test
esptool --chip esp32s3 --port COM8 --baud 921600 write-flash 0x0 firmware/03-esp32S3-MicTest.bin

# 4) Voice + beep + display 2.0
esptool --chip esp32s3 --port COM8 --baud 921600 write-flash 0x0 firmware/04-ESP32-S3-VoiceBeep-display2.0.bin

# 5) Voice + beep + WS2812 + display 2.0
esptool --chip esp32s3 --port COM8 --baud 921600 write-flash 0x0 firmware/05-ESP32-S3_voiceBeep-withWS2812-display2.0.bin

# 6) Voice monitoring + display 2.0
esptool --chip esp32s3 --port COM8 --baud 921600 write-flash 0x0 firmware/06-ESP32-S3-VoiceMonitoring-display2.0.bin

# 7) Voice recorder + PSRAM + display 2.0
esptool --chip esp32s3 --port COM8 --baud 921600 write-flash 0x0 firmware/07-ESP32S3-VoiceRecorder-withPSRAM-display2.0.bin
```

### Sintaks lama (esptool ≤ 4.x / esptool.py)

Beberapa pengguna masih punya esptool versi lama. Sintaksnya pakai underscore:

```bash
esptool.py --chip esp32s3 --port COM8 --baud 921600 write_flash 0x0 firmware/<nama-file>.bin
```

### Erase flash (opsional, kalau sebelumnya pernah di-flash project lain)

```bash
esptool --chip esp32s3 --port COM8 erase-flash
```

---

## Tips troubleshooting

- **Port tidak terdeteksi** — pastikan kabel USB-C bisa kirim data (bukan charger-only), driver CH340/CP210x atau native USB-CDC sudah ter-install.
- **`A fatal error occurred: Failed to connect to ESP32-S3`** — tahan tombol `BOOT`, tekan-lepas `RESET`, lalu lepas `BOOT`. Setelah itu jalankan ulang perintah `write-flash`.
- **Baud rate terlalu tinggi** — coba turunkan ke `460800` atau `115200` kalau koneksi sering putus.
- **Output Serial Monitor terlihat acak** — set baud monitor ke `115200`. Semua sketch dibuild dengan `USBMode=hwcdc`, jadi gunakan port USB-CDC bawaan ESP32-S3.
- **Mau verify hasil flash** — tambahkan `--verify` di akhir perintah `write-flash`.

---

## Catatan build

Detail lengkap (ukuran flash/RAM per project, error compile, dsb.) ada di [`build-log.txt`](./build-log.txt).

**Status: semua 8 project berhasil di-compile dan di-merge. Tidak ada error.**
