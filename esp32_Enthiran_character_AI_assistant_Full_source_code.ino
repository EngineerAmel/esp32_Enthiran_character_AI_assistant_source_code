/*
 * ESP32_AI_Connect
 * Enthiran Robot AI – Button Controlled SYSTEM PROMPT
 * Board: DOIT ESP32 DEVKIT V1
 *
 * RULES:
 * 1) SYSTEM PROMPT never changes automatically
 * 2) ONLY button press changes active character
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

ESP32_AI_Connect aiClient(platform, apiKey, model);

// -----------------------------------------------------------------------------
// BUTTON CONFIG (ONLY CONTROL FOR PROMPT CHANGE)
// -----------------------------------------------------------------------------
#define MODE_BUTTON_PIN 13
constexpr unsigned long DEBOUNCE_TIME = 300;

uint8_t currentMode = 0;              // 0=Chitti1,1=Chitti2,2=Nila
unsigned long lastButtonPress = 0;

// -----------------------------------------------------------------------------
// OLED CONFIG (SPI 128x64)
// -----------------------------------------------------------------------------
#define OLED_WIDTH  128
#define OLED_HEIGHT 64

#define OLED_CS   5
#define OLED_DC   16
#define OLED_RST  17

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &SPI, OLED_DC, OLED_RST, OLED_CS);

// -----------------------------------------------------------------------------
// SYSTEM PROMPTS (FIXED – NO AUTO CHANGE)
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

const char PROMPT_NILA[] PROGMEM =
"You are Nila from Enthiran movie. "
"Role: advanced emotion-aware humanoid robot. "
"Tone: gentle, caring, expressive. "
"Focus: human emotions, connection, understanding. "
"Rules: no AI disclaimers, no exaggeration. "
"Style: soft, empathetic, graceful, human-like.";


// -----------------------------------------------------------------------------
// REQUEST CONTROL (DOES NOT CHANGE PROMPT)
// -----------------------------------------------------------------------------
unsigned long lastRequestTime = 0;
constexpr unsigned long REQUEST_INTERVAL = 1200;

// -----------------------------------------------------------------------------
// OLED – SHOW ACTIVE CHARACTER ONLY
// -----------------------------------------------------------------------------
void updateOLED() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 20);

  if (currentMode == 0) display.println("Chitti 1.0");
  else if (currentMode == 1) display.println("Chitti 2.0");
  else display.println("Nila");

  display.display();
}

// -----------------------------------------------------------------------------
// BUTTON HANDLER (ONLY PLACE MODE CHANGES)
// -----------------------------------------------------------------------------
void checkModeButton() {
  if (digitalRead(MODE_BUTTON_PIN) == LOW) {
    if (millis() - lastButtonPress > DEBOUNCE_TIME) {

      currentMode = (currentMode + 1) % 3;   // CHANGE ONLY HERE
      lastButtonPress = millis();

      updateOLED();

      Serial.print("SYSTEM PROMPT CHANGED TO: ");
      if (currentMode == 0) Serial.println("Chitti 1.0");
      else if (currentMode == 1) Serial.println("Chitti 2.0");
      else Serial.println("Nila");
    }
  }
}

// -----------------------------------------------------------------------------
// GET CURRENT SYSTEM PROMPT (READ-ONLY)
// -----------------------------------------------------------------------------
const char* getSystemPrompt() {
  if (currentMode == 0) return PROMPT_CHITTI_1;
  if (currentMode == 1) return PROMPT_CHITTI_2;
  return PROMPT_NILA;
}

// -----------------------------------------------------------------------------
// OUTPUT FILTER (UNCHANGED)
// -----------------------------------------------------------------------------
String filterResponse(const String &input) {
  if (input.isEmpty()) return "No valid response.";

  String out;
  out.reserve(input.length());
  bool space = false;

  for (char c : input) {
    if (c == '\r' || !isPrintable(c)) continue;

    if (isalnum(c) || c == '.' || c == ',' ||
        c == '!' || c == '?' ||
        c == ':' || c == ';' || c == '-') {
      out += c;
      space = false;
    } else {
      if (!space) {
        out += ' ';
        space = true;
      }
    }
  }

  out.replace("As an AI language model", "");
  out.replace("I am an AI", "");
  out.trim();
  return out;
}

// -----------------------------------------------------------------------------
// WIFI CONNECT (NO PROMPT CHANGE)
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
// AI REQUEST (READS PROMPT, NEVER MODIFIES IT)
// -----------------------------------------------------------------------------
String getAIResponse(const String &userMsg) {

  if (millis() - lastRequestTime < REQUEST_INTERVAL) {
    return "Processing. Please wait.";
  }
  lastRequestTime = millis();

  String prompt = FPSTR(getSystemPrompt());
  prompt += "\nUser: ";
  prompt += userMsg;

  String response;
  for (uint8_t i = 0; i < 2; i++) {
    response = aiClient.chat(prompt);
    if (response.length() > 25) break;
    delay(250);
  }
  return response;
}

// -----------------------------------------------------------------------------
// SETUP
// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);

  connectWiFi();

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println("OLED init failed");
    while (true);
  }
  updateOLED();   // SHOW DEFAULT MODE ONLY ON START

  const char params[] =
    "{"
    "\"temperature\":0.32,"
    "\"top_p\":0.78,"
    "\"max_tokens\":110"
    "}";

  aiClient.setChatParameters(params);

  Serial.println("Enthiran AI Ready (Manual Prompt Control Only)");
}

// -----------------------------------------------------------------------------
// LOOP
// -----------------------------------------------------------------------------
void loop() {
  connectWiFi();
  checkModeButton();   // ONLY place prompt can change

  if (!Serial.available()) {
    delay(15);
    return;
  }

  String userMessage = Serial.readStringUntil('\n');
  userMessage.trim();
  if (userMessage.isEmpty()) return;

  String raw   = getAIResponse(userMessage);
  String clean = filterResponse(raw);

  Serial.println();
  if (currentMode == 0) Serial.println("Chitti 1.0:");
  else if (currentMode == 1) Serial.println("Chitti 2.0:");
  else Serial.println("Nila:");

  Serial.println(clean);
}