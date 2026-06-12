# Flash Guide - ESP32-S3 Xiaozhi ChatAI Display 2.0"

Panduan flashing firmware untuk seri project ESP32-S3 di repository ini.
Semua binary di folder ini adalah **merged binary** (bootloader + partitions + boot_app0 + app) yang langsung di-flash ke offset `0x0`.

## Build info

- **arduino-cli**: 1.5.0-rc.1
- **esp32 core**: 3.3.10
- **esptool**: 5.2.0
- **FQBN**: `esp32:esp32:esp32s3`
- **Flash mode**: dio | **Flash freq**: 80m | **Flash size**: 16MB
- **Tanggal build**: 2026-06-12

## Daftar Binary

Semua binary berhasil di-build tanpa error:

| # | File | Size (bytes) | Project |
|---|------|-------------:|---------|
| 00 | `00-esp32S3-WS2812BTest.bin` | 447,072 | Test LED WS2812B |
| 01 | `01-esp32S3-SpeakerTest.bin` | 377,568 | Test Speaker (I2S output) |
| 02 | `02-esp32-S3-display-test-display2.0.bin` | 410,976 | Test Display LCD 2.0" |
| 03 | `03-esp32S3-MicTest.bin` | 371,760 | Test Mikrofon (I2S input) |
| 04 | `04-ESP32-S3-VoiceBeep-display2.0.bin` | 444,816 | Voice Beep + Display |
| 05 | `05-ESP32-S3_voiceBeep-withWS2812-display2.0.bin` | 448,320 | Voice Beep + WS2812 + Display |
| 06 | `06-ESP32-S3-VoiceMonitoring-display2.0.bin` | 441,072 | Voice Monitoring + Display |
| 07 | `07-ESP32S3-VoiceRecorder-withPSRAM-display2.0.bin` | 442,432 | Voice Recorder + PSRAM + Display |
| 08 | merge-bin |  | xiaozhi firmware |

## Persiapan

1. Hubungkan board ESP32-S3 ke PC (port default: **COM8** — sesuaikan jika berbeda).
2. Tahan tombol **BOOT** -> tekan **RESET** -> lepas **BOOT** untuk masuk download mode (jika board tidak auto-reset).
3. Pastikan `esptool` v4.x atau v5.x sudah terinstall (`pip install esptool`).

## Perintah Flash (per binary)

Jalankan dari folder `firmware/`. Ganti `COM8` jika port berbeda.

### 00 - WS2812B Test
```bash
esptool --chip esp32s3 --port COM8 --baud 921600 write_flash 0x0 00-esp32S3-WS2812BTest.bin
```

### 01 - Speaker Test
```bash
esptool --chip esp32s3 --port COM8 --baud 921600 write_flash 0x0 01-esp32S3-SpeakerTest.bin
```

### 02 - Display Test
```bash
esptool --chip esp32s3 --port COM8 --baud 921600 write_flash 0x0 02-esp32-S3-display-test-display2.0.bin
```

### 03 - Mic Test
```bash
esptool --chip esp32s3 --port COM8 --baud 921600 write_flash 0x0 03-esp32S3-MicTest.bin
```

### 04 - Voice Beep + Display
```bash
esptool --chip esp32s3 --port COM8 --baud 921600 write_flash 0x0 04-ESP32-S3-VoiceBeep-display2.0.bin
```

### 05 - Voice Beep + WS2812 + Display
```bash
esptool --chip esp32s3 --port COM8 --baud 921600 write_flash 0x0 05-ESP32-S3_voiceBeep-withWS2812-display2.0.bin
```

### 06 - Voice Monitoring + Display
```bash
esptool --chip esp32s3 --port COM8 --baud 921600 write_flash 0x0 06-ESP32-S3-VoiceMonitoring-display2.0.bin
```

### 07 - Voice Recorder (PSRAM) + Display
```bash
esptool --chip esp32s3 --port COM8 --baud 921600 write_flash 0x0 07-ESP32S3-VoiceRecorder-withPSRAM-display2.0.bin
```

## 08 - burn xioazhi firmware

```
esptool --chip esp32s3 -p COM8 -b 921600 erase-flash; esptool --chip esp32s3 -p COM8 -b 921600 write-flash 0x0 merged-binary.bin
```



## Catatan

- **Erase flash dulu** jika sebelumnya pernah di-flash dengan partisi berbeda:
  ```bash
  esptool --chip esp32s3 --port COM8 erase_flash
  ```
- Untuk esptool v5.x, subcommand juga bisa pakai tanda hubung (`write-flash`, `erase-flash`, `merge-bin`). Untuk v4.x pakai underscore.
- Setelah flash selesai, tekan tombol **RESET** untuk memulai firmware baru.
- Monitor serial: `arduino-cli monitor -p COM8 -c baudrate=115200`
