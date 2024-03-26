## Getting Started

`Tinh chỉnh lại code`

1. Mở file `ESP32CAM.ino`.

2. Mở comment model camera tương ứng và đóng những cái còn lại. ví dụ `#define CAMERA_MODEL_AI_THINKER`.

3. Nhập thông tin wifi

    ```cpp
    const char* ssid = "YOUR_WIFI_SSID";
    const char* password = "YOUR_WIFI_PASSWORD";
    ```

4. Nhập thông tin server

    ```cpp
    String serverName = "YOUR_SERVER_IP_ADDRESS";
    const int serverPort = YOUR_SERVER_PORT;
    String serverPath = "/upload";
    String fileName = "ESP32-001";
    ```

<br>
5. Nạp code cho mạch.
<br>
6. Nhấn nút reset trên mạch để khởi động mạch

## Cấu hình Arduino IDE

Nếu sử dụng model AI_THINKER, hãy cấu hình Arduino IDE như sau:

-   Board: "AI Thinker ESP-32 CAM"
-   Port: "COMX"
-   CPU Frequency: "240MHz (WiFi/BT)"
-   Flash Frequency: "80MHz"
-   Flash Mode: "DIO"
-   Partition Scheme: "Huge APP (3MB No OTA/1MB SPIFFS)"
