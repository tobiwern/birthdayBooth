#include <WiFi.h>
//#include <ESPAsyncWebServer.h>
//#include <ESPAsyncWiFiManager.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "esp_camera.h"
#include "esp_timer.h"
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"

// Change the following settings according to your network
const char* ssid = "FRITZ!Box 7490";
const char* password = "79739197982501102402";

// Pin definition for the camera module
#define CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define LED_GPIO_NUM       4

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32-CAM Photo Viewer</title>
</head>
<body>
  <h1>ESP32-CAM Photo Viewer</h1>
  <button onclick="capturePhoto()">Capture Photo</button>
  <div id="photoContainer"></div>

  <script>
    var socket;

    // Function to connect to the WebSocket server
    function connectWebSocket() {
      socket = new WebSocket('ws://'+location.hostname+':81/');
      socket.binaryType = 'arraybuffer';

      // When the WebSocket connection is established
      socket.onopen = function(event) {
        console.log('Connected to WebSocket');
      };

      // When data is received from the WebSocket server
      socket.onmessage = function(event) {
        console.log("Message from WebSocket connection received.");                     
        const contentType = event.data.constructor.name;
        console.log('Received data type: ' + event.data.constructor.name);

        if (contentType === 'Blob' || contentType === 'ArrayBuffer') {
          console.log("Receiving image...");
          var imgData = new Uint8Array(event.data);
          var blob = new Blob([imgData], { type: 'image/jpeg' });
          var imgUrl = URL.createObjectURL(blob);
          var img = document.createElement('img');
          img.src = imgUrl;
          document.getElementById('photoContainer').replaceChildren(img);
        }
        else{
          console.log('WebSocket message: ' + event.data);
          console.log("Capture in progress...");          
        }
      };

      // When the WebSocket connection is closed
      socket.onclose = function(event) {
        console.log('WebSocket connection closed');
      };
    }

    // Function to capture a photo from the ESP32-CAM
    function capturePhoto() {
      console.log('Capturing photo...');
      if (socket.readyState === WebSocket.OPEN) {
        socket.send('C');
      }
      else{
        console.log('WebSocket not connected.')
      }
    }

    // Connect to the WebSocket server on page load
    window.addEventListener('load', connectWebSocket);
  </script>
</body>
</html>
)rawliteral";

//Create an instance of HTTP server
WebServer server(80);      
// Create an instance of the WebSocketsServer
WebSocketsServer webSocket = WebSocketsServer(81);

void handle_OnConnect() {
  Serial.println("Delivering landing page...");
  server.send(200, "text/html", index_html); 
}

void handle_OnCapture(){
  Serial.println("Capture request received.");
  server.send(200, "text/plain", "Capture request received successfully."); 
  captureAndSendPhoto();
}

void captureAndSendPhoto(){
    Serial.println("Taking photo...");
    Serial.println("Sending message for capturing to all WebSocket clients ...");
    webSocket.broadcastTXT("capturing",strlen("capturing"));
    Serial.println("Sent message for capturing to all WebSocket clients .");
    camera_fb_t *fb = NULL;
    fb = esp_camera_fb_get();
    if (fb) {
      Serial.println("Buffer from camera received.");
      Serial.println("Broadcasting image to WebSocket client 0 ...");
      webSocket.sendBIN(0,fb->buf, fb->len);
      Serial.println("Broadcasted image to WebSocket client 0 .");
      esp_camera_fb_return(fb);
    }
}

// Function to handle WebSocket events
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch(type) {
    // When a WebSocket connection is established
    case WStype_CONNECTED:
      Serial.printf("[%u] Connected!\n", num);
      break;
    // When data is received from a WebSocket connection
    case WStype_TEXT:
    Serial.println("Call to Websocket received.");
      if (payload[0] == 'C') { // Capture a photo
        captureAndSendPhoto();
      }
      break;
    // When a WebSocket connection is closed
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
  }
}

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.print("Connected to WiFi with IP ");
  Serial.println(WiFi.localIP());

// Turn-off the 'brownout detector'
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // Configure the camera
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 12; //10
    config.fb_count = 2; //2
    config.grab_mode = CAMERA_GRAB_LATEST;
    Serial.println("PSRAM present -> Camera resolution is UXGA (1600 × 1200)");
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;
    Serial.println("PSRAM not present -> Camera resolution is SVGA (800 x 600)");
  }

  // Initialize the camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera initialization failed with error 0x%x", err);
    ESP.restart();
  }

  // HTTP route for root / web page
  server.on("/", handle_OnConnect);

  // Alternative HTTP route to WebSocket for photo capture
  server.on("/capture", handle_OnCapture);

  //Start HTTP server
  Serial.println("Starting Web Server...");
  server.begin();
  Serial.println("Web Server started");

  // Start the WebSocket server
  Serial.println("Starting WebSocket...");  
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket server started");
}

void loop() {
  server.handleClient();
  webSocket.loop();
}
