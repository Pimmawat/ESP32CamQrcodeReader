#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// ตั้งค่าหน้าจอ LCD ที่ Address 0x3F ขนาด 16x4
LiquidCrystal_I2C lcd(0x3F, 16, 4);

// ข้อมูล Wi-Fi
const char* ssid = "_thephuuu";
const char* password = "00000000";

// พินที่ต่อกับรีเลย์
const int relayPin = D3;

// ตั้งค่า Web Server ที่พอร์ต 80
ESP8266WebServer server(80);

void setup() {
  Serial.begin(115200);

  // ตั้งค่าหน้าจอ LCD
  lcd.begin();
  lcd.backlight();

  // แสดง "Booting..." กลางจอ
  lcd.setCursor(3, 0);
  lcd.print("Booting...");
  delay(2000);
  lcd.clear();

  // แสดง "Connecting to Wi-Fi..." กลางจอ
  lcd.setCursor(1, 0);
  lcd.print("Connecting to");
  lcd.setCursor(4, 1);
  lcd.print("Wi-Fi...");

  // ตั้งค่าพินรีเลย์
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW); // ตั้งค่าเริ่มต้นเป็นปิดกลอนประตู

  // เชื่อมต่อ Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to Wi-Fi...");
  }

  Serial.println("Wi-Fi connected");

  // แสดง "Wi-Fi Connected!" กลางจอ
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Wi-Fi Connected!");

  // แสดง IP Address กลางจอ
  lcd.setCursor(1, 1);
  lcd.print("IP Address:");
  lcd.setCursor(0, 2);
  lcd.print(WiFi.localIP().toString());

  delay(3000); // แสดง IP 3 วินาที แล้วล้างหน้าจอ
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("Ready");

  // ตั้งค่าเส้นทาง `/open` สำหรับเปิดประตู
  server.on("/open", []() {
    digitalWrite(relayPin, HIGH); // เปิดรีเลย์
    Serial.println("Door opened.");

    // เคลียร์เฉพาะบรรทัดที่ 2 แล้วแสดงข้อความ
    lcd.setCursor(0, 1);
    lcd.print("                "); // ล้างข้อความเดิม
    lcd.setCursor(0, 1);
    lcd.print("Door Opened");

    server.send(200, "text/plain", "Door opened");
  });

  // ตั้งค่าเส้นทาง `/close` สำหรับปิดประตู
  server.on("/close", []() {
    digitalWrite(relayPin, LOW); // ปิดรีเลย์
    Serial.println("Door closed.");

    // เคลียร์เฉพาะบรรทัดที่ 2 แล้วแสดงข้อความ
    lcd.setCursor(0, 1);
    lcd.print("                "); // ล้างข้อความเดิม
    lcd.setCursor(0, 1);
    lcd.print("Door Closed");

    server.send(200, "text/plain", "Door closed");
  });

  // ตั้งค่าเส้นทาง `/display` สำหรับแสดงข้อความที่บรรทัดที่ 1
  server.on("/display", []() {
    if (server.hasArg("text")) {
      String text = server.arg("text");
      Serial.println("Display: " + text);

      // เคลียร์เฉพาะบรรทัดที่ 1 แล้วแสดงข้อความ
      lcd.setCursor(0, 0);
      lcd.print("                    "); // ล้างข้อความเดิม
      lcd.setCursor(0, 0);
      lcd.print(text);

      server.send(200, "text/plain", "Displayed: " + text);
    } else {
      server.send(400, "text/plain", "No text provided");
    }
  });

  // เริ่มต้น Web Server
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();
}
