//
// S3CAM_master.ino, 26.03.2026
//

#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"

// UART1 port for communication with the ESP32-C3 supermini slave controller
#define RX 18
#define TX 17

#define PART_BOUNDARY "123456789000000000000987654321"
#define FrameRate 15
#define ms_betweenFrames 1000/FrameRate
#define XON 0x11
#define XOFF 0x13
#define REGULAR_SPEED 170

#define CAMERA_MODEL_NEW_ESPS3_RE1_1 // Aliexpress card
// #define CAMERA_MODEL_ESP32S3_EYE  // Has PSRAM

const char* ssid = "XXXXXX";
const char* password = "YYYYYY";

String my_ip;    // ip address
uint8_t command = 0,
        k = 0x30,  // MIN_SPEED
        x = 0;

#if defined(CAMERA_MODEL_NEW_ESPS3_RE1_1)	// my first ESP32-S3-CAM board
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 10
#define SIOD_GPIO_NUM 21
#define SIOC_GPIO_NUM 14

#define Y9_GPIO_NUM 11
#define Y8_GPIO_NUM 9
#define Y7_GPIO_NUM 8
#define Y6_GPIO_NUM 6
#define Y5_GPIO_NUM 4
#define Y4_GPIO_NUM 2
#define Y3_GPIO_NUM 3
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 13
#define HREF_GPIO_NUM 12
#define PCLK_GPIO_NUM 7

#define USE_WS2812 // Use SK6812 rgb led   
#ifdef USE_WS2812
#define LED_GPIO_NUM 33 // SK6812 rgb led
#else
#define LED_GPIO_NUM 34 // green signal led 
#endif
// Define SD Pins
#define SD_MMC_CLK 42
#define SD_MMC_CMD 39
#define SD_MMC_D0 41
// Define Mic Pins
#define I2S_SD 35 // I2S Microphone
#define I2S_WS 37
#define I2S_SCK 36 

//-------------------------------------------------------------------------
#elif defined(CAMERA_MODEL_ESP32S3_EYE)		// my second ESP32-S3-CAM board
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  15
#define SIOD_GPIO_NUM  4
#define SIOC_GPIO_NUM  5

#define Y2_GPIO_NUM 11
#define Y3_GPIO_NUM 9
#define Y4_GPIO_NUM 8
#define Y5_GPIO_NUM 10
#define Y6_GPIO_NUM 12
#define Y7_GPIO_NUM 18
#define Y8_GPIO_NUM 17
#define Y9_GPIO_NUM 16

#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM  7
#define PCLK_GPIO_NUM  13

#else
#error "Camera model not selected"
#endif

static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t index_httpd = NULL;
httpd_handle_t stream_httpd = NULL;

uint32_t frameStartms;
uint32_t fr_next = 0; 

//-----------------------------------------------------------------------
static esp_err_t index_handler(httpd_req_t *req) {
  String out;
  char  buf[32];

  if (httpd_req_get_url_query_len(req) > 0) {

    httpd_req_get_url_query_str(req, buf, 32);
    
    switch(buf[0]) {
      case 'f': // Forward
      case 'b': // Backward
      case 'r': // Right
      case 'l': // Left
      case 's': // Stop
      case 'w': // headLight --> ON
      case 'x': // headLight --> OFF
                command = toupper(buf[0]);
                break;
      case 'p': if(k < 0x37) { // '7'
                  k++;
                  command = k;
                }
                break;
      case 'm': if(k > 0x30) { // '0'
                  k--;
                  command = k;
                }
    }
    do { 
        if(Serial1.available() > 0) { // если в буфере есть данные
          x = Serial1.read(); // считываем символ
        }
    } while(x != XON);
    Serial1.write(command);
    Serial.println(command);
                
    
    httpd_resp_set_status(req, HTTPD_204);
    return httpd_resp_send(req, NULL, 0);
  }

  // Формирование HTML-страницы
  out = "<!DOCTYPE html>";
  out += "<html><head><meta charset=\"utf-8\"><title>ESP32S3-CAM & ESP32C3-supermini RC Control</title>";

  out += "<style>body #myDIV{height:40px; font:10px Verdana Pro;}";

  out += ".custom-btn {width: 130px; height: 40px; color: #fff; border-radius: 5px; padding: 10px 25px;";
  out += "font-family: tahoma; font-weight: 500; background: transparent; cursor: pointer;";
  out += "transition: all 0.3s ease; position: relative; display: inline-block;";
  out += "box-shadow: inset 2px 2px 2px 0px rgba(255,255,255,.5), 7px 7px 20px 0px rgba(0,0,0,.1), 4px 4px 5px 0px rgba(0,0,0,.1);";
  out += "outline: none;}";

  out += ".mybtn {background: rgb(96,9,240);background: linear-gradient(0deg, rgba(96,9,240,1) 0%, rgba(129,5,240,1) 100%);";
  out += "border: none;}";
  out += ".mybtn:before {height: 0%; width: 2px;}";
  out += ".mybtn:hover {box-shadow: 4px 4px 6px 0 rgba(255,255,255,.5), -4px -4px 6px 0 rgba(116, 125, 136, .5),";
  out += "inset -4px -4px 6px 0 rgba(255,255,255,.2), inset 4px 4px 6px 0 rgba(0, 0, 0, .4);}";
  out += "</style></head>";

  out += "<body bgcolor=#3490DE><center>"; 
  out += "<div id='myDIV'><h2>WiFi Control by ESP32S3-CAM  &  ESP32C3</h2></div>";
  out += "<img src=\"http://" + my_ip + ":81/stream\"><br><br>"; 

  out += "<table cellspacing=15 bgcolor=#7fffd4>";

  out += "<tr>"; // 0th row of the table
  out += "<th>";
  out += "<th>";
  out += "<th><button class=\"custom-btn mybtn\" onclick=\"window.location.href='?f'\">Forward</button>";
  out += "<th>";
  out += "<th>";
  out += "</tr>";
  
  out += "<tr>"; // 1st row
  out += "<th><button id=\"button1\" class=\"custom-btn mybtn\" onclick=\"window.location.href='?w'\">Light ON</button>";
  out += "<th><button id=\"button6\" class=\"custom-btn mybtn\" onclick=\"window.location.href='?l'\">Left</button>";
  out += "<th><button id=\"button4\" class=\"custom-btn mybtn\" onclick=\"window.location.href='?s'\">Stop</button>";
  out += "<th><button id=\"button7\" class=\"custom-btn mybtn\" onclick=\"window.location.href='?r'\">Right</button>";
  out += "<th><button id=\"button8\" class=\"custom-btn mybtn\" onclick=\"window.location.href='?p'\">Speed +</button>";
  out += "</tr>";

  out += "<tr>"; // 2nd row
  out += "<th><button id=\"button2\" class=\"custom-btn mybtn\" onclick=\"window.location.href='?x'\">Light OFF</button>";
  out += "<th>";
  out += "<th><button id=\"button5\" class=\"custom-btn mybtn\" onclick=\"window.location.href='?b'\">Backward</button>";
  out += "<th>";
  out += "<th><button id=\"button9\" class=\"custom-btn mybtn\" onclick=\"window.location.href='?m'\">Speed -</button>";
  out += "</tr>";
  out += "</table>";
  out += "<br><br>";                        
  out += "</center>";

  out += "</body></html>";  

  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, (const char *)out.c_str(), out.length());  
}

//-------------------------------------------------------------------
static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t *_jpg_buf = NULL;
  char *part_buf[128];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if(res != ESP_OK)
    return res;

  while(true) {    
    while (millis() < fr_next)
      delay(1);
    fr_next = millis() + ms_betweenFrames;
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    } 
    else {
      if(fb->width > 400) {
        if(fb->format != PIXFORMAT_JPEG) {
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
          if(!jpeg_converted) {
            Serial.println("JPEG compression failed");
            res = ESP_FAIL;
          }
        }
        else {
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
        }
      }
    }
    if(res == ESP_OK) {
      size_t hlen = snprintf((char*) part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char*) part_buf, hlen);
    }
    if(res == ESP_OK) {
      res = httpd_resp_send_chunk(req, (const char*) _jpg_buf, _jpg_buf_len);
    }
    if(res == ESP_OK) {
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if(fb) {
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if(_jpg_buf){
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if(res != ESP_OK) {
      break;
    }
    uint32_t frameEndms = millis(); 
    uint32_t frameTimems = frameEndms - frameStartms;
    frameStartms = frameEndms;
    //Serial.printf("MJPG: %uB %ums\n", (uint32_t)(_jpg_buf_len), frameTimems);
  }
  return res;
}

//--------------------------------------------------------------------
void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = index_handler,
    .user_ctx  = NULL
  };
  
  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };
  
  if(httpd_start(&index_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(index_httpd, &index_uri);
  }
  
  config.server_port += 1;
  config.ctrl_port += 1;
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}

//-----------------------------------------------------------------------
void setup() {
  //WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detector
  Serial.begin(115200);

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
  config.frame_size = FRAMESIZE_VGA;  // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
  config.pixel_format = PIXFORMAT_JPEG; 
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  // for larger pre-allocated frame buffer.
  if(config.pixel_format == PIXFORMAT_JPEG){
    if(psramFound()){
      config.jpeg_quality = 10; // качество сжатия JPEG от 0 до 63 (меньшие значения -> более высокое качество и больший размер файла) 
      config.fb_count = 2;      // буферизуется 2 кадра
      config.grab_mode = CAMERA_GRAB_LATEST; // для предотвращения задержки при работе с буферизованными данными
    } 
    else { // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } 
  else { // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }
  
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
    Serial.printf("Camera init failed with error 0x%x", err);

  sensor_t *s = esp_camera_sensor_get();
  if (s->id.PID == OV2640_PID)
    Serial.println("Camera OV2640");
  else
    Serial.println("Unknown camera!");

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);   // flip vertically
#endif

  // Wi-Fi connection
  WiFi.mode(WIFI_STA);
  WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
  WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  startCameraServer();
  delay(3000);
  Serial.print("Camera Ready! Use 'http://");
  my_ip = WiFi.localIP().toString();
  Serial.print(my_ip.c_str());
  Serial.println("' to connect");  
  
  Serial1.begin(115200, SERIAL_8N1, RX, TX);  // Initialize UART1

  do { // If there is data in the UART1 buffer, we read it (clear the buffer)
    x = Serial1.read(); 
  } while(x != XON);
  Serial1.write('Y');
}

//---------------------------------------------------------------------
void loop() {
  delay(50);
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect();
    Serial.println("Lost connection, doing reconnect");
    delay(100);
    WiFi.begin(ssid, password);  
    delay(500);  
  }
}
