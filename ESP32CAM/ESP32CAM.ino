#include "esp_camera.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <driver/ledc.h>

//
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//
//            You must select partition scheme from the board menu that has at least 3MB APP space.
//            Face Recognition is DISABLED for ESP32 and ESP32-S2, because it takes up from 15
//            seconds to process single frame. Face Detection is ENABLED if PSRAM is enabled as well

// ===================
// Select camera model
// ===================
// #define CAMERA_MODEL_WROVER_KIT // Has PSRAM
// #define CAMERA_MODEL_ESP_EYE // Has PSRAM
// #define CAMERA_MODEL_ESP32S3_EYE // Has PSRAM
// #define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
// #define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM
// #define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
// #define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
// #define CAMERA_MODEL_M5STACK_UNITCAM // No PSRAM
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
// #define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM
// #define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
//  ** Espressif Internal Boards **
// #define CAMERA_MODEL_ESP32_CAM_BOARD
// #define CAMERA_MODEL_ESP32S2_CAM_BOARD
// #define CAMERA_MODEL_ESP32S3_CAM_LCD
// #define CAMERA_MODEL_DFRobot_FireBeetle2_ESP32S3 // Has PSRAM
// #define CAMERA_MODEL_DFRobot_Romeo_ESP32S3 // Has PSRAM
#include "camera_pins.h"
#include "HtmlStyleConfig.h"

#define FLASH_GPIO_NUM 4
#define CAMERA_LED_NUM 33

const int LEDC_CHANNEL = 0;         // Kênh LEDC được sử dụng
const int LEDC_FREQ_HZ = 5000;      // Tần số PWM (Hz)
const int LEDC_RESOLUTION = 8;      // Độ phân giải LEDC
const int POWER_FLASH_MODE_OFF = 0; // Giá trị PWM để tắt đèn flash
int POWER_FLASH_MODE_ON = 10;       // Giá trị PWM để bật đèn flash (tùy chỉnh)  // 0-255

const IPAddress staticIP(192, 168, 123, 1); // Địa chỉ IP tĩnh cho ESP32 ở chế độ AP
const IPAddress subnet(255, 255, 255, 0);   // Subnet mask
const int MAX_TIMES_TO_CONNECT = 20;        // Số lần kết nối tối đa toi wifi

const int MAX_TIMES_TO_CAPTURE_IMAGE_FAILED = 20; // so lan chup anh loi toi da
int captureFailedTimes = 0;

const char *apSSID = "ESP32-AP"; // Tên của mạng Wi-Fi riêng của ESP32
const char *apPassword = "";     // Mật khẩu của mạng Wi-Fi riêng
const int apPort = 80;           // Cổng của Web Server trong chế độ AP

const char *serverPath = "/arduino/postFile?room=1B18"; // Hằng số cho server path
const char *fileName = "file";                          // Hằng số cho file name

char ssid[32] = "virus.exe";
char password[32] = "123456789";
char serverName[32] = "10.241.14.147";
int serverPort = 3000;
int pictureInterval = 5000;
bool isCameraOn = true;
bool isConnectedWifi = false;

unsigned long latestPicture = 0; // last time image was sent (in milliseconds)

WebServer server(apPort); // Tạo đối tượng máy chủ với cổng apPort

WiFiClient client;

void setup()
{
  Serial.begin(9600);

  // config camera
  configCamera();

  ledcSetup(LEDC_CHANNEL, LEDC_FREQ_HZ, LEDC_RESOLUTION);
  ledcAttachPin(FLASH_GPIO_NUM, LEDC_CHANNEL);

  pinMode(CAMERA_LED_NUM, OUTPUT);

  // startAPMode();
  Serial.println("Starting AP mode...");

  // config static IP for ESP32 in AP mode
  if (!WiFi.softAPConfig(staticIP, staticIP, subnet))
  {
    Serial.println("AP failed to configure");
    return;
  }

  while (!WiFi.softAP(apSSID, apPassword))
  {
    Serial.print(".");
    delay(300);
  }

  Serial.println("AP mode started successfully!!");
  Serial.print("AP address: ");
  Serial.println(WiFi.softAPIP());

  connectToWifi();

  server.on("/", HTTP_GET, handleOnHomePageRequest);
  server.on("/config", HTTP_POST, handleConfigOnAPMode);
  server.on("/config", HTTP_GET, []() {
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });

  server.begin();
}

void loop()
{
  server.handleClient();

  if (isCameraOn && isConnectedWifi)
  {
    unsigned long currentMilliseconds = millis();
    if (currentMilliseconds - latestPicture >= pictureInterval)
    {
      takePicture();
      latestPicture = currentMilliseconds;
    }
  }
}

String takePicture()
{
  Serial.println("Taking image...");

  String responseHeaders;
  String responseBody;

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb)
  {
    if (captureFailedTimes > MAX_TIMES_TO_CAPTURE_IMAGE_FAILED)
    {
      Serial.println("Capture failed too many times. Restarting device...");
      flashing(1000);
      flashing(1000);
      ESP.restart();
    }
    Serial.println("Camera capture failed");
    captureFailedTimes++;
    delay(1000);
    return "";
  }

  Serial.println(String("Connecting to server: ") + serverName);

  if (client.connect(serverName, serverPort))
  {
    // turn off camera
    turnOffCamera();

    Serial.println("Connection successful!");
    String boundary = "--MK--";
    String head = "--" + boundary + "\r\nContent-Disposition: form-data; name=\"file\"; filename=\"" + fileName + ".jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--" + boundary + "--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t totalLen = imageLen + head.length() + tail.length();

    Serial.print("Sending Image");

    client.println(String("POST ") + serverPath + " HTTP/1.1");
    client.println(String("Host: ") + serverName);
    client.println("Content-Length: " + String(totalLen));
    client.println("Content-Type: multipart/form-data; boundary=" + boundary);
    client.println();
    client.print(head);

    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    size_t bufferSize = 1024;
    for (size_t n = 0; n < fbLen; n += bufferSize)
    {
      size_t remaining = fbLen - n;
      size_t chunkSize = remaining < bufferSize ? remaining : bufferSize;
      client.write(fbBuf + n, chunkSize);
    }
    client.print(tail);

    esp_camera_fb_return(fb);

    int timoutTimer = 10000;
    long startTimer = millis();
    boolean state = false;

    while ((startTimer + timoutTimer) > millis())
    {
      Serial.print(".");
      delay(100);
      while (client.available())
      {
        char c = client.read();
        if (c == '\n')
        {
          if (responseHeaders.length() == 0)
          {
            state = true;
          }
          responseHeaders = "";
        }
        else if (c != '\r')
        {
          responseHeaders += String(c);
        }
        if (state == true)
        {
          responseBody += String(c);
        }
        startTimer = millis();
      }
      if (responseBody.length() > 0)
      {
        break;
      }
    }

    client.stop();
    // Serial.println(responseBody);

    bool isError = getErrorValue(responseBody);

    if (isError == false)
    {
      // send image successful
      Serial.println();
      Serial.println("Nhan dien thanh cong");
      flashing(100);

      // reset capture failed times
      captureFailedTimes = 0;
    }
    else
    {
      Serial.println("Nhan dien that bai!");
    }

    // turn on camera
    turnOnCamera();
  }
  else
  {
    responseBody = String("Connection to ") + serverName + String(" failed.");
    Serial.println(responseBody);
  }

  Serial.println();
  return responseBody;
}

bool getErrorValue(String responseBody)
{
  // Khởi tạo bộ đệm để lưu trữ tài nguyên
  StaticJsonDocument<200> doc;

  // Phân tích chuỗi JSON vào tài nguyên
  DeserializationError error = deserializeJson(doc, responseBody);
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return true; // Nếu có lỗi trong quá trình phân tích JSON, trả về true
  }

  // Lấy giá trị của trường "error"
  bool isError = doc["error"];

  Serial.print("Error: ");
  Serial.println(doc["error"].as<String>());

  return isError;
}

void configCamera()
{
  Serial.println("Configuring Camera...");

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;
  // config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (config.pixel_format == PIXFORMAT_JPEG)
  {
    if (psramFound())
    {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    }
    else
    {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  }
  else
  {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();

  // ov2640
  if (s->id.PID == OV2640_PID)
  {
    s->set_gain_ctrl(s, 1);     // auto gain on
    s->set_exposure_ctrl(s, 1); // auto exposure on
    s->set_awb_gain(s, 1);      // Auto White Balance enable (0 or 1)
    s->set_saturation(s, -1);   // lower the saturation
  }

  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID)
  {
    s->set_vflip(s, 1);       // flip it back
    s->set_brightness(s, 1);  // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }

  // camera framesize
  s->set_framesize(s, FRAMESIZE_UXGA);

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

  // // Setup LED FLash if LED pin is defined in camera_pins.h
  // #if defined(LED_GPIO_NUM)
  // setupLedFlash(LED_GPIO_NUM);
  // #endif

  Serial.println("Camera configured successfully!!");
  Serial.println();
}

void handleOnHomePageRequest()
{
  String html = generateHomePageHtml();
  server.send(200, "text/html", html);
}

void handleConfigOnAPMode()
{
  // Xử lý yêu cầu cấu hình
  if (server.method() == HTTP_POST)
  {
    String newSSID = server.arg("ssid");
    String newPassword = server.arg("password");
    String newServerName = server.arg("serverName");
    int newServerPort = server.arg("serverPort").toInt();
    int newPictureInterval = server.arg("pictureInterval").toInt();

    // Lấy giá trị trạng thái của checkbox camera
    bool newCameraStatus = server.hasArg("checkboxCamera");

    // Lưu giá trị cấu hình vào biến tương ứng
    newSSID.toCharArray(ssid, sizeof(ssid));
    newPassword.toCharArray(password, sizeof(password));
    newServerName.toCharArray(serverName, sizeof(serverName));
    serverPort = newServerPort;
    pictureInterval = newPictureInterval;

    if (newCameraStatus == true)
    {
      turnOnCamera();
    }
    else
    {
      turnOffCamera();
    }

    bool isSuccess = connectToWifi();

    if (isSuccess)
    {
      Serial.println("Gui bieu mau thanh cong!!");
      String message = "Connect to wifi " + String(ssid) + " successfully!!";
      // server.send(200, "text/plain", "Gui bieu mau thanh cong!!");
      goToResultPage(message, 200);
    }
    else
    {
      Serial.println("Gui bieu mau that bai!!");
      String message = "Connect to wifi " + String(ssid) + " successfully!!";
      goToResultPage(message, 200);
    }
  }
  else
  {
    server.send(400, "text/plain", "Yêu cầu không hợp lệ");
  }
}

bool connectToWifi()
{
  Serial.println("Starting Wifi mode...");

  WiFi.begin(ssid, password);

  Serial.println("Connecting to Wi-Fi...");
  int connectTimes = 0; // Số lần kết nối đến WiFi
  while (WiFi.status() != WL_CONNECTED && connectTimes < MAX_TIMES_TO_CONNECT)
  {
    delay(1000);
    Serial.println(".");
    connectTimes++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Connected to WiFi");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    isConnectedWifi = true;
  }
  else
  {
    Serial.println("Failed to connect to WiFi");
    return false;

    isConnectedWifi = false;
  }

  return false;
}

void goToResultPage(String message, int code)
{
  String html = styleSuccessfulPage;
  html += "<h1 style='text-align: center; margin-top: 100px;'>" + message + "</h1>";
  html += "<a href='/'>Back to home</a>";

  server.send(code, "text/html", html);
}

void turnOnFlashLight()
{
  ledcWrite(LEDC_CHANNEL, POWER_FLASH_MODE_ON);
}

void turnOffFlashLight()
{
  ledcWrite(LEDC_CHANNEL, POWER_FLASH_MODE_OFF);
}

void flashing(int delayTime)
{
  Serial.print("Flash on - ");
  turnOnFlashLight();
  delay(delayTime);
  Serial.print("Flash off - ");
  turnOffFlashLight();
  delay(delayTime);
}

void turnOnCamera()
{
  isCameraOn = true;
  digitalWrite(CAMERA_LED_NUM, LOW);
  Serial.println("Camera turned on");
}

void turnOffCamera()
{
  isCameraOn = false;
  digitalWrite(CAMERA_LED_NUM, HIGH);
  Serial.println("Camera turned off");
}

String generateHomePageHtml()
{
  // Trang web cơ bản để nhập dữ liệu cấu hình
  String html = styleHomePage;
  html += "<h1 style='text-align: center; font-size: 40px; margin-top: 100px'>Config ESP32</h1>";
  html += "<form action='/config' method='post'>";
  html += "<div class='input-group'>";
  html += "<label for='ssid'>SSID:</label>";
  html += "<input type='text' id='ssid' name='ssid' value='" + String(ssid) + "' required />";
  html += "</div>";
  html += "<div class='input-group'>";
  html += "<label for='password'>Password:</label>";
  html += "<input type='text' id='password' name='password' value='" + String(password) + "' required />";
  html += "</div>";
  html += "<div class='input-group'>";
  html += "<label for='serverName'>Server Name:</label>";
  html += "<input type='text' id='serverName' name='serverName' value='" + String(serverName) + "' required />";
  html += "</div>";
  html += "<div class='input-group'>";
  html += "<label for='serverPort'>Server Port:</label>";
  html += "<input type='number' id='serverPort' name='serverPort' value='" + String(serverPort) + "' required />";
  html += "</div>";
  html += "<div class='input-group'>";
  html += "<label for='pictureInterval'>Picture Interval (ms):</label>";
  html += "<input type='number' id='pictureInterval' name='pictureInterval' value='" + String(pictureInterval) + "' required />";
  html += "</div>";
  html += "<div class='input-group'>";
  html += "<div class='switch'>";
  html += "<label for='checkboxCamera'>Camera:</label>";
  html += "<input type='checkbox' id='checkboxCamera' name='checkboxCamera'  class='default-action'";
  if (isCameraOn)
  {
    html += " checked='checked'";
  }
  html += " />";
  html += "<label class='slider' for='checkboxCamera'></label>";
  html += "</div>";
  html += "</div>";
  html += "<input type='submit' value='Submit'>";
  html += "</form>";

  return html;
}