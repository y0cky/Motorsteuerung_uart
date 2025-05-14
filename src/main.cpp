#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Display-Pins
#define OLED_SDA 33
#define OLED_SCL 35
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Deine WLAN-Zugangsdaten
const char* ssid = "Motorsteuerung";
const char* password = "12345678";

// Webserver auf Port 80
AsyncWebServer server(80);

// UART zu S32K144 (TX=GPIO17, RX=GPIO18)
#define UART_TX 17
#define UART_RX 18

void showDisplay(const String& head, const String& line2 = "", const String& line3 = "") {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(head);
  if (line2.length() > 0) {
    display.setCursor(0, 15); display.println(line2);
  }
  if (line3.length() > 0) {
    display.setCursor(0, 30); display.println(line3);
  }
  display.display();
}

void setup() {
  Serial.begin(115200);         // USB-Debugging
  Serial1.begin(115200, SERIAL_8N1, UART_RX, UART_TX);  // UART zur S32K144

  // I2C für OLED initialisieren
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 nicht gefunden!"));
    while(1); // Stoppe weiter
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("BLDC Motorsteuerung");
  display.setCursor(0,15);
  display.println("ESP32 WLAN AP:");
  display.display();

  WiFi.softAP(ssid, password);
  Serial.println("WLAN gestartet: " + WiFi.softAPIP().toString());
  display.setCursor(0,30);
  display.println(WiFi.softAPIP());
  display.display();

  // HTML UI
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head>
        <title>BLDC Motorsteuerung</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
          body { font-family: Arial; text-align: center; padding: 20px; background: #f2f2f2; }
          h2 { color: #333; }
          input[type=number] {
            padding: 8px; width: 80px; border-radius: 5px; border: 1px solid #ccc;
          }
          input[type=submit], .btn {
            padding: 10px 20px;
            background: #007BFF;
            color: white;
            border: none;
            border-radius: 5px;
            margin: 5px;
            cursor: pointer;
          }
          .btn:hover, input[type=submit]:hover {
            background: #0056b3;
          }
          .container {
            background: white;
            padding: 20px;
            border-radius: 10px;
            display: inline-block;
            box-shadow: 0 0 10px rgba(0,0,0,0.1);
          }
        </style>
      </head>
      <body>
        <div class="container">
          <h2>BLDC Motorsteuerung</h2>
          <form action="/set" method="get">
            <label>Drehzahl (RPM):</label><br>
            <input type="number" name="speed" min="0" max="10000"><br><br>
            <input type="submit" value="Setzen">
          </form>
          <br>
          <a href="/start"><button class="btn">Motor Starten</button></a>
          <a href="/stop"><button class="btn">Motor Stoppen</button></a>
          <a href="/status"><button class="btn">Status abfragen</button></a>
        </div>
      </body>
      </html>
    )rawliteral";
    request->send(200, "text/html", html);
  });

  // Drehzahl setzen
  server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("speed")) {
      String speed = request->getParam("speed")->value();
      String cmd = "SET_SPEED:" + speed + "\n";
      Serial1.print(cmd);
      showDisplay("Drehzahl gesetzt", "RPM: " + speed);
      request->send(200, "text/plain", "Drehzahl gesetzt auf: " + speed);
    } else {
      showDisplay("Fehler", "Kein Speed");
      request->send(400, "text/plain", "Fehlender Parameter");
    }
  });

  // Start
  server.on("/start", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial1.println("START");
    showDisplay("Motor gestartet");
    request->send(200, "text/plain", "Motor gestartet");
  });

  // Stop
  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial1.println("STOP");
    showDisplay("Motor gestoppt");
    request->send(200, "text/plain", "Motor gestoppt");
  });

  // Status
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial1.println("STATUS?");
    delay(100); // Antwortzeit abwarten
    String status = "";
    while (Serial1.available()) {
      status += (char)Serial1.read();
    }
    if (status.length() == 0) status = "Keine Antwort vom Dev Kit.";
    showDisplay("Status:", status.substring(0, 20));
    request->send(200, "text/plain", "Status:\n" + status);
  });

  server.begin();
}

void loop() {
  // Nichts notwendig, Webserver läuft asynchron
}