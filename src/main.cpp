#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "automation.h"


// ==== Display-Pins und I2C ====
#define OLED_SDA 33
#define OLED_SCL 35
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ==== WLAN Zugangsdaten ====
const char* ssid = "Motorsteuerung";
const char* password = "12345678";

// ==== Webserver ====
AsyncWebServer server(80);

// ==== UART zu S32K144 ====
#define UART_TX 7
#define UART_RX 5

// ==== Potentiometer-Eingang ====
#define POTT_PIN 3  // ADC1_CHANNEL_6 (nur ADC1 Kanäle für WiFi!) 

// ==== Drehzahl-Modus ====
volatile bool externeVorgabe = true;      // Steuerung: false = Interne Vorgabe, true = Extern via Poti
volatile int letztePottiDrehzahl = -1;     // Zum Senden nur bei Änderung

// ==== Hilfsfunktion Display ====
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

// ==== SETUP ====
void setup() {
  Serial.begin(115200);         // Debugging über USB
  Serial.println("DEBUG: setup() läuft an!");

  Serial1.begin(115200, SERIAL_8N1, UART_RX, UART_TX); // UART zur S32K144
  

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

  // ==== HTML UI mit Modus-Umschalter ====
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <title>BLDC Motorsteuerung</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body { font-family: Arial; background: #f2f2f2; text-align:center; padding:30px;}
      .container {background:#fff; padding:20px; border-radius:10px; display:inline-block; box-shadow:0 0 10px rgba(0,0,0,0.1);}
      h2 {color:#333;}
      .btn {padding:10px 24px; background:#007BFF; color:white; border:none; border-radius:5px; margin:6px; cursor:pointer;}
      .btn:hover {background:#0056b3;}
      input[type=number] {padding: 7px; width: 90px; border-radius:5px; border:1px solid #ccc;}
      label {font-size:1.07em; margin:8px;}
      .active {
      background-color: #28a745 !important; /* Bootstrap-Grün */
      }
    </style>
  </head>
  <body>
    <div class="container">
      <h2>BLDC Motorsteuerung</h2>
      <form id="drehzahlForm" action="/set" method="get" style="display:inline;">
        <label>
          <input type="radio" name="modus" value="intern" checked onchange="modusChanged()"> Interne Drehzahl (Web)
        </label>
        <label>
          <input type="radio" name="modus" value="extern" onchange="modusChanged()"> Externe Drehzahl (Poti)
        </label>
        <br><br>
        <div id="drehzahlEingabe">
          <label>Drehzahl (RPM):</label><br>
          <input type="number" name="speed" min="0" max="10000"><br><br>
        </div>
        <input type="submit" class="btn" value="Setzen">
      </form>
      <br>
      <button class="btn" onclick="sendCommand('/startr')">Motor Rechtslauf</button>
      <button class="btn" onclick="sendCommand('/startl')">Motor Linkslauf</button>
      <button class="btn" onclick="sendCommand('/stop')">Motor Stop</button>
      <!--
      <button class="btn" onclick="sendCommand('/stop')">Motor Stoppen</button>
      <button id="autoStartBtn" class="btn" onclick="sendCommand('/auto_start', true)">Automatik Start</button>
      <button class="btn" onclick="sendCommand('/auto_stop')">Automatik Stop</button>
      <button class="btn" onclick="sendCommand('/status')">Status abfragen</button>
      -->
      <br><br>
      <div id="infoText"></div>
      <!--
      <div id="autoStatus" style="margin-top:20px; font-weight:bold;">Automationsstatus: Wird geladen...</div>
      -->
    </div>
    <script>
    function sendCommand(url, checkStatus = false) {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", url, true);
    xhr.onload = function () {
      document.getElementById("infoText").innerText = xhr.responseText;
      if (checkStatus) updateAutomationStatus();  // z.B. bei auto_start
    };
    xhr.send();
  }

      function modusChanged() {
        var extern = document.querySelector('input[name="modus"]:checked').value === "extern";
        document.getElementById('drehzahlEingabe').style.display = extern ? "none" : "block";
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/set_mode?extern=" + (extern?1:0), true);
        xhr.onload = function(){document.getElementById("infoText").innerText = xhr.responseText;}
        xhr.send();
      }

      function updateAutomationStatus() {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/automation_status", true);
      xhr.onload = function() {
      if (xhr.status === 200) {
        const status = xhr.responseText;
        document.getElementById("autoStatus").innerText = "Automationsstatus: " + status;

        // Button-Farbe anpassen
        const btn = document.getElementById("autoStartBtn");
        if (status === "STARTED" || status === "DIRECTION_CHANGED") {
          btn.classList.add("active");
        } else {
          btn.classList.remove("active");
        }
      }
  };
  xhr.send();
}


      setInterval(updateAutomationStatus, 2000); // Alle 2 Sekunden aktualisieren
      window.onload = updateAutomationStatus;
    </script>
  </body>
  </html>
)rawliteral";
    request->send(200, "text/html", html);
  });

  


  server.on("/auto_start", HTTP_GET, [](AsyncWebServerRequest *request){
    startAutomationSequence();
    showDisplay("Automatik", "Sequenz gestartet");
    request->send(200, "text/plain", "Automatik gestartet");
  });

  server.on("/auto_stop", HTTP_GET, [](AsyncWebServerRequest *request){
    stopAutomationSequence();
    showDisplay("Automatik", "Sequenz gestoppt");
    request->send(200, "text/plain", "Automatik gestoppt");
  });

  server.on("/automation_status", HTTP_GET, [](AsyncWebServerRequest *request){
  State s = getCurrentAutomationState();
  String statusText;

  switch (s) {
    case IDLE: statusText = "IDLE"; break;
    case STARTED: statusText = "STARTED"; break;
    case DIRECTION_CHANGED: statusText = "DIRECTION_CHANGED"; break;
    case STOPPED: statusText = "STOPPED"; break;
    default: statusText = "UNBEKANNT"; break;
  }

  request->send(200, "text/plain", statusText);
  });


  // ==== Drehzahl-Modus setzen (extern/intern) ====
  server.on("/set_mode", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("extern")) {
      externeVorgabe = request->getParam("extern")->value().toInt() == 1;
      letztePottiDrehzahl = -1; // Wertangabe zurücksetzen
      showDisplay("Modus gewechselt", externeVorgabe ? "Extern (Poti)" : "Intern (Web)");
      request->send(200, "text/plain", externeVorgabe ? "Modus: Extern (Poti)" : "Modus: Intern (Web)");
    } else {
      request->send(400, "text/plain", "Fehlender Parameter");
    }
  });

  // ==== Drehzahl setzen per Web ====
  server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!externeVorgabe) { // Nur wenn interne Vorgabe aktiv
      if (request->hasParam("speed")) {
        String speed = request->getParam("speed")->value();
        // Setze Drehzahl an Motorsteuerung
        String cmd = "SET_SPEED:" + speed + "\n";
        Serial1.print(cmd);
        Serial.println("UART: " + cmd);
        showDisplay("Drehzahl gesetzt", "RPM: " + speed);
        request->send(200, "text/plain", "Drehzahl gesetzt auf: " + speed);
      } else {
        showDisplay("Fehler", "Kein Speed");
        request->send(400, "text/plain", "Fehlender Parameter");
      }
    } else {
      request->send(200, "text/plain", "Im externen Modus ist die Vergabe gesperrt!");
    }
  });

  // ==== Start ====
  server.on("/startl", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial1.print("STARTL\n");
    Serial.print("UART: STARTL\n");
    showDisplay("Motor gestartet Linkslauf");
    request->send(200, "text/plain", "Motor gestartet Linkslauf");
  });

  // ==== Start ====
  server.on("/startr", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial1.print("STARTR\n");
    Serial.print("UART: STARTR\n");
    showDisplay("Motor gestartet Rechslauf");
    request->send(200, "text/plain", "Motor gestartet Rechtslauf");
  });

  // ==== Stop ====
  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial1.print("STOP\n");
    Serial.print("UART: STOP\n");
    showDisplay("Motor gestoppt");
    request->send(200, "text/plain", "Motor gestoppt");
  });

  // ==== Status ====
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial1.print("STATUS?\n");
    Serial.print(("UART: STATUS?\n"));
    delay(100); // Kurze Wartezeit für Antwort
    String status = "";
    while (Serial1.available()) {
      status += (char)Serial1.read();
    }
    if (status.length() == 0) status = "Keine Antwort vom Dev Kit.";
    showDisplay("Status:", status.substring(0, 20));
    request->send(200, "text/plain", "Status:\n" + status);
  });

  server.begin();

  // ==== ADC (Potentiometer) vorbereiten ====
  analogReadResolution(12);        // 0...4095 für ESP32
  pinMode(POTT_PIN, INPUT);        // Potentiometer-Pin als Eingang

  // ==== Automatisierung initialisieren ====
  initAutomation();

}

// ==== HAUPTSCHLEIFE ====
void loop() {

  // --- Externe Vorgabe: Potentiometer abfragen und an Motor-Steuerung senden ---
  if(externeVorgabe) {
    long summe = 0;
    const int N = 500;
    for(int i=0; i<N; i++)
      summe += analogRead(POTT_PIN);
    int pottiWert = summe / N;
    // Serial.println("Poti: " + String(pottiWert));
    int rpm = map(pottiWert, 0, 4095, 800, 2200);            // Auf RPM umrechnen
    // int rpm = (rpm_raw / 50) * 50; // Rundung auf nächste 50 (0, 50, 100, 150, ..., 1500)
    // if(rpm > 1500) rpm = 1500; // Maximalwert beschränken
    // Nur bei signifikanter Änderung senden (Schwelle z.B. 50 U/min)
    if ((abs(rpm - letztePottiDrehzahl) > 5) || (rpm == 0) || (rpm == 1500) && (letztePottiDrehzahl != 0) && (letztePottiDrehzahl != 1500)) {
      String cmd = "SET_SPEED:" + String(rpm) + "\n";
      Serial1.print(cmd);
      Serial.print("UART: " + cmd);
      showDisplay("Poti Drehzahl", "RPM: " + String(rpm));
      letztePottiDrehzahl = rpm;
    }
    delay(100); // Abfrageinterval (anpassen bei Bedarf!)
  }
  // --- Automatisierung aktualisieren ---
    updateAutomation();
  // Der Webserver läuft asynchron (kein weiteres Handling nötig)
}
