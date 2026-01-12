#include <WiFi.h>
#include <PubSubClient.h>
#include <Ascon128.h>

/* ====== WiFi ====== */
const char* ssid = "AhmedCh";
const char* password = "chayma...";

/* ====== MQTT ====== */
const char* mqtt_server = "192.168.141.56";
const int   mqtt_port   = 1883;
const char* mqtt_user   = "esp32user";
const char* mqtt_pass   = "chayma2002";
const char* topic       = "iot/soil/humidity";

/* ====== Objects ====== */
WiFiClient espClient;
PubSubClient client(espClient);
Ascon128 ascon;

/* ====== ASCON key (128-bit) ====== */
uint8_t key[16] = {
  0x00,0x01,0x02,0x03,
  0x04,0x05,0x06,0x07,
  0x08,0x09,0x0A,0x0B,
  0x0C,0x0D,0x0E,0x0F
};

/* ====== Utils ====== */
String toHex(const uint8_t *data, size_t len) {
  String s = "";
  for (size_t i = 0; i < len; i++) {
    if (data[i] < 16) s += "0";
    s += String(data[i], HEX);
  }
  return s;
}

/* ====== MQTT connect ====== */
void connectMQTT() {
  while (!client.connected()) {
    Serial.print("🔌 Connexion MQTT...");
    if (client.connect("ESP32_SOIL", mqtt_user, mqtt_pass)) {
      Serial.println(" OK");
    } else {
      Serial.print(" ÉCHEC, rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  /* WiFi */
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n📶 WiFi connecté");

  /* MQTT */
  client.setServer(mqtt_server, mqtt_port);
  client.setBufferSize(512);  // important pour JSON + crypto

  /* Random seed (CRITIQUE pour IV) */
  randomSeed(esp_random());
}

void publishHumidity(int humidity) {
  char plaintext[32];
  snprintf(plaintext, sizeof(plaintext), "humidity=%d", humidity);

  uint8_t iv[16];
  for (int i = 0; i < 16; i++) {
    iv[i] = random(0, 256);
  }

  uint8_t ciphertext[32];
  uint8_t tag[16];

  ascon.setKey(key, sizeof(key));
  ascon.setIV(iv, sizeof(iv));

  ascon.encrypt(ciphertext, (uint8_t*)plaintext, strlen(plaintext));
  ascon.computeTag(tag, sizeof(tag));

  String payload = "{";
  payload += "\"iv\":\"" + toHex(iv,16) + "\",";
  payload += "\"cipher\":\"" + toHex(ciphertext, strlen(plaintext)) + "\",";
  payload += "\"tag\":\"" + toHex(tag,16) + "\"";
  payload += "}";

  client.publish(topic, payload.c_str());
  Serial.println("📤 Donnée chiffrée envoyée");
}

void loop() {
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();

  int humidity = analogRead(34);
  publishHumidity(humidity);

  delay(5000);
}
