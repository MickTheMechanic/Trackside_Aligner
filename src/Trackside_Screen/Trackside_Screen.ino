#include <Arduino.h>
#include <math.h>
#include <Wire.h>
#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"
#include "images.h"
#include <SPIFFS.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <EasyButton.h>
#include <VL53L1X.h>
#include "Server.h"


#define MPU9250_ADDRESS     0x68
#define ACC_FULL_SCALE_2_G  0x00
#define BUTTON_PIN          17

VL53L1X toe_sensor_1;
VL53L1X toe_sensor_2;


EasyButton button(BUTTON_PIN, 35, true, true);
SSD1306Wire display(0x3c, 5, 4);
OLEDDisplayUi ui     ( &display );
TwoWire Wire2 = TwoWire(1);
AsyncWebServer server(80);
IPAddress myIP;

bool disable = false;
bool calibrating = false;
bool left = true;
bool sensorFail = false;
char FITMENT_SIDE [6];
char* ssid = "Trackside_Aligner";
char* password = "NoLimits!";
char buffer [255];
char camberVal [16];
char toeVal [16];
uint8_t currentPage = 1;
uint8_t frameCount = 4;
uint8_t Buf[14];
uint8_t averageVal = 16;
uint8_t minutes;
int16_t degrees;
uint8_t toeMinutes;
int16_t toeDegrees;
float C = 0;
float B = 0;
uint16_t sensor_timeout = 300; // milliseconds
uint32_t measurement_duration = 200000; //microseconds
float c;
float camberPosRight = 0;
float camberPosLeft = 0;
float delta = 0;
float camberValRight = 0;
float lineAngleRight = 0;
float tireAngleRight = 0;
float camberValLeft = 0;
float lineAngleLeft = 0;
float tireAngleLeft = 0;
float camberAngle;
float toeAngle;
float camberRaw;
float toeRaw;
float decimalDegrees;
float filteredDecimal;
const float calibrationVal = 1.85;
const float alpha = 0.1;


void drawFrame1 (OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_24);
  display->drawString(64 + x, 5 + y, "TOE");
  if (!disable) {
    if (calibrating) {
      display->setFont(ArialMT_Plain_16);
    }
    display->drawString(64 + x, 30 + y, toeVal);
  }
  else {
    display->setFont(ArialMT_Plain_10);
    display->drawString(64 + x, 30 + y, "SENSORS NOT FOUND!");
    display->drawString(64 + x, 41 + y, "CHECK CONNECTION");
  }
}

void drawFrame2 (OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_24);
  display->drawString(64 + x, 5 + y, "CAMBER");
  display->drawString(64 + x, 30 + y, camberVal);
}

void drawFrame3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64 + x, 4 + y,  "HOLD BUTTON TO");
  display->drawString(64 + x, 15 + y,  "SELECT FITMENT SIDE");
  display->setFont(ArialMT_Plain_24);
  display->drawString(64 + x, 27 + y,  FITMENT_SIDE);
}

void drawFrame4(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {

  display->drawXbm(x + 34, y + 2, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64 + x, 38 + y,  "IP: 192.168.4.1");
}

FrameCallback frames[] = { drawFrame1, drawFrame2, drawFrame3, drawFrame4};


void onPressed() {
  ui.nextFrame();
  currentPage ++;
  if (currentPage > 4)currentPage = 1;
  if (currentPage == 4)webserver_begin();
  else webserver_end();
}

void onPressedForDuration() {
  if (currentPage == 3) {
    if (left) {
      left = false;
      snprintf(FITMENT_SIDE, 6, "RIGHT");
    }
    else {
      left = true;
      snprintf(FITMENT_SIDE, 6, "LEFT");
    }
    display.clear();
    ui.switchToFrame(2);
    display.display();
  }
  else if (currentPage == 1) {
    if (disable) {
      disable = false;
      snprintf(toeVal, 10, "Connecting...");
    }
  }
}


void run_webserver() {
  getCamber();
  getToe();
  snprintf
  (buffer,
   255,
   "[%.4f, %.4f, %d, %d, %d, 0, 0, 0, 0, 0, 0, 0, %d, %d, 0, 0, 0, 0, 0, 0, 0 ]",
   tireAngleRight,
   lineAngleRight,
   degrees,
   minutes,
   toeDegrees,
   toeMinutes);

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


void getCamber() {
  camberRaw = 0;
  for (uint8_t i = 0; i < averageVal; i++) {
    camber_sensor_read(MPU9250_ADDRESS, 0x3B, 14, Buf);
    int16_t ay = -(Buf[2] << 8 | Buf[3]);
    camberRaw += ay / -180.0;
  }
  camberRaw /= averageVal;
  //camberRaw += calibrationVal;
  camberAngle = (alpha * camberRaw) + (1.0 - alpha) * camberAngle;
  degrees = (int)camberAngle;
  decimalDegrees = camberAngle - (float)degrees;
  //filteredDecimal = (alpha * decimalDegrees) + (1.0 - alpha) * filteredDecimal;
  minutes = fabs (decimalDegrees * 60.f);
  //if (minutes > 1) minutes += 1;
  minutes = minutes + 5 / 2;
  minutes -= minutes % 5;
  if (minutes == 60) {
    minutes = 55;
  }
  if (minutes < 10) {
    if (camberAngle < 0) {
      snprintf(camberVal, 10, "-%.0f°0%d'", fabs (degrees), minutes);
    }
    else {
      snprintf(camberVal, 10, "+%d°0%d'", degrees, minutes);
    }
  }
  else {
    if (camberAngle < 0) {
      snprintf(camberVal, 10, "-%.0f°%d'", fabs (degrees), minutes);
    }
    else {
      snprintf(camberVal, 10, "+%d°%d'", degrees, minutes);
    }
  }
  delta = camberPosRight - camberAngle;
  tireAngleRight = camberPosRight - delta;
  lineAngleRight = tireAngleRight * -1;
}



const int numReadings = 30;
int readIndex = 0;

int B_readings[numReadings];
int B_total = 0;
int B_average = 0;
int C_readings[numReadings];
int C_total = 0;
int C_average = 0;
int timeoutcount = 0;


void getToe() {

  toe_sensor_2.readSingle(false);
  while (1) {
    if (toe_sensor_2.dataReady()) {
      break;
    }
  }
  B = (alpha * B_corrected(toe_sensor_2.read(false))) + (1.0 - alpha) * B;


  toe_sensor_1.readSingle(false);
  while (1) {
    if (toe_sensor_1.dataReady()) {
      break;
    }
  }
  C = (alpha * C_corrected(toe_sensor_1.read(false))) + (1.0 - alpha) * C;



  //B = (alpha * (toe_sensor_2.read()+30)) + (1.0 - alpha) * B;
  //C = (alpha * (toe_sensor_1.read()+45)) + (1.0 - alpha) * C;

  B_total -= B_readings[readIndex];
  B_readings[readIndex] = B;
  B_total += B_readings[readIndex];

  C_total -= C_readings[readIndex];
  C_readings[readIndex] = C;
  C_total += C_readings[readIndex];


  readIndex = readIndex + 1;
  if (readIndex >= numReadings) {
    readIndex = 0;
  }

  B_average = B_total / numReadings;
  C_average = C_total / numReadings;





  if (!toe_sensor_1.timeoutOccurred() && !toe_sensor_2.timeoutOccurred()) {


    /*Serial.println ("blue purple (raw, filtered)");
      Serial.println ("B");
      Serial.println (B);
      Serial.println(",");
      Serial.println ("C");
      //Serial.println(rawC);
      Serial.println(C);
      Serial.println("-------------");*/


    c = B_average - C_average;
    toeRaw = c * 360.0;
    toeRaw /= 3140.0; //500mm
    //toeRaw /= 2198.0;//350mm


    if (toeRaw > 45 | toeRaw < -45) {
      snprintf(toeVal, 16, "Calibrating...");
      timeoutcount ++;
      if (timeoutcount == 50) {
        for (int i = 0; i < numReadings; i++) {
          B_readings[i] = 0;
          C_readings[i] = 0;
          timeoutcount = 0;
        }
      }
      if (!calibrating) {
        calibrating = true;
      }
    }
    else {
      if (calibrating) {
        calibrating = false;
        timeoutcount = 0;
      }

      toeAngle = (alpha * toeRaw) + (1.0 - alpha) * toeAngle;
      toeDegrees = (int)toeAngle;
      decimalDegrees = toeAngle - (float)toeDegrees;
      toeMinutes = fabs (decimalDegrees * 60.f);
      toeMinutes = toeMinutes + 5 / 2;
      toeMinutes -= toeMinutes % 5;
      if (toeMinutes == 60) {
        toeMinutes = 55;
      }
      if (toeMinutes < 10) {
        if ( toeAngle < 0) {
          snprintf(toeVal, 10, "-%.0f°0%d'", fabs (toeDegrees), toeMinutes);
        }
        else {
          snprintf(toeVal, 10, "+%d°0%d'", toeDegrees, toeMinutes);
        }
      }
      else {
        if ( toeAngle < 0) {
          snprintf(toeVal, 10, "-%.0f°%d'", fabs (toeDegrees), toeMinutes);
        }
        else {
          snprintf(toeVal, 10, "+%d°%d'", toeDegrees, toeMinutes);
        }
      }
      //delta = camberPosRight - camberAngle;
      //tireAngleRight = camberPosRight - delta;
      //lineAngleRight = tireAngleRight * -1;
      //}
    }
  }
  else {
    disable = true;
  }
}

float C_corrected (uint32_t x) { //Must be used for sensor 1. no matter if port c or b is used.
  return +1.048528623775596120442283642364410252156404627070179406947333755e-26 * pow(x, 11.0) + -1.736087260967192113684413175287683459230181731394319780835535002e-22 * pow(x, 10.0) + 1.299338966125989370889474143709369687823881569372612690744454656e-18 * pow(x, 9.0) + -5.801884512713824798445107393867097443311759888604790224509440997e-15 * pow(x, 8.0) + 1.717241253239583981748495157121357520673226339692804642139909451e-11 * pow(x, 7.0) + -3.537212987138216263412731754615318613893007895708287189971876767e-8 * pow(x, 6.0) + 0.00005173635334707580031038000821019196108325354661722487936355382629 * pow(x, 5.0) + -0.05372775088391900392785477772472858526519056355260512703577163986 * pow(x, 4.0) + 38.82038796399021234582211681787996165586847700152264082281531805 * pow(x, 3.0) + -18584.66864835792302919470758429877040484283361755526714387791659 * pow(x, 2.0) + 5305075.740348194864200292820422992044331808110615085008694215835 * pow(x, 1.0) + -684010062.2981505405453520208160884429591770511378812011352933365 * pow(x, 0.0);
}

float B_corrected (uint32_t x) { //Must be used for sensor 2. no matter if port c or b is used.
  return +6.319218381488328493653230274343923406269170593311528305808648957e-27 * pow(x, 11.0) + -1.055084572283266908050751716728855411450031731629723236368430208e-22 * pow(x, 10.0) + 7.963578115356476532260027412552126729478670590423294138342670023e-19 * pow(x, 9.0) + -3.586455530213215009448336920651754525720598310399356804315353168e-15 * pow(x, 8.0) + 1.070736737797824706272159064751703890184301073480274002472301573e-11 * pow(x, 7.0) + -2.224932544056935983132270704208869063405021270795330080855453483e-8 * pow(x, 6.0) + 0.00003283301122159604753130412978974389110871465362722171840577607865 * pow(x, 5.0) + -0.03440604251071080261548732413096837997532229659073405553447859938 * pow(x, 4.0) + 25.08921232561086276008274065393844573059394793751399128491945754 * pow(x, 3.0) + -12124.17622074377338773575476571653372590403299357359725564666359 * pow(x, 2.0) + 3494209.097557018625115494030267594988742190573416001182837768847 * pow(x, 1.0) + -454967827.0383628740999305385175888908757784094159861983788981725 * pow(x, 0.0);
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





void setup() {
  button.begin();
  button.onPressed(onPressed);
  button.onPressedFor(1000, onPressedForDuration);
  pinMode(18, INPUT);
  Serial.begin(115200);

  ui.setTargetFPS(30);
  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);
  ui.setIndicatorPosition(BOTTOM);
  ui.setIndicatorDirection(LEFT_RIGHT);
  ui.setFrameAnimation(SLIDE_LEFT);
  ui.setFrames(frames, frameCount);
  //ui.setOverlays(overlays, overlaysCount);
  ui.disableAutoTransition();
  snprintf(FITMENT_SIDE, 6, "LEFT");
  ui.init();

  SPIFFS.begin();
  Wire.begin(5, 4);
  Wire2.begin(33, 32);
  camber_sensor_write(MPU9250_ADDRESS, 29, 0x06);
  camber_sensor_write(MPU9250_ADDRESS, 28, ACC_FULL_SCALE_2_G);
  camber_sensor_write(MPU9250_ADDRESS, 108, 0x2F);



  if (!toe_sensor_1.init() | !toe_sensor_2.init()) {
    disable = true;
  }
  toe_sensor_1.setTimeout(500);
  toe_sensor_1.setDistanceMode(VL53L1X::Long);
  toe_sensor_1.setMeasurementTimingBudget(100000);
  toe_sensor_1.setROISize(4, 4);
  //toe_sensor_1.startContinuous(100);
  toe_sensor_2.setBus(&Wire2);
  toe_sensor_2.setTimeout(500);
  toe_sensor_2.setDistanceMode(VL53L1X::Long);
  toe_sensor_2.setMeasurementTimingBudget(100000);
  toe_sensor_2.setROISize(4, 4);
  //toe_sensor_2.startContinuous(100);


  for (int i = 0; i < numReadings; i++) {
    B_readings[i] = 0;
    C_readings[i] = 0;
  }


}



void loop() {
  int remainingTimeBudget = ui.update();
  if (remainingTimeBudget > 0) {
    //getToe();
    //Serial.println( toe_sensor_1.readRangeSingleMillimeters());

    if (currentPage == 1) {
      getToe();
    }
    if (currentPage == 2) {
      getCamber();
    }
    button.read();
    delay(remainingTimeBudget);
  }
}
