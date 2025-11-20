/*
  ESP32 – Totem Motivacional + Descanso
  - Conexao WiFi para Wokwi
  - Frases motivacionais via API (HTTPS)
  - Frases de descanso locais
  - Troca a cada 30 segundos, alternando motivacao/descanso
  - Sanitiza texto para tirar acentos (display nao suporta UTF-8)

  API usada: https://motivacional.top/api.php?acao=aleatoria
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#define TFT_DC  2
#define TFT_CS  15

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ============ CONFIG WIFI ============
const char* ssid     = "Wokwi-GUEST";
const char* password = "";

// ============ API ============
const char* MOTIVACIONAL_URL = "https://motivacional.top/api.php?acao=aleatoria";

WiFiClientSecure secureClient;

// ============ TEMPO ============
unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 30000; // 30 segundos
bool mostrarDescanso = false;

// ============ FRASES DE DESCANSO ============
const char* descanso[] = {
  "Levante-se e alongue o corpo.",
  "Respire fundo por 10 segundos.",
  "Descanse os olhos e olhe longe.",
  "Beba um pouco de agua.",
  "Alongue o pescoco devagar.",
  "Relaxe os ombros por alguns segundos.",
  "De uma pequena caminhada.",
  "Respire profundamente tres vezes.",
  "Mantenha uma boa postura ao trabalhar.",
  "Feche os olhos por 5 segundos e relaxe."
};
const int QTD_DESCANSO = sizeof(descanso) / sizeof(descanso[0]);

// ---- Texto com quebra de linha ----
void drawWrappedText(const String &text, int x, int y, int maxWidth) {
  int16_t x1, y1;
  uint16_t w, h;
  String line = "";
  int cursorY = y;

  for (int i = 0; i < text.length(); i++) {
    char c = text[i];
    if (c == '\n') {
      tft.setCursor(x, cursorY);
      tft.println(line);
      line = "";
      cursorY += 20;
    } else {
      line += c;
      tft.getTextBounds(line, x, cursorY, &x1, &y1, &w, &h);
      if (w > maxWidth) {
        line.remove(line.length() - 1);
        tft.setCursor(x, cursorY);
        tft.println(line);
        cursorY += 20;
        line = "";
        line += c;
      }
    }
  }

  if (line.length() > 0) {
    tft.setCursor(x, cursorY);
    tft.println(line);
  }
} 

// ---- Mensagem padrão ----
void showMessage(const String &msg, uint16_t color = ILI9341_WHITE) {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(color);
  tft.setTextSize(2);
  drawWrappedText(msg, 10, 10, 300);
} 

// ---- Remove acentos / bytes UTF-8 para o display ----
String sanitizeText(const String &in) {
  String out = "";
  for (unsigned int i = 0; i < in.length(); i++) {
    uint8_t c = (uint8_t)in[i];

    // se for byte de UTF-8 (>= 128), ignora
    if (c >= 128) {
      continue;
    }
    // ignora \r
    if (c == '\r') continue;

    out += (char)c;
  }
  return out;
} 

// ---- Conecta WiFi com timeout e log de tempo ----
bool connectWifiComTimeout(unsigned long timeoutMs) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password, 6); // canal 6 é o estável no Wokwi

  unsigned long inicio = millis();
  Serial.print("Conectando WiFi");

  while (WiFi.status() != WL_CONNECTED && (millis() - inicio) < timeoutMs) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    unsigned long fim = millis();
    Serial.print("WiFi conectado em ");
    Serial.print((fim - inicio) / 1000.0);
    Serial.println(" segundos.");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("Timeout ao conectar WiFi.");
    return false;
  }
} 

// ---- Garante que o WiFi está conectado (reconecta se cair) ----
bool ensureWifiConnected() {
  if (WiFi.status() == WL_CONNECTED) return true;
  Serial.println("WiFi caiu, tentando reconectar...");
  showMessage("Reconectando\nWiFi...", ILI9341_WHITE);
  bool ok = connectWifiComTimeout(10000);
  if (!ok) {
    showMessage("Sem conexao WiFi.\nRodando offline.", ILI9341_RED);
  }
  return ok;
} 

// ---- Frase de descanso ----
void mostrarFraseDescanso() {
  int index = random(QTD_DESCANSO);

  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("Hora do descanso:");

  tft.setTextColor(ILI9341_WHITE);
  drawWrappedText(descanso[index], 10, 50, 300);

  Serial.println(String("DESCANSO: ") + descanso[index]);
} 

// ---- Frase motivacional da API ----
void fetchMotivacional() {
  if (!ensureWifiConnected()) {
    // se não tiver WiFi, não chama API
    return;
  }

  showMessage("Carregando\nmotivacao...", ILI9341_CYAN);

  HTTPClient http;
  secureClient.setInsecure(); // ignora certificado (ok p/ testes/Wokwi)

  if (!http.begin(secureClient, MOTIVACIONAL_URL)) {
    showMessage("Erro HTTPClient.", ILI9341_RED);
    return;
  }

  int httpCode = http.GET();
  if (httpCode != 200) {
    showMessage("Erro HTTP: " + String(httpCode), ILI9341_RED);
    http.end();
    return;
  }

  String payload = http.getString();
  http.end();

  Serial.println("Resposta API:");
  Serial.println(payload);

  StaticJsonDocument<2048> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    showMessage("Erro no JSON.", ILI9341_RED);
    Serial.println(err.c_str());
    return;
  }

  const char* fraseRaw = doc["dados"][0]["frase"] | "Frase nao encontrada.";
  const char* autorRaw = doc["dados"][0]["autor"] | "";

  // limpa acentos/bytes estranhos para o display
  String frase = sanitizeText(String(fraseRaw));
  String autor = sanitizeText(String(autorRaw));

  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_CYAN);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("Motivacao:");

  tft.setTextColor(ILI9341_WHITE);
  drawWrappedText(String("\"") + frase + "\"", 10, 50, 300);

  if (autor.length() > 0) {
    tft.setTextColor(ILI9341_GREEN);
    tft.setTextSize(2);
    drawWrappedText(String("- ") + autor, 10, 180, 300);
  }

  Serial.println(String("MOTIVACIONAL: ") + frase);
} 

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0));

  tft.begin();
  tft.setRotation(1);
  showMessage("Totem Motivacional\nConectando WiFi...", ILI9341_WHITE);

  bool wifiOK = connectWifiComTimeout(10000); // tenta 10s

  if (wifiOK) {
    showMessage("WiFi conectado!\nIniciando...", ILI9341_GREEN);
    fetchMotivacional();           // começa com motivacional
  } else {
    showMessage("Sem WiFi.\nRodando offline.", ILI9341_RED);
    mostrarFraseDescanso();        // começa com descanso
  }

  lastUpdate = millis();
} 

void loop() {
  unsigned long agora = millis();

  if (agora - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = agora;

    if (mostrarDescanso) {
      mostrarFraseDescanso();
    } else {
      fetchMotivacional();
    }

    mostrarDescanso = !mostrarDescanso;
  }

  delay(50);
} 
