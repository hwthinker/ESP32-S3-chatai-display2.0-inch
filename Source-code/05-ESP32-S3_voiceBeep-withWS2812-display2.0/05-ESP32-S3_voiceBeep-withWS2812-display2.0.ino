/*
 * ESP32-S3 Complete Hardware Test - NeoPixel Version
 * Board: ESP32-S3 N16R8 with ST7789 2.0" LCD
 * Tests: LCD + Speaker + Microphone + WS2812B (using NeoPixel lib)
 *
 * Hardware SPI 40MHz + baseline noise calibration (robust percentile)
 */

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_NeoPixel.h>
#include <driver/i2s.h>
#include <math.h>

// ========== LCD PINS (ST7789 2.0" 240x320) ==========
#define PIN_TFT_SCLK  21
#define PIN_TFT_MOSI  47
#define PIN_TFT_MISO  -1
#define PIN_TFT_CS    41
#define PIN_TFT_DC    40
#define PIN_TFT_RST   45
#define PIN_TFT_BL    42
#define TFT_WIDTH     240
#define TFT_HEIGHT    320

// ========== SPEAKER PINS ==========
#define SPEAKER_DOUT  7
#define SPEAKER_BCLK  15
#define SPEAKER_LRC   16

// ========== MICROPHONE PINS ==========
#define MIC_WS        4
#define MIC_SCK       5
#define MIC_SD        6

// ========== WS2812B LED (NeoPixel) ==========
#define LED_PIN       48
#define NUM_LEDS      1

// ========== I2S CONFIGURATION ==========
#define SAMPLE_RATE   16000
#define I2S_SPEAKER   I2S_NUM_0
#define I2S_MIC       I2S_NUM_1

// ========== OBJECTS ==========
SPIClass spi(FSPI);
Adafruit_ST7789 tft = Adafruit_ST7789(&spi, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ========== VARIABLES ==========
int32_t mic_buffer[512];
int16_t speaker_buffer[512];
bool speaker_playing = false;
float tone_freq = 440.0f;
double phase = 0.0;
int led_mode = 3;  // 0=off, 1=rainbow, 2=breathing, 3=mic reactive

// Baseline noise calibration
int32_t baseline_avg = 0;
int32_t last_raw_level = 0;

// ========== LCD FUNCTIONS ==========
void initLCD() {
  spi.begin(PIN_TFT_SCLK, PIN_TFT_MISO, PIN_TFT_MOSI);

  pinMode(PIN_TFT_BL, OUTPUT);
  digitalWrite(PIN_TFT_BL, HIGH);

  // Initialize ST7789 2.0" 240x320
  tft.init(TFT_WIDTH, TFT_HEIGHT);
  tft.setSPISpeed(40000000);  // 40 MHz hardware SPI
  tft.setRotation(2);  // Portrait inverted (180°): 240x320
  tft.fillScreen(ST77XX_BLACK);

  // Draw border
  tft.drawRect(0, 0, TFT_WIDTH, TFT_HEIGHT, ST77XX_BLUE);
  tft.setTextWrap(false);

  // Title
  tft.setCursor(28, 16);
  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(2);
  tft.print("ESP32-S3");

  // Subtitle
  tft.setCursor(15, 56);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(1);
  tft.print("HW TEST v2.1");

  // Divider line
  tft.drawLine(9, 84, 230, 84, ST77XX_WHITE);
}

void updateLCDStatus(const char* device, const char* status, uint16_t color) {
  static int line = 0;
  int y = 96 + (line * 24);

  if (line >= 6) {
    tft.fillRect(9, 96, 221, 144, ST77XX_BLACK);
    line = 0;
    y = 96;
  }

  tft.setCursor(15, y);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.print(device);

  tft.setCursor(122, y);
  tft.setTextColor(color);
  tft.print(status);

  line++;
}

void drawMicLevel(int level) {
  int barWidth = map(level, 0, 1000, 0, 202);
  barWidth = constrain(barWidth, 0, 202);

  // Clear bar area
  tft.fillRect(19, 284, 202, 20, ST77XX_BLACK);

  if (barWidth > 0) {
    uint16_t barColor = ST77XX_GREEN;
    if (level > 500) barColor = ST77XX_YELLOW;
    if (level > 800) barColor = ST77XX_RED;

    tft.fillRect(19, 284, barWidth, 20, barColor);
  }

  // Draw bar outline
  tft.drawRect(19, 284, 202, 20, ST77XX_WHITE);
}

void updateLEDModeDisplay() {
  tft.fillRect(19, 210, 202, 24, ST77XX_BLACK);
  tft.setCursor(19, 214);
  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(1);
  tft.print("LED: ");

  switch(led_mode) {
    case 0: tft.print("OFF"); break;
    case 1: tft.print("Rainbow"); break;
    case 2: tft.print("Breath"); break;
    case 3: tft.print("MicReact"); break;
  }
}

// ========== LED FUNCTIONS (NeoPixel) ==========
void initLED() {
  Serial.print("Initializing WS2812B (NeoPixel) on GPIO 48... ");

  strip.begin();
  strip.setBrightness(80);  // 0-255
  strip.clear();
  strip.show();

  delay(200);

  Serial.println("OK!");
  Serial.println("Testing LED: R-G-B-W...");

  // Red
  strip.setPixelColor(0, strip.Color(255, 0, 0));
  strip.show();
  delay(300);

  // Green
  strip.setPixelColor(0, strip.Color(0, 255, 0));
  strip.show();
  delay(300);

  // Blue
  strip.setPixelColor(0, strip.Color(0, 0, 255));
  strip.show();
  delay(300);

  // White
  strip.setPixelColor(0, strip.Color(255, 255, 255));
  strip.show();
  delay(300);

  // Off
  strip.setPixelColor(0, strip.Color(0, 0, 0));
  strip.show();

  Serial.println("LED test complete!");
}

// Helper function: Convert HSV to RGB
void setPixelColorHSV(uint16_t hue, uint8_t sat, uint8_t val) {
  uint32_t color = strip.gamma32(strip.ColorHSV(hue * 65536 / 360, sat, val));
  strip.setPixelColor(0, color);
}

void updateLED(int mic_level) {
  static uint16_t hue = 0;
  static uint8_t brightness = 0;
  static bool breathing_up = true;

  switch(led_mode) {
    case 0:  // OFF
      strip.setPixelColor(0, strip.Color(0, 0, 0));
      break;

    case 1:  // Rainbow cycle
      hue += 2;
      if (hue >= 360) hue = 0;
      setPixelColorHSV(hue, 255, 255);
      break;

    case 2:  // Breathing effect (Blue)
      if (breathing_up) {
        brightness += 5;
        if (brightness >= 250) breathing_up = false;
      } else {
        brightness -= 5;
        if (brightness <= 5) breathing_up = true;
      }
      strip.setPixelColor(0, strip.Color(0, 0, brightness));
      break;

    case 3:  // Mic reactive
      {
        uint8_t led_brightness = map(mic_level, 0, 1000, 0, 255);
        led_brightness = constrain(led_brightness, 0, 255);

        // Color transition: Green (low) -> Yellow (med) -> Red (high)
        uint8_t red, green, blue;

        if (mic_level < 500) {
          // Green to Yellow
          red = map(mic_level, 0, 500, 0, 255);
          green = 255;
          blue = 0;
        } else {
          // Yellow to Red
          red = 255;
          green = map(mic_level, 500, 1000, 255, 0);
          blue = 0;
        }

        // Apply brightness
        red = (red * led_brightness) / 255;
        green = (green * led_brightness) / 255;
        blue = (blue * led_brightness) / 255;

        strip.setPixelColor(0, strip.Color(red, green, blue));
      }
      break;
  }

  strip.show();
}

// ========== SPEAKER FUNCTIONS ==========
void initSpeaker() {
  i2s_config_t speaker_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t speaker_pins = {
    .bck_io_num = SPEAKER_BCLK,
    .ws_io_num = SPEAKER_LRC,
    .data_out_num = SPEAKER_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_SPEAKER, &speaker_config, 0, NULL);
  i2s_set_pin(I2S_SPEAKER, &speaker_pins);
  i2s_set_clk(I2S_SPEAKER, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
}

void playTone(float freq, int duration_ms) {
  speaker_playing = true;
  const int16_t AMP = 27000;
  const double TWOPI = 6.283185307179586;
  double dphi = TWOPI * freq / SAMPLE_RATE;

  int total_samples = (SAMPLE_RATE * duration_ms) / 1000;
  int samples_written = 0;

  while (samples_written < total_samples) {
    int samples_this_round = min(256, total_samples - samples_written);

    for (int i = 0; i < samples_this_round; i++) {
      phase += dphi;
      if (phase >= TWOPI) phase -= TWOPI;

      int16_t sample = (int16_t)(AMP * sin(phase));
      speaker_buffer[i * 2] = sample;
      speaker_buffer[i * 2 + 1] = sample;
    }

    size_t written;
    i2s_write(I2S_SPEAKER, speaker_buffer, samples_this_round * 4, &written, portMAX_DELAY);
    samples_written += samples_this_round;
  }

  speaker_playing = false;
}

// ========== MICROPHONE FUNCTIONS ==========
void initMicrophone() {
  i2s_config_t mic_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t mic_pins = {
    .bck_io_num = MIC_SCK,
    .ws_io_num = MIC_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = MIC_SD
  };

  i2s_driver_install(I2S_MIC, &mic_config, 0, NULL);
  i2s_set_pin(I2S_MIC, &mic_pins);
  i2s_zero_dma_buffer(I2S_MIC);

  for (int i = 0; i < 10; i++) {
    size_t bytes_read;
    i2s_read(I2S_MIC, mic_buffer, sizeof(mic_buffer), &bytes_read, 100);
  }
}

int readMicLevel() {
  size_t bytes_read;
  i2s_read(I2S_MIC, mic_buffer, sizeof(mic_buffer), &bytes_read, 100);

  if (bytes_read == 0) return 0;

  int samples = bytes_read / sizeof(int32_t);
  int64_t sum = 0;

  for (int i = 0; i < samples; i++) {
    int16_t sample = (int16_t)(mic_buffer[i] >> 14);
    sum += abs(sample);
  }

  last_raw_level = sum / samples;
  // Kurangi baseline — bar/LED hanya merespons level di atas ambient
  int32_t diff = last_raw_level - baseline_avg;
  return diff > 0 ? diff : 0;
}

// ========== BASELINE CALIBRATION ==========
void measureBaseline() {
  Serial.println("\n📊 Measuring baseline noise (3 sec)...");
  Serial.println("🤫 STAY QUIET...\n");

  // Tampilan kalibrasi di LCD
  tft.fillRect(9, 96, 221, 144, ST77XX_BLACK);

  tft.setCursor(13, 108);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.print("CALIBRATING");

  tft.setCursor(19, 152);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.print("Measuring ambient noise");
  tft.setCursor(19, 170);
  tft.setTextColor(ST77XX_CYAN);
  tft.print("Please stay quiet...");

  // Progress bar frame
  const int barX = 26, barY = 200, barW = 187, barH = 20;
  tft.drawRect(barX, barY, barW, barH, ST77XX_WHITE);

  // Pastikan LED off supaya tidak ada side effect
  strip.setPixelColor(0, strip.Color(0, 0, 0));
  strip.show();

  // Tunggu ekor beep test menghilang + flush DMA
  delay(500);
  i2s_zero_dma_buffer(I2S_MIC);

  // Extended warmup — buang frame awal (garbage + ekor suara)
  for (int i = 0; i < 20; i++) {
    size_t bytes_read;
    i2s_read(I2S_MIC, mic_buffer, sizeof(mic_buffer), &bytes_read, portMAX_DELAY);
  }

  // Kumpulkan rata-rata per-buffer untuk analisis distribusi
  const int N = 30;
  int32_t buf_avgs[N];

  for (int i = 0; i < N; i++) {
    size_t bytes_read;
    i2s_read(I2S_MIC, mic_buffer, sizeof(mic_buffer), &bytes_read, portMAX_DELAY);

    int samples = bytes_read / sizeof(int32_t);
    int64_t sum = 0;
    for (int j = 0; j < samples; j++) {
      int16_t sample = (int16_t)(mic_buffer[j] >> 14);
      sum += abs(sample);
    }
    buf_avgs[i] = (samples > 0) ? (sum / samples) : 0;

    int prog = map(i + 1, 0, N, 0, barW - 2);
    tft.fillRect(barX + 1, barY + 1, prog, barH - 2, ST77XX_CYAN);

    if (i % 5 == 0) Serial.print(".");
  }

  // Sort ascending (insertion sort, N kecil)
  for (int i = 1; i < N; i++) {
    int32_t key = buf_avgs[i];
    int j = i - 1;
    while (j >= 0 && buf_avgs[j] > key) {
      buf_avgs[j + 1] = buf_avgs[j];
      j--;
    }
    buf_avgs[j + 1] = key;
  }

  // Pakai rata-rata 30% sample TERENDAH — tahan transient noise
  int low_count = (N * 3) / 10;
  int64_t sum_low = 0;
  for (int i = 0; i < low_count; i++) sum_low += buf_avgs[i];
  baseline_avg = sum_low / low_count;

  Serial.printf("\n  Distribusi: min=%d  p25=%d  median=%d  p75=%d  max=%d\n",
                buf_avgs[0], buf_avgs[N / 4], buf_avgs[N / 2],
                buf_avgs[(3 * N) / 4], buf_avgs[N - 1]);
  Serial.printf("✓ Baseline (rata2 30%% terendah): %d\n", baseline_avg);

  if (baseline_avg < 30) {
    Serial.println("⚠️  Baseline TOO LOW — cek L/R pin INMP441 ke GND");
  } else if (baseline_avg > 2000) {
    Serial.println("⚠️  Baseline TOO HIGH — banyak noise elektrik");
  } else {
    Serial.println("✓ Baseline OK!");
  }
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n╔════════════════════════════════════════════╗");
  Serial.println("║   ESP32-S3 Complete Hardware Test v2.1    ║");
  Serial.println("║   ST7789 2.0\" 240x320 — Hardware SPI     ║");
  Serial.println("╚════════════════════════════════════════════╝\n");

  // Initialize LCD
  Serial.print("Initializing ST7789 LCD... ");
  initLCD();
  Serial.println("OK!");
  updateLCDStatus("LCD:", "OK", ST77XX_GREEN);
  delay(400);

  // Initialize Speaker
  Serial.print("Initializing Speaker... ");
  initSpeaker();
  Serial.println("OK!");
  updateLCDStatus("Speaker:", "OK", ST77XX_GREEN);
  delay(400);

  // Initialize Microphone
  Serial.print("Initializing Microphone... ");
  initMicrophone();
  Serial.println("OK!");
  updateLCDStatus("Mic:", "OK", ST77XX_GREEN);
  delay(400);

  // Initialize LED (NeoPixel)
  initLED();
  updateLCDStatus("RGB LED:", "OK", ST77XX_GREEN);
  delay(400);

  // Test speaker with beep + LED flash
  Serial.println("Testing speaker (beep)...");
  updateLCDStatus("Test:", "Beeping..", ST77XX_YELLOW);

  strip.setPixelColor(0, strip.Color(255, 255, 0));  // Yellow
  strip.show();
  playTone(440, 300);

  strip.setPixelColor(0, strip.Color(0, 255, 255));  // Cyan
  strip.show();
  delay(200);
  playTone(523, 300);

  strip.setPixelColor(0, strip.Color(0, 0, 0));  // Off
  strip.show();

  updateLCDStatus("Test:", "Done!", ST77XX_GREEN);
  delay(400);

  // Measure baseline noise (ambient) — setelah beep test
  measureBaseline();

  // Show monitoring screen
  tft.fillRect(9, 96, 221, 144, ST77XX_BLACK);

  tft.setCursor(22, 104);
  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(2);
  tft.print("MONITOR");

  tft.setCursor(15, 144);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.print("Speak/clap");

  tft.setCursor(15, 166);
  tft.print("to test mic");

  tft.setCursor(15, 190);
  tft.print("BOOT = Beep");

  updateLEDModeDisplay();

  tft.setCursor(19, 250);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(1);
  tft.print("Mic Level:");

  // Tampilkan baseline value
  tft.setCursor(135, 250);
  tft.setTextColor(ST77XX_CYAN);
  tft.printf("Base:%4d", baseline_avg);

  Serial.println("\n╔════════════════════════════════════════════╗");
  Serial.println("║  ALL SYSTEMS READY!                       ║");
  Serial.println("║  - Speak/clap to test microphone          ║");
  Serial.println("║  - Press BOOT (short) for beep test       ║");
  Serial.println("║  - Hold BOOT (1s) to change LED mode      ║");
  Serial.println("║                                            ║");
  Serial.println("║  LED Modes:                                ║");
  Serial.println("║    0 = OFF                                 ║");
  Serial.println("║    1 = Rainbow                             ║");
  Serial.println("║    2 = Breathing                           ║");
  Serial.println("║    3 = Mic Reactive (VU Meter) ⭐         ║");
  Serial.println("╚════════════════════════════════════════════╝\n");

  pinMode(0, INPUT_PULLUP);
}

// ========== LOOP ==========
void loop() {
  static uint32_t last_update = 0;
  static uint32_t last_beep = 0;
  static uint32_t boot_press_time = 0;
  static bool boot_pressed = false;
  static bool long_press_handled = false;

  // Check BOOT button
  if (digitalRead(0) == LOW) {
    if (!boot_pressed) {
      boot_pressed = true;
      boot_press_time = millis();
      long_press_handled = false;
    }

    // Long press (>1 second) = change LED mode
    if (!long_press_handled && (millis() - boot_press_time > 1000)) {
      led_mode = (led_mode + 1) % 4;

      Serial.printf("\n💡 LED Mode changed to: %d\n", led_mode);
      updateLEDModeDisplay();

      // Visual feedback
      strip.setPixelColor(0, strip.Color(255, 255, 255));
      strip.show();
      delay(100);
      strip.setPixelColor(0, strip.Color(0, 0, 0));
      strip.show();

      long_press_handled = true;
    }

  } else if (boot_pressed) {
    uint32_t press_duration = millis() - boot_press_time;

    // Short press (<1 second) = beep test
    if (press_duration < 1000 && !long_press_handled) {
      if (millis() - last_beep > 500) {
        Serial.println("\n🔊 Playing test beep...");

        tft.fillRect(19, 210, 202, 24, ST77XX_BLACK);
        tft.setCursor(38, 214);
        tft.setTextColor(ST77XX_YELLOW);
        tft.setTextSize(1);
        tft.print("Beeping...");

        strip.setPixelColor(0, strip.Color(255, 0, 255));  // Magenta
        strip.show();
        playTone(440, 200);

        delay(100);

        strip.setPixelColor(0, strip.Color(0, 255, 255));  // Cyan
        strip.show();
        playTone(523, 200);

        strip.setPixelColor(0, strip.Color(0, 0, 0));
        strip.show();

        updateLEDModeDisplay();
        last_beep = millis();
      }
    }

    boot_pressed = false;
  }

  // Update mic level and LED every 50ms
  if (millis() - last_update > 50) {
    int mic_level = readMicLevel();  // sudah dikurangi baseline

    // Update LCD bar
    drawMicLevel(mic_level);

    // Update LED based on mode
    updateLED(mic_level);

    // Print to serial every 200ms
    static uint32_t last_serial = 0;
    if (millis() - last_serial > 200) {
      int bar_length = map(mic_level, 0, 1000, 0, 40);
      bar_length = constrain(bar_length, 0, 40);

      Serial.print("[");
      for (int i = 0; i < 40; i++) {
        Serial.print(i < bar_length ? "█" : " ");
      }
      Serial.print("] ");
      Serial.printf("Raw:%4d Base:%4d Diff:%4d ",
                    last_raw_level, baseline_avg, mic_level);

      if (mic_level < 50) {
        Serial.print("⚪");
      } else if (mic_level < 200) {
        Serial.print("🔉");
      } else if (mic_level < 500) {
        Serial.print("🔊");
      } else {
        Serial.print("📢");
      }

      Serial.printf(" | LED:%d", led_mode);
      Serial.println();

      last_serial = millis();
    }

    last_update = millis();
  }

  delay(10);
}
