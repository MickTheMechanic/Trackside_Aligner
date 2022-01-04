#include <Wire.h>
#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"
#include "images.h"
#include <SPIFFS.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <EasyButton.h>
#include "Server.h"

#include <SimpleKalmanFilter.h>
#include <RunningMedian.h>



#define RX_BUF_SIZE  64
#define RXD2 16
#define TXD2 17
#define MPU9250_ADDRESS     0x68
#define ACC_FULL_SCALE_2_G  0x00
#define BUTTON_PIN          17
#define PORT_1 1
#define PORT_2 2

SimpleKalmanFilter filtered_camber (2, 2, 0.05);

SimpleKalmanFilter filtered_C (2, 2, 0.075);
SimpleKalmanFilter filtered_B (2, 2, 0.075);

RunningMedian filtered_toe = RunningMedian(20);

EasyButton button(BUTTON_PIN, 35, true, true);
SSD1306Wire display(0x3c, 5, 4);
OLEDDisplayUi ui     ( &display );
TwoWire Wire2 = TwoWire(1);
AsyncWebServer server(80);
IPAddress myIP;



char* ssid = "Trackside_Aligner";
char* password = "NoLimits!";


char rx_buffer[RX_BUF_SIZE];

int val = 0;

bool error = false;
char buffer [512];
char camberVal [16];
char toeVal [16];
char mm_buffer [32];
uint8_t currentPage = 1;
uint8_t frameCount = 3;
uint8_t Buf[14];



float alpha = 0.5;
float camberAngle;
float toeAngle;



void drawFrame1 (OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64 + x, 0 + y, mm_buffer);
  display->setFont(ArialMT_Plain_24);
  display->drawString(64 + x, 10 + y, "TOE:");
  display->drawString(64 + x, 34 + y, toeVal);
}

void drawFrame2 (OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_24);
  display->drawString(64 + x, 5 + y, "CAMBER");
  display->drawString(64 + x, 30 + y, camberVal);
}


void drawFrame3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {

  display->drawXbm(x + 34, y + 2, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64 + x, 38 + y,  "IP: 192.168.4.1");
}

FrameCallback frames[] = { drawFrame1, drawFrame2, drawFrame3};


void setup() {
  //button.begin();
  pinMode(18, INPUT);

  Serial.begin(256000);
  Serial2.begin(256000, SERIAL_8N1, 12, -1);



  ui.setTargetFPS(10);
  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);
  ui.setIndicatorPosition(BOTTOM);
  ui.setIndicatorDirection(LEFT_RIGHT);
  ui.setFrameAnimation(SLIDE_LEFT);
  ui.setFrames(frames, frameCount);
  //ui.setOverlays(overlays, overlaysCount);
  ui.disableAutoTransition();
  ui.init();
  SPIFFS.begin();
  Wire2.begin(33, 32);
  camber_sensor_write(MPU9250_ADDRESS, 29, 0x06);
  camber_sensor_write(MPU9250_ADDRESS, 28, ACC_FULL_SCALE_2_G);
  camber_sensor_write(MPU9250_ADDRESS, 108, 0x2F);


}





void loop() {
  int remainingTimeBudget = ui.update();
  if (remainingTimeBudget > 0) {
    button.read();
    if (button.pressedFor(1000)) {
      onPressedForDuration();
    }
    else if (button.wasPressed()) {
      onPressed();
    }
    else {
      if (currentPage == 1) {
        getToe();
      }
      else if (currentPage == 2) {
        getCamber();
      }

    }
    delay(remainingTimeBudget);
  }
}


void getCamber() {

  camber_sensor_read(MPU9250_ADDRESS, 0x3B, 14, Buf);
  int16_t ay = -(Buf[2] << 8 | Buf[3]);
  camberAngle = ay / -180.0;
  camberAngle = filtered_camber.updateEstimate(camberAngle);
  convert_degrees (camberAngle, camberVal);
}

void getToe() {

  wait_for_serial(1);
  if (error)snprintf(toeVal, 10, "Error!");
  else {
    float C = filtered_C.updateEstimate(handle_data(1));
    wait_for_serial(2);
    if (error)snprintf(toeVal, 10, "Error!");
    else {
      float B = filtered_B.updateEstimate(handle_data(2));
      float c = B - C;
      toeAngle = c * 360.0;
      toeAngle /= 3893.6;
      filtered_toe.add(toeAngle);
      sprintf(mm_buffer, "C:%.0fmm  B:%.0fmm", C, B);
      toeAngle = filtered_toe.getMedian();
      convert_degrees (toeAngle, toeVal);
    }
  }
}

void convert_degrees (float angle, char *buffer) {
  float degrees = (int)angle;
  float decimal = angle - (float)degrees;
  degrees = angle;
  if (degrees < 0.0 && degrees > -1.0) degrees = -0.1;
  else degrees = degrees - decimal;
  uint8_t minutes = fabs (decimal * 60.f);
  minutes = minutes + 5 / 2;
  minutes -= minutes % 5;
  if (minutes == 60) minutes = 55;
  snprintf(buffer, 10, "%+.0f° %02d'", degrees, minutes);
}

float handle_data(uint8_t port) {
  String rx_string(rx_buffer);
  String tmp_dist_str = rx_string.substring(rx_string.indexOf(";") + 1);
  tmp_dist_str = tmp_dist_str.substring(0, tmp_dist_str.indexOf(";"));
  //val = atoi(tmp_dist_str.c_str());
  return atoi(tmp_dist_str.c_str());

}


int wait_for_serial (uint8_t Serial_port)
{
  error = false;
  bool start_found = false;
  uint16_t char_cnt = 0;
  int timeout = 0;
  while (1)
  {
    int rx_byte;
    if (Serial_port == 1) {
      while (Serial.available() < 1) {
        delay (1);
        timeout ++;
        if (timeout == 100) {
          error = true;
          break;
        }
      }
      rx_byte = Serial.read();
    }
    else {
      while (Serial2.available() < 1) {
        delay (1);
        timeout ++;
        if (timeout == 100) {
          error = true;
          break;
        }
      }
      rx_byte = Serial2.read();
    }
    if (error) {
      break;
    }
    else {
      timeout = 0;
    }
    if (rx_byte == '\n') //end of string found
    {                            
      if (start_found == false)
        start_found = true;//Next char is a begin of the string to be analysed
      else
      {
        rx_buffer[char_cnt] = 0;
        return char_cnt;
      }
      char_cnt = 0;
    }
    else //not \n char
    {
      if (char_cnt < RX_BUF_SIZE)
      {
        rx_buffer[char_cnt] = (int)rx_byte;
        char_cnt++;
      }
    }
  }
}

void onPressed() {
  currentPage ++;
  if (currentPage > 3) {
    currentPage = 1;
    webserver_end();
    delay (200);
  }
  if (currentPage == 3)webserver_begin();
  ui.nextFrame();
  delay (300);
}

void onPressedForDuration() {

}


void run_webserver() {
  for (uint8_t i; i < 5; i++){
  getCamber();
  getToe();
  }
  snprintf
  (buffer, 512, "{ \"camber_left\":\"%s\", \"tire_left\":\"%.3f\", \"line_left\":\"%.3f\", \"camber_right\":\"test\", \"tire_right\":\"1\", \"line_right\":\"1\", \"toe_total\":\"%s\", \"toe_angle\":\"%.3f\", \"toe_angle_invert\":\"%.3f\" }",
   camberVal,
   camberAngle,
   camberAngle * -1,
   toeVal,
   toeAngle,
   toeAngle * -1);
}

void webserver_begin() {
  WiFi.softAP(ssid, password);
  myIP = WiFi.softAPIP();
  server.begin();
  server.on("/api/data", [](AsyncWebServerRequest * request) {
    run_webserver();
    request->send(200, "text/javascript", buffer);
  });
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/html", Server_HTML);
  });
  server.on("/FrontView.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/FrontView.png", "image/png");
  });
  server.on("/TopView.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/TopView.png", "image/png");
  });
}

void webserver_end() {
  server.end();
  server.reset();
  WiFi.disconnect();
  WiFi.mode(WIFI_MODE_NULL);
}



void camber_sensor_read(uint8_t Address, uint8_t Register, uint8_t Nbytes, uint8_t* Data)
{
  // Set register address
  Wire2.beginTransmission(Address);
  Wire2.write(Register);
  Wire2.endTransmission();

  // Read Nbytes
  Wire2.requestFrom(Address, Nbytes);
  uint8_t index = 0;
  while (Wire2.available())
    Data[index++] = Wire2.read();
}

void camber_sensor_write(uint8_t Address, uint8_t Register, uint8_t Data)
{
  // Set register address
  Wire2.beginTransmission(Address);
  Wire2.write(Register);
  Wire2.write(Data);
  Wire2.endTransmission();
}
