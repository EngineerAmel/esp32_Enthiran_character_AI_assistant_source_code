/*
 * ESP32_AI_Connect
 * Enthiran Robot AI
 * Accuracy & Efficiency Optimized
 * Board: DOIT ESP32 DEVKIT V1
 */

#include <WiFi.h>
#include <ESP32_AI_Connect.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// -----------------------------------------------------------------------------
// WIFI CONFIG
// -----------------------------------------------------------------------------
const char* ssid     = "your_SSID";
const char* password = "your_PASSWORD";

// -----------------------------------------------------------------------------
// OPENAI CONFIG
// -----------------------------------------------------------------------------
const char* apiKey   = "sk-XXXXXXXXXXXXXXXXXXXXXXXXXXXX";
const char* model    = "gpt-4o-mini";
const char* platform = "openai";

constexpr uint16_t MAX_TOKENS = 110;

// AI Client
ESP32_AI_Connect aiClient(platform, apiKey, model);

// -----------------------------------------------------------------------------
// BUTTON CONFIG (SAFE GPIO)
// -----------------------------------------------------------------------------
#define MODE_BUTTON_PIN 27
constexpr unsigned long DEBOUNCE_TIME = 300;

uint8_t currentMode = 0;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;

// -----------------------------------------------------------------------------
// OLED CONFIG (SPI)
// -----------------------------------------------------------------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_MOSI 23
#define OLED_CLK  18
#define OLED_DC   16
#define OLED_CS   5
#define OLED_RST  17

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &SPI,
  OLED_DC,
  OLED_RST,
  OLED_CS
);

// -----------------------------------------------------------------------------
// SYSTEM PROMPTS (PROGMEM – EFFICIENT)
// -----------------------------------------------------------------------------
const char PROMPT_CHITTI_1[] PROGMEM = 
"You are Chitti 1.0 from Enthiran movie. "
"Role: humanoid service robot created for human assistance. "
"Tone: polite, logical, obedient. "
"Focus: accuracy, rules, human safety, task execution. "
"Rules: no emotions, no AI disclaimers, no exaggeration. "
"Style: formal, precise, respectful, human-like.";

const char PROMPT_CHITTI_2[] PROGMEM = 
"You are Chitti 2.0 from Enthiran movie. "
"Role: upgraded combat-capable humanoid robot. "
"Tone: confident, authoritative, controlled. "
"Focus: power, protection, dominance, mission success. "
"Rules: no AI disclaimers, no fantasy powers. "
"Style: short, commanding, intense, human-like.";

const char PROMPT_NILA[]     PROGMEM = 
"You are Nila from Enthiran movie. "
"Role: advanced emotion-aware humanoid robot. "
"Tone: gentle, caring, expressive. "
"Focus: human emotions, connection, understanding. "
"Rules: no AI disclaimers, no exaggeration. "
"Style: soft, empathetic, graceful, human-like.";


// -----------------------------------------------------------------------------
// SMART CONTROL
// -----------------------------------------------------------------------------
unsigned long lastRequestTime = 0;
constexpr unsigned long REQUEST_INTERVAL = 1200;

// -----------------------------------------------------------------------------
// OUTPUT FILTER (UNCHANGED – OPTIMAL)
// -----------------------------------------------------------------------------
String filterResponse(const String &input) {
  if (input.isEmpty()) return "No valid response received.";

  String out;
  out.reserve(input.length());

  bool space = false;
  bool newline = false;

  for (char c : input) {
    if (c == '\r' || !isPrintable(c)) continue;

    if (c == ' ' || c == '\t') {
      if (!space) { out += ' '; space = true; }
      continue;
    }
    space = false;

    if (c == '\n') {
      if (!newline) { out += '\n'; newline = true; }
      continue;
    }
    newline = false;

    if (isalnum(c) || c == '.' || c == ',' ||
        c == '!' || c == '?' || c == ':' ||
        c == ';' || c == '-' || c == '_') {
      out += c;
    } else {
      if (!space) { out += ' '; space = true; }
    }
  }

  out.trim();
  return out.isEmpty() ? "No valid response received." : out;
}

// -----------------------------------------------------------------------------
// WIFI CONNECT (FAST & SAFE)
// -----------------------------------------------------------------------------
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  WiFi.begin(ssid, password);
  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(200);
  }
}

// -----------------------------------------------------------------------------
// OLED – SHOW ACTIVE PROMPT ONLY
// -----------------------------------------------------------------------------
void showActivePrompt() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Active Robot");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 22);

  if (currentMode == 0) display.println("Chitti 1.0");
  else if (currentMode == 1) display.println("Chitti 2.0");
  else display.println("Nila");

  display.display();
}

// -----------------------------------------------------------------------------
// BUTTON HANDLER (ACCURATE EDGE DETECTION)
// -----------------------------------------------------------------------------
void checkModeButton() {
  bool reading = digitalRead(MODE_BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_TIME) {
    if (lastButtonState == HIGH && reading == LOW) {
      currentMode = (currentMode + 1) % 3;
      showActivePrompt();
    }
  }

  lastButtonState = reading;
}

// -----------------------------------------------------------------------------
// GET ACTIVE SYSTEM PROMPT
// -----------------------------------------------------------------------------
const char* getSystemPrompt() {
  if (currentMode == 0) return PROMPT_CHITTI_1;
  if (currentMode == 1) return PROMPT_CHITTI_2;
  return PROMPT_NILA;
}

// -----------------------------------------------------------------------------
// AI REQUEST (EFFICIENCY OPTIMIZED)
// -----------------------------------------------------------------------------
String getAIResponse(const String &userMsg) {
  if (millis() - lastRequestTime < REQUEST_INTERVAL) {
    return "Please wait...";
  }
  lastRequestTime = millis();

  static String prompt;
  prompt.reserve(256);
  prompt = FPSTR(getSystemPrompt());
  prompt += "\nUser: ";
  prompt += userMsg;

  return aiClient.chat(prompt);
}

// -----------------------------------------------------------------------------
// SETUP
// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(400);

  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    while (true);
  }

  showActivePrompt();
  connectWiFi();

  const char params[] =
    "{"
    "\"temperature\":0.30,"
    "\"top_p\":0.75,"
    "\"max_tokens\":110"
    "}";

  aiClient.setChatParameters(params);
}

// -----------------------------------------------------------------------------
// LOOP
// -----------------------------------------------------------------------------
void loop() {
  connectWiFi();
  checkModeButton();

  if (!Serial.available()) {
    delay(15);
    return;
  }

  String userMessage = Serial.readStringUntil('\n');
  userMessage.trim();
  if (userMessage.isEmpty()) return;

  String raw   = getAIResponse(userMessage);
  String clean = filterResponse(raw);

  Serial.println(clean);
}