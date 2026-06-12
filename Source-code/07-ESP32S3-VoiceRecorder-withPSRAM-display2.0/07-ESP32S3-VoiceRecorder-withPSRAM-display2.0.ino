/*
 * ESP32-S3 Voice Recorder - 30 Second with PSRAM
 * Modified for ST7789 2.0" 240x320 LCD
 *
 * Features:
 * - 30 second recording using PSRAM (8MB available)
 * - 500ms delay to skip button click
 * - No display flicker
 * - Auto playback after recording
 * - WS2812B LED indicator (GPIO 48):
 *   RED = Recording
 *   GREEN = Playing
 *   OFF = Idle/Waiting
 *
 * IMPORTANT: Enable PSRAM in Arduino IDE!
 * Tools > PSRAM > "OPI PSRAM"
 */

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_NeoPixel.h>
#include <driver/i2s.h>
#include <esp_psram.h>

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

// ========== AUDIO PINS ==========
#define SPEAKER_DOUT  7
#define SPEAKER_BCLK  15
#define SPEAKER_LRC   16

#define MIC_WS        4
#define MIC_SCK       5
#define MIC_SD        6

// ========== AUDIO SETTINGS ==========
#define SAMPLE_RATE   16000
#define RECORD_TIME   5
#define BUFFER_SIZE   512
#define BUTTON_DELAY  500       // 500ms delay to skip button click sound

// ========== WS2812B LED ==========
#define LED_PIN       48
#define NUM_LEDS      1

// Calculate total samples for 30 seconds
// Memory usage: 30s × 16000 × 2 bytes = 960KB (requires PSRAM!)
#define TOTAL_SAMPLES (SAMPLE_RATE * RECORD_TIME)

// ========== I2S PORTS ==========
#define I2S_SPEAKER   I2S_NUM_0
#define I2S_MIC       I2S_NUM_1

// ========== OBJECTS ==========
SPIClass spi(FSPI);
Adafruit_ST7789 tft = Adafruit_ST7789(&spi, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ========== BUFFERS ==========
int32_t mic_buffer[BUFFER_SIZE];
int16_t speaker_buffer[BUFFER_SIZE * 2]; // stereo

// Recording buffer - 30 seconds at 16kHz = 480,000 samples = 960KB
// MUST be in PSRAM!
int16_t* recording_buffer = NULL;
int recorded_samples = 0;

// ========== STATE ==========
enum State {
  IDLE,
  WAITING,
  RECORDING,
  PLAYING
};

State current_state = IDLE;
bool boot_pressed = false;
uint32_t state_start_time = 0;
int playback_position = 0;

// Display update tracking
float last_countdown = -1;
int last_progress = -1;

// ========== LED FUNCTIONS ==========
void setLED(uint8_t r, uint8_t g, uint8_t b) {
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();
}

void ledOff() {
  setLED(0, 0, 0);
}

void ledRed() {
  setLED(255, 0, 0);
}

void ledGreen() {
  setLED(0, 255, 0);
}

void ledYellow() {
  setLED(255, 255, 0);
}

// ========== LCD FUNCTIONS ==========
void initLCD() {
  // Hardware SPI 40MHz — jauh lebih cepat dari software SPI
  spi.begin(PIN_TFT_SCLK, PIN_TFT_MISO, PIN_TFT_MOSI);

  pinMode(PIN_TFT_BL, OUTPUT);
  digitalWrite(PIN_TFT_BL, HIGH);

  // Initialize ST7789 2.0" 240x320
  tft.init(TFT_WIDTH, TFT_HEIGHT);
  tft.setSPISpeed(40000000);  // 40 MHz hardware SPI
  tft.setRotation(2);  // Portrait inverted (180°): 240x320
  tft.fillScreen(ST77XX_BLACK);

  // Title
  tft.drawRect(0, 0, TFT_WIDTH, TFT_HEIGHT, ST77XX_BLUE);
  tft.setCursor(15, 16);
  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(1);
  tft.print("VOICE RECORDER");

  tft.drawLine(9, 44, 230, 44, ST77XX_WHITE);
}

void drawIdleScreen() {
  tft.fillRect(9, 56, 221, 254, ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setCursor(56, 70);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("READY");

  tft.setTextSize(1);
  tft.setCursor(15, 120);
  tft.setTextColor(ST77XX_YELLOW);
  tft.print("Press BOOT to");
  tft.setCursor(47, 144);
  tft.print("record");

  tft.setCursor(15, 180);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Record time:");
  tft.setCursor(15, 202);
  tft.printf("%d seconds", RECORD_TIME);

  tft.setCursor(15, 230);
  tft.setTextColor(ST77XX_MAGENTA);
  tft.print("Using PSRAM");

  if (recorded_samples > 0) {
    tft.setCursor(15, 260);
    tft.setTextColor(ST77XX_GREEN);
    tft.print("Last: ");
    tft.printf("%.1fs", recorded_samples / (float)SAMPLE_RATE);
  }

  tft.setCursor(19, 286);
  tft.setTextColor(ST77XX_CYAN);
  tft.printf("BOOT=Rec %ds", RECORD_TIME);

  // LED OFF when idle
  ledOff();
}

void drawWaitingScreen() {
  tft.fillRect(9, 56, 221, 254, ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setCursor(28, 80);
  tft.setTextColor(ST77XX_YELLOW);
  tft.print("GETREADY");

  tft.setTextSize(1);
  tft.setCursor(19, 140);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Recording in..");

  // LED YELLOW when waiting
  ledYellow();
}

void updateWaitingCountdown() {
  uint32_t elapsed = millis() - state_start_time;
  float remaining = (BUTTON_DELAY - elapsed) / 1000.0;

  if (remaining < 0) remaining = 0;

  if (abs(remaining - last_countdown) > 0.05) {
    tft.fillRect(75, 180, 94, 60, ST77XX_BLACK);
    tft.setTextSize(3);
    tft.setCursor(90, 190);
    tft.setTextColor(ST77XX_YELLOW);
    tft.printf("%.1f", remaining);
    last_countdown = remaining;
  }
}

void drawRecordingScreen() {
  tft.fillRect(9, 56, 221, 254, ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setCursor(15, 70);
  tft.setTextColor(ST77XX_RED);
  tft.print("RECORDING");

  tft.drawRect(19, 270, 202, 28, ST77XX_WHITE);
  last_countdown = -1;
  last_progress = -1;

  // LED RED when recording
  ledRed();
}

void updateRecordingProgress() {
  uint32_t elapsed = millis() - state_start_time;
  float remaining = RECORD_TIME - (elapsed / 1000.0);
  if (remaining < 0) remaining = 0;

  if (abs(remaining - last_countdown) > 0.1) {
    tft.fillRect(56, 140, 131, 70, ST77XX_BLACK);
    tft.setTextSize(3);
    tft.setCursor(84, 150);
    tft.setTextColor(ST77XX_RED);
    tft.printf("%.0f", remaining);
    last_countdown = remaining;
  }

  int progress = map(elapsed, 0, RECORD_TIME * 1000, 0, 198);
  progress = constrain(progress, 0, 198);

  if (abs(progress - last_progress) > 2) {
    tft.fillRect(20, 272, 198, 24, ST77XX_BLACK);
    tft.fillRect(20, 272, progress, 24, ST77XX_RED);
    last_progress = progress;
  }

  // Blinking LED effect during recording
  static uint32_t last_blink = 0;
  if (millis() - last_blink > 500) {
    static bool blink_state = false;
    if (blink_state) {
      ledRed();
    } else {
      setLED(100, 0, 0);  // Dim red
    }
    blink_state = !blink_state;
    last_blink = millis();
  }
}

void drawPlayingScreen() {
  tft.fillRect(9, 56, 221, 254, ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setCursor(47, 70);
  tft.setTextColor(ST77XX_GREEN);
  tft.print("PLAYING");

  tft.drawRect(19, 270, 202, 28, ST77XX_WHITE);
  last_progress = -1;

  // LED GREEN when playing
  ledGreen();
}

void updatePlayingProgress() {
  float progress_pct = (playback_position * 100.0) / recorded_samples;

  static int last_pct = -1;
  if (abs(progress_pct - last_pct) > 1) {
    tft.fillRect(66, 140, 112, 70, ST77XX_BLACK);
    tft.setTextSize(2);
    tft.setCursor(75, 160);
    tft.setTextColor(ST77XX_GREEN);
    tft.printf("%.0f%%", progress_pct);
    last_pct = progress_pct;
  }

  int progress = map(playback_position, 0, recorded_samples, 0, 198);
  progress = constrain(progress, 0, 198);

  if (abs(progress - last_progress) > 2) {
    tft.fillRect(20, 272, 198, 24, ST77XX_BLACK);
    tft.fillRect(20, 272, progress, 24, ST77XX_GREEN);
    last_progress = progress;
  }
}

// ========== AUDIO FUNCTIONS ==========
void initSpeaker() {
  i2s_config_t speaker_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 512,
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
}

void initMicrophone() {
  i2s_config_t mic_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 512,
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

// ========== RECORDING FUNCTIONS ==========
void startWaiting() {
  Serial.println("\n⏳ WAITING (skip button click)...");
  current_state = WAITING;
  state_start_time = millis();
  drawWaitingScreen();
}

void startRecording() {
  Serial.printf("\n🔴 RECORDING START (%d seconds)\n", RECORD_TIME);

  current_state = RECORDING;
  recorded_samples = 0;
  state_start_time = millis();

  // Allocate buffer in PSRAM
  if (recording_buffer == NULL) {
    Serial.println("Allocating 960KB buffer in PSRAM...");

    // Try to allocate in PSRAM first
    recording_buffer = (int16_t*)ps_malloc(TOTAL_SAMPLES * sizeof(int16_t));

    if (recording_buffer == NULL) {
      Serial.println("❌ PSRAM allocation failed! Trying heap...");
      recording_buffer = (int16_t*)malloc(TOTAL_SAMPLES * sizeof(int16_t));
    }

    if (recording_buffer == NULL) {
      Serial.println("❌ Failed to allocate recording buffer!");
      Serial.println("   Make sure PSRAM is enabled in Arduino IDE:");
      Serial.println("   Tools > PSRAM > OPI PSRAM");

      tft.fillRect(9, 56, 221, 254, ST77XX_BLACK);
      tft.setCursor(19, 120);
      tft.setTextColor(ST77XX_RED);
      tft.setTextSize(1);
      tft.print("ERROR:");
      tft.setCursor(19, 150);
      tft.print("No memory!");
      tft.setCursor(19, 180);
      tft.print("Enable PSRAM");
      tft.setCursor(19, 202);
      tft.print("in IDE");

      current_state = IDLE;
      delay(3000);
      drawIdleScreen();
      return;
    }

    // Check if buffer is in PSRAM
    if (heap_caps_get_allocated_size(recording_buffer) > 0) {
      Serial.println("✓ Buffer allocated successfully!");
      if ((uint32_t)recording_buffer >= 0x3C000000) {
        Serial.println("✓ Buffer is in PSRAM!");
      } else {
        Serial.println("⚠ Buffer in internal RAM (might be unstable for 30s)");
      }
    }
  }

  // Clear buffer
  memset(recording_buffer, 0, TOTAL_SAMPLES * sizeof(int16_t));

  // Flush mic buffer
  for (int i = 0; i < 5; i++) {
    size_t bytes_read;
    i2s_read(I2S_MIC, mic_buffer, sizeof(mic_buffer), &bytes_read, 100);
  }

  Serial.printf("Recording %d samples (%d seconds)...\n", TOTAL_SAMPLES, RECORD_TIME);
  drawRecordingScreen();
}

void recordAudio() {
  size_t bytes_read;
  i2s_read(I2S_MIC, mic_buffer, sizeof(mic_buffer), &bytes_read, portMAX_DELAY);

  if (bytes_read == 0) return;

  int samples = bytes_read / sizeof(int32_t);

  for (int i = 0; i < samples && recorded_samples < TOTAL_SAMPLES; i++) {
    int16_t sample = (int16_t)(mic_buffer[i] >> 14);
    sample = sample * 2;

    if (sample > 16000) sample = 16000;
    if (sample < -16000) sample = -16000;

    recording_buffer[recorded_samples++] = sample;
  }

  if (recorded_samples >= TOTAL_SAMPLES || (millis() - state_start_time) >= (RECORD_TIME * 1000)) {
    Serial.printf("✓ Recording complete! Captured %d samples (%.1fs)\n",
                  recorded_samples, recorded_samples / (float)SAMPLE_RATE);
    current_state = PLAYING;
    playback_position = 0;
    drawPlayingScreen();
    delay(300);
  }
}

void playbackAudio() {
  if (playback_position >= recorded_samples) {
    Serial.println("✓ Playback complete!\n");
    current_state = IDLE;
    playback_position = 0;
    drawIdleScreen();
    return;
  }

  int samples_to_play = min(BUFFER_SIZE, recorded_samples - playback_position);

  for (int i = 0; i < samples_to_play; i++) {
    int16_t sample = recording_buffer[playback_position + i];
    speaker_buffer[i * 2] = sample;
    speaker_buffer[i * 2 + 1] = sample;
  }

  size_t bytes_written;
  i2s_write(I2S_SPEAKER, speaker_buffer, samples_to_play * 4, &bytes_written, portMAX_DELAY);

  playback_position += samples_to_play;
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n╔════════════════════════════════════════════╗");
  Serial.printf("║   ESP32-S3 Voice Recorder (%d seconds)    ║\n", RECORD_TIME);
  Serial.println("║   ST7789 2.0\" 240x320 LCD Version        ║");
  Serial.println("╚════════════════════════════════════════════╝\n");

  // Check PSRAM
  if (psramFound()) {
    Serial.printf("✓ PSRAM found: %d bytes available\n", ESP.getFreePsram());
    Serial.printf("  Total PSRAM: %d bytes\n", ESP.getPsramSize());
  } else {
    Serial.println("❌ WARNING: PSRAM not found!");
    Serial.println("   Please enable PSRAM in Arduino IDE:");
    Serial.println("   Tools > PSRAM > OPI PSRAM");
    Serial.println("\n   Program will try to use heap RAM (may fail)\n");
  }

  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Buffer needed: %d bytes (960KB)\n\n", TOTAL_SAMPLES * 2);

  // Initialize LED
  Serial.print("Initializing WS2812B LED... ");
  strip.begin();
  strip.setBrightness(80);
  strip.clear();
  strip.show();
  Serial.println("OK!");

  // LED test sequence
  Serial.println("LED Test: R-G-B...");
  ledRed();
  delay(300);
  ledGreen();
  delay(300);
  setLED(0, 0, 255);  // Blue
  delay(300);
  ledOff();

  // Initialize LCD
  Serial.print("Initializing ST7789 LCD... ");
  initLCD();
  Serial.println("OK!");

  tft.setCursor(19, 100);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(1);
  tft.print("Initializing");
  tft.setCursor(47, 124);
  tft.print("audio...");

  if (psramFound()) {
    tft.setCursor(19, 160);
    tft.setTextColor(ST77XX_GREEN);
    tft.print("PSRAM: ");
    tft.printf("%dMB", ESP.getPsramSize() / 1024 / 1024);
  } else {
    tft.setCursor(19, 160);
    tft.setTextColor(ST77XX_RED);
    tft.print("PSRAM:");
    tft.setCursor(19, 182);
    tft.print("NOT FOUND!");
  }

  // Initialize audio
  Serial.print("Initializing Speaker... ");
  initSpeaker();
  Serial.println("OK!");

  Serial.print("Initializing Microphone... ");
  initMicrophone();
  Serial.println("OK!");

  pinMode(0, INPUT_PULLUP);

  Serial.println("\n╔════════════════════════════════════════════╗");
  Serial.println("║  READY!                                   ║");
  Serial.printf("║  Press BOOT button to record %d seconds   ║\n", RECORD_TIME);
  Serial.println("║  (500ms delay to skip button click)       ║");
  Serial.println("║  Playback starts automatically after rec  ║");
  Serial.println("║                                            ║");
  Serial.println("║  LED Indicator:                            ║");
  Serial.println("║    🔴 RED    = Recording                   ║");
  Serial.println("║    🟢 GREEN  = Playing                     ║");
  Serial.println("║    ⚫ OFF    = Idle/Ready                  ║");
  Serial.println("╚════════════════════════════════════════════╝\n");

  delay(1000);
  drawIdleScreen();
}

// ========== LOOP ==========
void loop() {
  static uint32_t last_display_update = 0;

  if (current_state == IDLE) {
    if (digitalRead(0) == LOW && !boot_pressed) {
      boot_pressed = true;
      startWaiting();
    } else if (digitalRead(0) == HIGH) {
      boot_pressed = false;
    }
  }

  switch(current_state) {
    case IDLE:
      break;

    case WAITING:
      if (millis() - state_start_time >= BUTTON_DELAY) {
        startRecording();
      } else {
        updateWaitingCountdown();
      }
      break;

    case RECORDING:
      recordAudio();
      if (millis() - last_display_update > 100) {
        updateRecordingProgress();
        last_display_update = millis();
      }
      break;

    case PLAYING:
      playbackAudio();
      if (millis() - last_display_update > 200) {
        updatePlayingProgress();
        last_display_update = millis();
      }
      break;
  }
}
