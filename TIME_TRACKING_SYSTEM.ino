#ifdef ESP32
#include <WiFi.h>
#include <HTTPClient.h>
#else
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#endif

#include "time.h"
#include <Wire.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <LiquidCrystal_I2C.h>
#include <Arduino.h>
#include <SPI.h>
#include <mfrc630.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
LiquidCrystal_I2C lcd(0x27, 20, 4);

//const char* ssid          = "Amil Phone";                                                                 //network credentials
const char* ssid          = "Brkic_Net_24G";                                                            //network credentials
const char* password      = "Amil1205.";                                                                  //network password

const char* serverName    = "https://timetrackingsystem.net/post-esp-data.php";                           //Domain name and URL path
String apiKeyValue        = "tPmAT5Ab3j7F9";                                                              //API Key value
String SENSOR             = "RFID Reader";
String LOCATION           = "In School";
String value2             = " ";
String value3             = " ";
String stringOne          = " ";
String CARD_ID            = " ";

#define CHIP_SELECT 5
#define blue_RGB 33
#define green_RGB 25
#define red_RGB 34
#define RST_NFC 17

const int buzzer = 16;
const int buzzer_freq = 4000;
const int buzzer_Channel = 0;
const int buzzer_resolution = 8;
unsigned long buzzer_previousMillis = 0;
const long buzzer_interval = 800;
int buzzer_State = LOW;

void mfrc630_SPI_transfer(const uint8_t* tx, uint8_t* rx, uint16_t len) {
  for (uint16_t i = 0; i < len; i++) {
    rx[i] = SPI.transfer(tx[i]);
  }
}
void mfrc630_SPI_select() {
  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));  // gain control of SPI bus
  digitalWrite(CHIP_SELECT, LOW);
}

void mfrc630_SPI_unselect() {
  digitalWrite(CHIP_SELECT, HIGH);
  SPI.endTransaction();    // release the SPI bus
}

void print_block(uint8_t * block, uint8_t length) {
  for (uint8_t i = 0; i < length; i++) {
    if (block[i] < 16) {
      Serial.print("0");
      Serial.print(block[i], HEX);
    } else {
      Serial.print(block[i], HEX);
    }
    Serial.print(" ");
  }
}

void setup() {

  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  
  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Time Tracking System");
  timeClient.begin();
  timeClient.setTimeOffset(3600);
  ledcSetup(buzzer_Channel, buzzer_freq, buzzer_resolution);
  ledcAttachPin(buzzer, buzzer_Channel);

  pinMode(CHIP_SELECT, OUTPUT);
  pinMode(blue_RGB, OUTPUT);
  pinMode(green_RGB, OUTPUT);
  pinMode(red_RGB, OUTPUT);
  digitalWrite(blue_RGB, HIGH);
  digitalWrite(green_RGB, LOW);
  digitalWrite(red_RGB, LOW);
  digitalWrite(RST_NFC, LOW);
  SPI.begin();

  mfrc630_AN1102_recommended_registers(MFRC630_PROTO_ISO14443A_106_MILLER_MANCHESTER);
  mfrc630_write_reg(0x28, 0x8E);
  mfrc630_write_reg(0x29, 0x15);
  mfrc630_write_reg(0x2A, 0x11);
  mfrc630_write_reg(0x2B, 0x06);
}

void loop(){ 
      
  timeClient.update();
  lcd.setCursor(12, 1);
  lcd.print(timeClient.getFormattedTime());
  lcd.setCursor(10, 2);
  lcd.print("07.02.2022");
  lcd.setCursor(0, 3);
  lcd.print("Please Card");

  uint16_t atqa = mfrc630_iso14443a_REQA();
  if (atqa != 0) {  // Are there any cards that answered?
    uint8_t sak;
    uint8_t uid[10] = {0};  // uids are maximum of 10 bytes long.

    // Select the card and discover its uid.
    uint8_t uid_len = mfrc630_iso14443a_select(uid, &sak);

    if (uid_len != 0) {  // did we get an UID?
      Serial.print("UID of ");
      Serial.print(uid_len);
      Serial.print(" bytes (SAK: ");
      Serial.print(sak);
      Serial.print("): ");
      print_block(uid, uid_len);
      Serial.print("\n");

      // Use the manufacturer default key...
      uint8_t FFkey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

      mfrc630_cmd_load_key(FFkey);  // load into the key buffer
      CARD_ID = " ";
      Serial.print("Card ID 1: ");
      for (int i = 0; i < uid_len; i++) {
        //Serial.print(uid[i], HEX);
        stringOne = String(uid[i], HEX);
        Serial.print(stringOne);
        CARD_ID = CARD_ID + stringOne;
      }
      CARD_ID.toUpperCase();
      Serial.print("\n");
      Serial.print("Card ID 2:");
      Serial.print(CARD_ID);
      Serial.print("\n");

      if (mfrc630_MF_auth(uid, MFRC630_MF_AUTH_KEY_A, 0)) {
        digitalWrite(blue_RGB, LOW);
        delay(10);
        digitalWrite(green_RGB, HIGH);
        Serial.println("Yay! We are authenticated!");
        ledcWriteTone(buzzer_Channel, 2000.000);
        buzzer_State = HIGH;
        delay(300);
        ledcWriteTone(buzzer_Channel, 0);
        buzzer_State = LOW;
        mfrc630_MF_deauth();  // be sure to call this after an authentication!
        //Check WiFi connection status
        
        HTTPClient http;
        http.begin(serverName);                                                                               // Your Domain name with URL path or IP address with path
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");                                  // Specify content-type header
        String httpRequestData = "api_key=" + apiKeyValue +
                                 "&sensor=" + SENSOR +
                                 "&location=" + LOCATION +
                                 "&CARD_ID=" + CARD_ID +
                                 "&value2=" + value2 +
                                 "&value3=" + value3 + "";

        Serial.print("httpRequestData: ");
        Serial.println(httpRequestData);
        
        int httpResponseCode = http.POST(httpRequestData);                                                    // Send HTTP POST request
        digitalWrite(green_RGB, LOW);
        delay(10);
        digitalWrite(blue_RGB, HIGH);
        
        if (httpResponseCode > 0) {
          Serial.print("HTTP Response code: ");
          Serial.println(httpResponseCode);
        }
        else {
          Serial.print("Error code: ");
          Serial.println(httpResponseCode);
        }

        http.end();
        
      } else {
        digitalWrite(blue_RGB, LOW);
        digitalWrite(green_RGB, LOW);
        delay(10);
        digitalWrite(red_RGB, HIGH);
        ledcWriteTone(buzzer_Channel, 2000.000);
        buzzer_State = HIGH;
        delay(100);
        ledcWriteTone(buzzer_Channel, 0);
        buzzer_State = LOW;
        delay(100);
        ledcWriteTone(buzzer_Channel, 2000.000);
        buzzer_State = HIGH;
        delay(100);
        ledcWriteTone(buzzer_Channel, 0);
        buzzer_State = LOW;
        delay(100);
        ledcWriteTone(buzzer_Channel, 2000.000);
        buzzer_State = HIGH;
        delay(100);
        ledcWriteTone(buzzer_Channel, 0);
        buzzer_State = LOW;
        delay(100);
        digitalWrite(red_RGB, LOW);
        delay(10);
        digitalWrite(blue_RGB, HIGH);
        
        Serial.print("Could not authenticate :(\n");
      }
    } else {
      ledcWriteTone(buzzer_Channel, 2000.000);
      buzzer_State = HIGH;
      delay(100);
      ledcWriteTone(buzzer_Channel, 0);
      buzzer_State = LOW;
      delay(100);
      ledcWriteTone(buzzer_Channel, 2000.000);
      buzzer_State = HIGH;
      delay(100);
      ledcWriteTone(buzzer_Channel, 0);
      buzzer_State = LOW;
      delay(100);
      ledcWriteTone(buzzer_Channel, 2000.000);
      buzzer_State = HIGH;
      delay(100);
      ledcWriteTone(buzzer_Channel, 0);
      buzzer_State = LOW;
      delay(100);
      Serial.print("Could not determine UID, perhaps some cards don't play");
      Serial.print(" well with the other cards? Or too many collisions?\n");
    }

  } else {
    Serial.print("No answer to REQA, no cards?\n");
  }

  delay(300);

}
