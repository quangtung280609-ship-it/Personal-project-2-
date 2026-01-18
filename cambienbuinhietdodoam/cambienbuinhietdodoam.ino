#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ThingSpeak.h>
#include <Adafruit_AHTX0.h>

/* ===== PM SENSOR ===== */
SoftwareSerial mySerial(D4, D3); // TX, RX
int pm1 = 0;
int pm2_5 = 0;
int pm10 = 0;

/* ===== AHT SENSOR ===== */
Adafruit_AHTX0 aht;
float temperature = 0;
float humidity = 0;

/* ===== WIFI & THINGSPEAK ===== */
const char* ssid = "BKSTAR";
const char* password = "stemstar";

unsigned long channelID = 3202138;
const char* writeAPIKey = "V2Z2DBEAE8GKMWC9";

WiFiClient client;

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);

  /* ===== AHT INIT ===== */
  if (!aht.begin()) {
    Serial.println("AHT sensor not found");
    while (1);
  }
  Serial.println("AHT sensor ready");

  /* ===== WIFI ===== */
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

  ThingSpeak.begin(client);
}

void loop() {
  /* ===== READ AHT ===== */
  sensors_event_t humEvent, tempEvent;
  aht.getEvent(&humEvent, &tempEvent);

  temperature = tempEvent.temperature;
  humidity = humEvent.relative_humidity;

  /* ===== READ PM ===== */
  int index = 0;
  char value;
  char previousValue;

  while (mySerial.available()) {
    value = mySerial.read();

    if ((index == 0 && value != 0x42) || (index == 1 && value != 0x4D)) {
      Serial.println("PM header error");
      break;
    }

    if (index == 4 || index == 6 || index == 8) {
      previousValue = value;
    }
    else if (index == 5) {
      pm1 = 256 * previousValue + value;
    }
    else if (index == 7) {
      pm2_5 = 256 * previousValue + value;
    }
    else if (index == 9) {
      pm10 = 256 * previousValue + value;
    }
    else if (index > 15) break;

    index++;
  }
  while (mySerial.available()) mySerial.read();

  /* ===== SERIAL OUTPUT ===== */
  Serial.println("======= DATA =======");
  Serial.print("Temp: "); Serial.print(temperature); Serial.println(" °C");
  Serial.print("Humidity: "); Serial.print(humidity); Serial.println(" %RH");
  Serial.print("PM1: "); Serial.print(pm1); Serial.println(" ug/m3");
  Serial.print("PM2.5: "); Serial.print(pm2_5); Serial.println(" ug/m3");
  Serial.print("PM10: "); Serial.print(pm10); Serial.println(" ug/m3");
  Serial.println("====================");

  /* ===== SEND TO THINGSPEAK ===== */
  ThingSpeak.setField(1, temperature);
  ThingSpeak.setField(2, humidity);
  ThingSpeak.setField(3, pm1);
  ThingSpeak.setField(4, pm2_5);
  ThingSpeak.setField(5, pm10);

  int status = ThingSpeak.writeFields(channelID, writeAPIKey);
  if (status == 200) {
    Serial.println("ThingSpeak update success");
  } else {
    Serial.print("ThingSpeak error: ");
    Serial.println(status);
  }

  delay(20000); // >= 15s theo ThingSpeak
}
