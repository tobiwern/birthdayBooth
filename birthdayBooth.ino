#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "esp_camera.h"
#include "esp_timer.h"
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#include "driver/rtc_io.h"
#include "credentials.h"
#include "webpage.h"
#include "leds.h"

const char* ssid = "BirthdayBooth";      //mySSID;
const char* password = "BirthdayBooth";  //myPASSWORD;
String hostname = "birthday-booth";
const int channel = 6;
//const bool  hide_SSID = false;
//const int   max_connection = 2;
IPAddress local_ip(192, 168, 0, 50);
IPAddress gateway(192, 168, 0, 50);
IPAddress subnet(255, 255, 255, 0);

// Pin definition for the camera module
#define CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22
#define LED_GPIO_NUM 4

#define STATE_IDLE 0
#define STATE_COUNTDOWN 1
#define STATE_CAPTURE 2
int STATE = 0;
int STATE_PREVIOUS = -1;
String stateTbl[] = { "STATE_IDLE", "STATE_COUNTDOWN", "STATE_CAPTURE" };

//Create an instance of HTTP server
WebServer server(80);
// Create an instance of the WebSocketsServer
WebSocketsServer webSocket = WebSocketsServer(81);

void handle_OnConnect() {
  Serial.println("Delivering landing page...");
  server.send(200, "text/html", index_html);
}

void handle_OnCapture() {
  Serial.println("Capture request received.");
  server.send(200, "text/plain", "Capture request received successfully.");
  STATE = STATE_COUNTDOWN;
}

void captureAndSendPhoto() {
  Serial.println("Taking photo...");
  Serial.println("Sending message for capturing to all WebSocket clients ...");
  webSocket.broadcastTXT("capturing", strlen("capturing"));
  Serial.println("Sent message for capturing to all WebSocket clients .");
  camera_fb_t* fb = NULL;
  fb = esp_camera_fb_get();
  if (fb) {
    Serial.println("Buffer from camera received.");
    Serial.println("Broadcasting image to WebSocket client 0 ...");
    //    webSocket.sendBIN(0, fb->buf, fb->len);
    webSocket.broadcastBIN(fb->buf, fb->len);
    Serial.println("Broadcasted image to WebSocket client 0 .");
    esp_camera_fb_return(fb);
  }
}

// Function to handle WebSocket events
void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    // When a WebSocket connection is established
    case WStype_CONNECTED:
      Serial.printf("[%u] Connected!\n", num);
      break;
    // When data is received from a WebSocket connection
    case WStype_TEXT:
      Serial.println("Call to Websocket received.");
      if (payload[0] == 'C') {  // Capture a photo
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

  // Open Wi-Fi Access Point
  //  WiFi.mode(WIFI_AP);
  //  WiFi.softAPConfig(local_ip, gateway, subnet);
  Serial.println("Opening Access Point...");
  WiFi.softAP(ssid, password);  //, channel); //WiFi.softAP(ssid, password, channel, hide_SSID, max_connection);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("IP address for Access Point: ");
  Serial.println(IP);
  WiFi.setSleep(false);

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
    config.jpeg_quality = 12;  //10
    config.fb_count = 2;       //2
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

  // Setup LED RING
  setupLeds();
}

void loop() {
  server.handleClient();
  webSocket.loop();
  handleState();
}

void handleState() {
  unsigned long millisNow = millis();
  //  Serial.println("STATE = " + stateTbl[STATE] + ", STATE_PREVIOUS = " + stateTbl[STATE_PREVIOUS]);
  if (STATE != STATE_PREVIOUS) { Serial.println("STATE = " + stateTbl[STATE]); }
  switch (STATE) {
    case STATE_IDLE:  //***********************************************************
      ledsRainbowWithGlitter();
      counter = 0;
      break;
    case STATE_COUNTDOWN:  //***********************************************************
      Serial.println("STATE = " + stateTbl[STATE]);
      ledsCountdown();
      if (counter > NUM_LEDS) { STATE = STATE_CAPTURE; }
      break;
    case STATE_CAPTURE:
      Serial.println("STATE = " + stateTbl[STATE]);
      ledsGreen();
      captureAndSendPhoto();
      //      sinelon();
      STATE = STATE_IDLE;
      break;
    default:
      break;
  }
  STATE_PREVIOUS = STATE;
}
