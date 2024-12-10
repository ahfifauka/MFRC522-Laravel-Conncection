#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Base64.h>
#include <ArduinoJson.h>

// pin Map
// SDA D2
// SCK D5
// MOSI D7
// MISO D6
// RST D1


#define RST_PIN D1
#define SDA_PIN D2

// Adjust your connection
const char* ssid = "SSID";
const char* password = "PASSWORD";

// laravel route
const char* serverName = "http://IP/rfidiot/public/rfid/get_id";

MFRC522 mfrc522(SDA_PIN, RST_PIN);  // Initialization MFRC522

WiFiClient client;  // Objek WiFiClient for HTTP Connection

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }

  Serial.println("Connected to network");

  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("Put your card to the reader...");
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Get UID From RFID Card
  String content = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println("UID in HEX: " + content);

  // this step is optional, if you want send the Uid to Base64 Format 
  // Convert UID ke Base64
  byte uidBuffer[mfrc522.uid.size];
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uidBuffer[i] = mfrc522.uid.uidByte[i];
  }
  String base64Encoded = base64::encode(uidBuffer, mfrc522.uid.size);
  Serial.println("UID in Base64: " + base64Encoded);

  sendBase64ToServer(base64Encoded);

  delay(2000);
}

void sendBase64ToServer(String base64Encoded) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // make query string for send rfid card tag data
    String url = String(serverName) + "?rfid_tag=" + urlencode(base64Encoded);
    Serial.println("Mengirim request ke server: " + url);

    http.begin(client, url);

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();  // get respons from server
      Serial.println("Respons dari server:");
      Serial.println(payload);  // show all JSON Response

      // Parsing JSON respons for get value from key "rfid_tag"
      deserializeJsonResponse(payload);
    } else {
     
      Serial.println("Error Sending GET request");
    }

    http.end();
  } else {
    Serial.println("Not Connection Network");
  }
}

void deserializeJsonResponse(String payload) {
  // create a JSON object to hold the received data
  DynamicJsonDocument doc(1024);  // Set buffer JSON Size

  // Parsing JSON
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print("Parsing JSON gagal: ");
    Serial.println(error.f_str());
    return;
  }

  // get value of 'rfid_tag' from JSON
  const char* rfidTag = doc["tag"];
  const char* saldo = doc["saldo"];
  
  Serial.println(rfidTag);
  Serial.println(saldo);
}

// URL Decode Function
String urlencode(String str) {
  String encoded = "";
  char c;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else {
      encoded += "%" + String((int)c, HEX);
    }
  }
  return encoded;
}
