/*
 * IniK-Box - Name auf dem Display, RGB-LED blinkt, Taster steuern den Takt
 * Informatik im Kontext, WiSe 26/27
 *
 * Hardware: Carenuity C3 Mini auf der Platine IniK-Box v1.3
 * In der Arduino IDE als Board "LOLIN C3 MINI" auswaehlen.
 *
 * Pin        Funktion
 * ---------- ------------------------------------------
 * GPIO 4     schwarzer Taster, gegen GND
 * GPIO 5     roter Taster, gegen GND
 * GPIO 7     RGB-LED (WS2812B) auf dem C3 Mini
 * GPIO 8     SDA (I2C)
 * GPIO 10    SCL (I2C)
 *
 * Benoetigte Bibliotheken (Bibliotheksverwalter der Arduino IDE):
 * U8g2, Adafruit NeoPixel
 *
 * -------------------------------------------------------------------------
 * WARUM WUERDE DAS MIT delay() NICHT FUNKTIONIEREN? (Begruendung)
 * -------------------------------------------------------------------------
 * Ein Aufruf von delay(interval) haelt den gesamten Mikrocontroller fuer
 * die angegebene Zeit komplett an ("blockierender Code"). Waehrend dieser
 * Wartezeit werden keine weiteren Codezeilen ausgefuehrt – der Controller
 * ist quasi "blind und taub".
 *
 * Konkret bedeutet das:
 * 1. Drueckt man einen Taster waehrend der Blinkpause, bemerkt der ESP32
 *    das gar nicht, weil er in delay() feststeckt.
 * 2. Besonders bei langen Intervallen (z. B. 1,5 Sekunden) muesste man
 *    den Taster extrem lange gedrueckt halten und genau den winzigen
 *    Moment abpassen, in dem delay() gerade vorbei ist.
 * 3. Mit millis() laeuft die loop()-Schleife hingegen tausende Male pro
 *    Sekunde ungebremst durch: Der Controller prueft staendig die Taster
 *    und schaut nebenbei auf die Uhr, ob das Intervall schon um ist.
 * -------------------------------------------------------------------------
 */

#include <Wire.h>
#include <U8g2lib.h>
#include <Adafruit_NeoPixel.h>

U8G2_SSD1306_64X48_ER_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

const int PIN_SCHW = 4;   // schwarz
const int PIN_ROT  = 5;   // rot
const int PIN_LED  = 7;   // RGB-LED

Adafruit_NeoPixel led(1, PIN_LED, NEO_GRB + NEO_KHZ800);

unsigned long previousMillis = 0;
unsigned long interval = 300;   // Millisekunden je Phase
bool ledState = false;          // leuchtet die LED gerade?
int farbe = 0;                  // 0 rot, 1 gruen, 2 blau

// Variablen fuer die Taster-Steuerung und Entprellung
const unsigned long MIN_INTERVAL = 50;    // schnellstes Blinken (50 ms)
const unsigned long MAX_INTERVAL = 1500;  // langsamstes Blinken (1500 ms)
const unsigned long SCHRITT = 50;         // Aenderung pro Tastendruck

bool gestoppt = false;                    // Zusatz: Blinken am langsamen Ende gestoppt?
bool letzterStatusSchw = HIGH;            // vorheriger Zustand schwarzer Taster
bool letzterStatusRot  = HIGH;            // vorheriger Zustand roter Taster
unsigned long letzteTasterZeit = 0;       // zum Entprellen der Taster

void zeigeFarbe() {
  if (!ledState || gestoppt) {
    led.clear();
  } else if (farbe == 0) {
    led.setPixelColor(0, led.Color(255, 0, 0));
  } else if (farbe == 1) {
    led.setPixelColor(0, led.Color(0, 255, 0));
  } else {
    led.setPixelColor(0, led.Color(0, 0, 255));
  }
  led.show();
}

void setup() {
  Wire.begin(8, 10);

  pinMode(PIN_SCHW, INPUT_PULLUP);
  pinMode(PIN_ROT,  INPUT_PULLUP);

  display.begin();
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);

  // Name mittig auf dem 64x48 Display ausgeben
  const char* name = "Florin";
  int x = (64 - display.getStrWidth(name)) / 2;
  int y = (48 + display.getAscent()) / 2;
  display.setCursor(x, y);
  display.print(name);
  display.sendBuffer();

  led.begin();
  led.setBrightness(40);        // volle Helligkeit blendet
  led.clear();
  led.show();
}

void loop() {
  unsigned long currentMillis = millis();

  // Aktuellen Zustand der Taster einlesen (LOW = gedrueckt wegen INPUT_PULLUP)
  bool statusSchw = digitalRead(PIN_SCHW);
  bool statusRot  = digitalRead(PIN_ROT);

  // Taster nur alle 30 ms abfragen (Entprellen verhindert Mehrfach-Ausloesung)
  if (currentMillis - letzteTasterZeit >= 30) {
    
    // 1. Schwarzer Taster wurde neu gedrueckt (Wechsel von HIGH auf LOW)
    if (statusSchw == LOW && letzterStatusSchw == HIGH) {
      letzteTasterZeit = currentMillis;

      if (gestoppt) {
        // Zusatzaufgabe: Blinken wieder starten
        gestoppt = false;
        interval = MAX_INTERVAL;
        previousMillis = currentMillis;
      } else if (interval > MIN_INTERVAL) {
        // Schneller blinken -> Intervall verkleinern
        interval -= SCHRITT;
      }
    }

    // 2. Roter Taster wurde neu gedrueckt (Wechsel von HIGH auf LOW)
    if (statusRot == LOW && letzterStatusRot == HIGH) {
      letzteTasterZeit = currentMillis;

      if (!gestoppt) {
        if (interval < MAX_INTERVAL) {
          // Langsamer blinken -> Intervall vergroessern
          interval += SCHRITT;
        } else {
          // Zusatzaufgabe: Am langsamen Ende Blinken ganz anhalten
          gestoppt = true;
          ledState = false;
          zeigeFarbe();
        }
      }
    }

    // Aktuelle Zustaende fuer den naechsten Durchlauf merken
    letzterStatusSchw = statusSchw;
    letzterStatusRot  = statusRot;
  }

  // Blink-Logik (laeuft nur, wenn nicht gestoppt)
  if (!gestoppt && (currentMillis - previousMillis >= interval)) {
    previousMillis = currentMillis;
    ledState = !ledState;
    if (ledState) {
      farbe = (farbe + 1) % 3;
    }
    zeigeFarbe();
  }
}
