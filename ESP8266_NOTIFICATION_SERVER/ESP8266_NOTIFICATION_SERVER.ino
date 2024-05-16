#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "HtmlStyleConfig.h"
#include "SoftwareSerial.h"
#include "DFPlayerMini_Fast.h"
#include "AudioDataIndex.h"

// Set your Static IP address
IPAddress localIP_AP(192, 168, 200, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress gatewayWifi(192, 168, 1, 1);

// Use pins 2 and 3 to communicate with DFPlayer Mini
static const uint8_t PIN_MP3_TX = D6; // Connects to module's RX
static const uint8_t PIN_MP3_RX = D3; // Connects to module's TX

#define RX_ESPCAM 10
#define TX_ESPCAM 9

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_MOSI 13
#define OLED_CLK 14
#define OLED_DC 5
#define OLED_CS 15
#define OLED_RESET 4

const int MAX_TIMES_TRY_CONNECT_WIFI = 100;
const int DELAY_TIME_CONNECT_WIFI = 200;
const int FOLDER_CLASS_NAME = 1;
const int FOLDER_STUDENT_NAME = 2;
const int FOLDER_ERROR = 3;
const int MAX_AUDIO = 30;

// function prototype
void handleOnHomePageRequest();
void handleOnConnectWifi();
void handleOnNotifyRequest();
void handleOnChangeSpeakerStatus();
void updateScreen(String studentCode, String className, String studentName);
void playAudio(int folderNum, int trackNum);
String generateResponseJson(bool isSuccessNotify);
String generateHomePageHtml();
int getIndexAudio(std::map<String, String> list, String targetString);

String wifi_ssid = "Toan Thang";
String wifi_password = "15012002";
String localIP_Wifi = ""; // IP address of the ESP8266 on the local network

bool isWifiConnected = false;
bool canPlayAudio = true;
int volume = 25;
unsigned long lastTimeNotify = 0; // Last time the notification on screen was implemented
bool hasNotify = false;           // Check if the notification on screen has been implemented
int timeNotify = 20000;           // Time to notify on screen in milliseconds

// Create the Player object
// DFRobotDFPlayerMini player;
DFPlayerMini_Fast player;
SoftwareSerial softwareSerial(PIN_MP3_RX, PIN_MP3_TX);
SoftwareSerial esp32Serial(RX_ESPCAM, TX_ESPCAM);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
ESP8266WebServer server(80);

void setup()
{
  // Init USB serial port for debugging
  Serial.begin(9600);
  // esp32Serial.begin(9600);
  // Init serial port for DFPlayer Mini
  softwareSerial.begin(9600);
  pinMode(D3, INPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC))
  {
    Serial.println(F("SSD1306 allocation failed"));
    return;
  }

  if (!WiFi.softAPConfig(localIP_AP, localIP_AP, subnet))
  {
    Serial.println("AP Failed to configure");
    return;
  }

  while (!WiFi.softAP("ESP8266 WiFI", "12345678"))
  {
    Serial.print(".");
    delay(300);
  }

  Serial.print("Ap IP address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleOnHomePageRequest);
  server.on("/connect", HTTP_POST, handleOnConnectWifi);
  server.on("/notify", HTTP_POST, handleOnNotifyRequest);
  server.on("/speaker", HTTP_GET, handleOnChangeSpeakerStatus);
  server.on("/test_audio", HTTP_GET, handleTest);
  server.begin();

  displayDefaultText("WelCome!");

  connectToWifi();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}
void loop()
{
  server.handleClient();
  // Serial.println("ESP32: ");
  // Serial.println(esp32Serial.readString());
  // while(esp32Serial.available())
  // {
  //   Serial.print("ESP32: ");
  //   Serial.println(esp32Serial.read());
  // }

  if (hasNotify)
  {
    unsigned long currentTime = millis();
    if (currentTime - lastTimeNotify >= timeNotify)
    {
      hasNotify = false;
      displayDefaultText("WelCome!");
    }
  }
}

void handleOnChangeSpeakerStatus()
{
  // Lấy dữ liệu GET từ request
  canPlayAudio = server.arg("speaker_status") == "true" ? true : false;
  volume = server.arg("speaker_volume").toInt();

  Serial.println("Speaker Status: " + String(canPlayAudio));
  Serial.println("Volume: " + String(volume));

  String json = generateResponseJson(true);
  server.send(200, "application/json", json);
  return;
}

void handleOnHomePageRequest()
{
  // Send the JSON response
  String html = generateHomePageHtml();
  server.send(200, "text/html", html);
}

void handleOnNotifyRequest()
{
  // Lấy dữ liệu POST từ request
  String classCode = server.arg("class_code");
  String studentCode = server.arg("student_code");
  String error = server.arg("error");
  int errorType = server.arg("error_type").toInt();

  // Hiển thị dữ liệu POST trên Serial Monitor
  Serial.println("Class Code: " + classCode);
  Serial.println("Student Code: " + studentCode);

  if (classCode.length() == 0 || studentCode.length() == 0)
  {
    String json = generateResponseJson(false);
    server.send(404, "application/json", json);
    return;
  }

  int audioClassNameIndex = getIndexAudio(ClassCodeAudioMap, classCode);
  int audioStudentNameIndex = getIndexAudio(StudentCodeAudioMap, studentCode);

  Serial.println("audioClassNameIndex: " + String(audioClassNameIndex));
  Serial.println("audioStudentNameIndex: " + String(audioStudentNameIndex));

  if (error == "true" || audioClassNameIndex == 0 || audioStudentNameIndex == 0)
  {
    String json = generateResponseJson(true);
    server.send(404, "application/json", json);
    hasNotify = true;
    lastTimeNotify = millis();
    showMessage("Unknow!!!");
    playAudio(FOLDER_ERROR, errorType); // khong the nhan dien duoc
    return;
  }

  // Send the JSON response
  String json = generateResponseJson(true);
  server.send(200, "application/json", json);

  String className = ClassCodeAudioMap[classCode];
  String studentName = StudentCodeAudioMap[studentCode];

  // 
  hasNotify = true;
  lastTimeNotify = millis();

  updateScreen(studentCode, className, studentName);

  playAudio(FOLDER_STUDENT_NAME, audioStudentNameIndex);
  playAudio(FOLDER_CLASS_NAME, audioClassNameIndex);
}

void handleOnConnectWifi()
{
  // Lấy dữ liệu POST từ request
  wifi_ssid = server.arg("ssid");
  wifi_password = server.arg("password");

  Serial.print("Connecting to WiFi: ");
  Serial.println(wifi_ssid);
  Serial.print("Password: ");
  Serial.println(wifi_password);

  connectToWifi();

  // Send the JSON response
  String html = generateHomePageHtml();
  server.sendHeader("Location", "/", true);
  server.send(302, "text/html", html);
}

void updateScreen(String studentCode, String className, String studentName)
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(20, 5);
  display.println("Welcome");

  display.setTextSize(1);
  display.setCursor(0, 25);
  display.println("MSSV : " + studentCode);
  display.setCursor(0, 35);
  display.println("Name : " + studentName);
  display.setCursor(0, 45);
  display.println("Class: " + className);
  display.display();
}

void showMessage(String message)
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(10, 30);
  display.println(message);
  display.display();
}

void playAudio(int folderNum, int trackNum)
{
  // Start communication with DFPlayer Mini
  if (player.begin(softwareSerial) && canPlayAudio)
  {
    // Set volume to maximum (0 to 30).
    player.volume(volume);
    // Play the specified MP3 file on the SD card
    player.playFolder(folderNum, trackNum);
    Serial.print("Playing: ");
    Serial.println(trackNum);

    // // Wait until audio finishes playing
    while (player.isPlaying())
    {
      delay(10); // Delay for a short time
    }
  }
  else
  {
    Serial.println("Connecting to DFPlayer Mini failed!");
  }
}

String generateResponseJson(bool isSuccessNotify)
{
  // tao doi tuong JSON
  StaticJsonDocument<200> doc;
  // tao bo dem de luu tru du lieu JSON
  char jsonBuffer[200];

  // neu thong bao thanh cong
  if (isSuccessNotify)
  {
    // them du lieu vao doi tuong JSON
    doc["message"] = "Thông báo thành công!";
    doc["error"] = false;
  }
  else
  {
    // them du lieu vao doi tuong JSON
    doc["message"] = "Thông báo thất bại!";
    doc["error"] = true;
  }

  // chuyen doi doi tuong JSON sang chuoi JSON
  serializeJson(doc, jsonBuffer);

  // tra ve chuoi JSON
  return jsonBuffer;
}

String generateHomePageHtml()
{
  String html = HomePageStyle;
  html += "<h1 style='text-align: center; font-size: 40px; margin-top: 100px'>Cấu hình module ESP8266</h1>";
  html += "<h2 style='text-align: center; font-size: 20px; margin-top: 20px'>Hãy nhập thông tin wifi</h2>";
  html += "<form action='/connect' method='POST'>";
  html += "<div class='input-group'><label for='ssid'>SSID:</label><input type='text' id='ssid' name='ssid' value='" + wifi_ssid + "' placeholder='Enter Wifi name' required /></div>";
  html += "<div class='input-group'><label for='password'>Password:</label><input type='text' id='password' name='password' value='" + wifi_password + "' placeholder='Enter Wifi password' required /></div>";
  html += "<input id = 'btnConnect' type='submit' value='Kết nối Wifi' class='btn-submit'>";
  html += "</form>";
  html += "<table style='width:60%; border: 1px solid #00aeff; padding: 10px;'>";
  html += "<tr><th></th><th></th></tr>";

  if (isWifiConnected)
  {
    html += "<tr><td>Trạng thái</td><td style='font-weight: bold; color: rgb(0, 255, 0);'>Đã kết nối</td></tr>";
    html += "<tr><td>Tên wifi</td><td>" + wifi_ssid + "</td></tr>";
    html += "<tr><td>Mật khẩu wifi</td><td>" + wifi_password + "</td></tr>";
    html += "<tr><td>Địa chỉ IP</td><td>" + localIP_Wifi + "</td></tr>";
  }
  else
  {
    html += "<tr><td>Trạng thái</td><td style='font-weight: bold; color: rgb(255, 0, 0);'>Chưa kết nối</td></tr>";
    html += "<tr><td>Tên wifi</td><td>?-?-?-?</td></tr>";
    html += "<tr><td>Mật khẩu wifi</td><td>?-?-?-?</td></tr>";
    html += "<tr><td>Địa chỉ IP</td><td>?-?-?-?</td></tr>";
  }
  html += "</table>";

  if (canPlayAudio)
  {
    html += "<div class='input-group mt-20'><div class='switch'><label for='checkboxCamera'>Speaker:</label><input type='checkbox' id='checkboxCamera' name='checkboxCamera' class='default-action' onclick='changeStatusLabel(this)' checked/><label class='slider' for='checkboxCamera'></label><span id='camera-status'>ON</span></div></div>";
  }
  else
  {
    html += "<div class='input-group mt-20'><div class='switch'><label for='checkboxCamera'>Speaker:</label><input type='checkbox' id='checkboxCamera' name='checkboxCamera' class='default-action' onclick='changeStatusLabel(this)'/><label class='slider' for='checkboxCamera'></label><span id='camera-status'>OFF</span></div></div>";
  }

  html += "<div class='input-group flex-direction-row mt-20'><label for='speaker-volume-range'>Volume:</label><input id='speaker-volume-range' name='speaker-volume-range' type='range' min='0' max='" + String(MAX_AUDIO) + "' value='" + String(volume) + "' class='' step='1' onchange='showVal(this.value)' oninput='showVal(this.value)'><span id='speaker-volume'>" + String(volume) + "</span></div>";

  html += "<input id = 'btn-change-speaker-status' type='submit' value='Cập nhật' class='btn-submit'>";
  html += HomePageScript;

  return html;
}

// return index of audio file in SD card
int getIndexAudio(std::map<String, String> list, String targetString)
{
  int index = 0;

  for (auto const &item : list)
  {
    if (item.first == targetString)
    {
      return index + 1;
    }
    index++;
  }
  return 0;
}

void connectToWifi()
{
  int times = 0; // time try to connect wifi
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED && times < MAX_TIMES_TRY_CONNECT_WIFI)
  {
    delay(DELAY_TIME_CONNECT_WIFI);
    Serial.print(".");
    times++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    isWifiConnected = true;
    localIP_Wifi = WiFi.localIP().toString();
    Serial.println("Connected to WiFi");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    isWifiConnected = false;
    Serial.println("Failed to connect to WiFi");
  }
}

void displayDefaultText(String text)
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(20, 30);
  display.println(text);
  display.display();
}

void handleTest()
{
  int folderTrack = server.arg("folder_track").toInt();
  int trackNum = server.arg("track_num").toInt();
  playAudio(folderTrack, trackNum);

  String json = generateResponseJson(true);
  server.send(200, "application/json", json);
  return;
}