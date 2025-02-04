#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32QRCodeReader.h>

#define WIFI_SSID "_thephuuu"
#define WIFI_PASSWORD "00000000"
#define WEBHOOK_URL "https://coding-project-api.vercel.app/api/iot/check-qr"
#define FLASH_LIGHT 4
const String ESP8266_IP = "http://172.20.10.3";

String lastQRCode = ""; // QR Code ล่าสุดที่อ่านได้
unsigned long lastReadTime = 0; // เวลาที่อ่าน QR Code ล่าสุด
const unsigned long readCooldown = 5000; // ช่วงเวลาในการหน่วง (มิลลิวินาที)

ESP32QRCodeReader reader(CAMERA_MODEL_AI_THINKER);
struct QRCodeData qrCodeData;
bool isConnected = false;
bool isProcessing = false;

void blinkFlashLight(int times, int interval) {
  for (int i = 0; i < times; i++) {
    digitalWrite(FLASH_LIGHT, HIGH);
    delay(interval);
    digitalWrite(FLASH_LIGHT, LOW);
    delay(interval);
  }
}

void openDoor() {
  HTTPClient http;
  String url = ESP8266_IP + "/open";
  Serial.print("Sending request to: ");
  Serial.println(url);
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpCode);
    String response = http.getString();
    Serial.println("Response: " + response);
  } else {
    Serial.print("Error on HTTP request: ");
    Serial.println(http.errorToString(httpCode));
  }
  http.end();
}

void closeDoor() {
  HTTPClient http;
  String url = ESP8266_IP + "/close";
  Serial.print("Sending request to: ");
  Serial.println(url);
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpCode);
    String response = http.getString();
    Serial.println("Response: " + response);
  } else {
    Serial.print("Error on HTTP request: ");
    Serial.println(http.errorToString(httpCode));
  }
  http.end();
}

bool connectWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(FLASH_LIGHT, LOW);
    sendToESP8266("ESP32WifiConnected");
    return true;
  }
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int maxRetries = 10;
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(FLASH_LIGHT, HIGH);
    delay(500);
    Serial.print(".");
    digitalWrite(FLASH_LIGHT, LOW);
    delay(500);
    maxRetries--;
    if (maxRetries <= 0) {
      return false;
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");
  sendToESP8266("ESP32WifiConnected");
  return true;
}

bool isDuplicateQRCode(String currentQRCode) {
  return (currentQRCode == lastQRCode && millis() - lastReadTime < readCooldown);
}

void sendToESP8266(String text) {
  HTTPClient http;
  String url = ESP8266_IP + "/display?text=" + text;
  Serial.print("Sending text to ESP8266: ");
  Serial.println(url);
  
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    Serial.print("ESP8266 Response: ");
    Serial.println(httpCode);
  } else {
    Serial.print("Error: ");
    Serial.println(http.errorToString(httpCode));
  }
  http.end();
}

void callWebhook(String code) {
  HTTPClient http;
  http.begin(WEBHOOK_URL);
  http.addHeader("Content-Type", "application/json");

  Serial.print("Sending payload: ");
  Serial.println(code);

  int httpCode = http.POST(code);
  if (httpCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpCode);
    String response = http.getString();
    Serial.println("Response from server: " + response);

    if (httpCode == 200) {
      Serial.println("Access granted: Opening door...");
      sendToESP8266("AccessGranted"); // ส่งข้อความไป ESP8266
      openDoor();
      delay(5000);
      closeDoor();
    } else {
      Serial.println("Access denied: Door remains locked.");
      sendToESP8266("Denied_TryAgain");
      closeDoor();
    }
  } else {
    Serial.print("Error on HTTP request: ");
    Serial.println(http.errorToString(httpCode));
    sendToESP8266("ConnectionError");
    closeDoor();
  }

  http.end();
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  pinMode(FLASH_LIGHT, OUTPUT);
  digitalWrite(FLASH_LIGHT, LOW);

  reader.setup();
  Serial.println("Setup QRCode Reader");
  reader.begin();
  Serial.println("Begin QR Code reader");

  delay(1000);
}

void loop() {
  if (!isConnected) {
    isConnected = connectWifi();
    if (!isConnected) {
      digitalWrite(FLASH_LIGHT, HIGH);
      delay(300);
      digitalWrite(FLASH_LIGHT, LOW);
      delay(300);
      return;
    }
  }

  if (isProcessing) {
    return;
  }

  if (reader.receiveQrCode(&qrCodeData, 100)) {
    if (qrCodeData.valid) {
      String currentQRCode = String((const char *)qrCodeData.payload);

      if (isDuplicateQRCode(currentQRCode)) {
        Serial.println("Duplicate QR Code detected. Ignoring...");
        return;
      }

      lastQRCode = currentQRCode;
      lastReadTime = millis();

      isProcessing = true;
      Serial.println("Found QR Code");
      sendToESP8266("FoundQrCode");
      blinkFlashLight(2, 200);

      Serial.print("Payload: ");
      Serial.println(currentQRCode);

      callWebhook(currentQRCode);
      isProcessing = false;
    } else {
      Serial.println("Invalid QR Code");
      sendToESP8266("InvalidQrCode");
      blinkFlashLight(3, 100);
    }
  }
}
