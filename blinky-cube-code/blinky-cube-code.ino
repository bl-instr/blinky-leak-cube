union CubeData
{
  struct
  {
    int16_t state;          
    int16_t watchdog;       
    int16_t chipTemp; 
    int16_t chipTemp_off; 
    int16_t condMult;
    int16_t samples; 
    int16_t sensor1raw; 
    int16_t sensor1open; 
    int16_t sensor1short;   
    int16_t sensor1;       
    int16_t sensor2raw;    
    int16_t sensor2open;   
    int16_t sensor2short;   
    int16_t sensor2;        
    int16_t sensor3raw;    
    int16_t sensor3open;   
    int16_t sensor3short;   
    int16_t sensor3;        
  };
  byte buffer[36];
};
CubeData cubeData;

#include "BlinkyPicoWCube.h"


int commLEDPin = 2;
int commLEDBright = 255; 
int resetButtonPin = 3;

unsigned long lastPublishTime;
unsigned long publishInterval = 2000;

float fsensor1;
float fsensor2;
float fsensor3;

void setupServerComm()
{
  // Optional setup to overide defaults
  Serial.begin(115200);
  BlinkyPicoWCube.setChattyCathy(true);
  BlinkyPicoWCube.setWifiTimeoutMs(20000);
  BlinkyPicoWCube.setWifiRetryMs(20000);
  BlinkyPicoWCube.setMqttRetryMs(3000);
  BlinkyPicoWCube.setResetTimeoutMs(10000);
  BlinkyPicoWCube.setHdwrWatchdogMs(8000);
  BlinkyPicoWCube.setBlMqttKeepAlive(8);
  BlinkyPicoWCube.setBlMqttSocketTimeout(4);
  BlinkyPicoWCube.setMqttLedFlashMs(10);
  BlinkyPicoWCube.setWirelesBlinkMs(100);
  BlinkyPicoWCube.setMaxNoMqttErrors(5);
  
  // Must be included
  BlinkyPicoWCube.init(commLEDPin, commLEDBright, resetButtonPin);
}

void setupCube()
{
  lastPublishTime = millis();
  cubeData.state = 1;
  cubeData.watchdog = 0;
  cubeData.chipTemp_off = 0;
  cubeData.condMult = 1000;
  cubeData.sensor1open = 2827;
  cubeData.sensor1short = 14;
  cubeData.sensor2open = 2829;
  cubeData.sensor2short = 14;
  cubeData.sensor3open = 2835;
  cubeData.sensor3short = 15;
  cubeData.samples = 1000;
  analogReadResolution(12);
  analogRead(A0);
  analogRead(A1);
  analogRead(A2);
  delay(1000);
  fsensor1 = (float) analogRead(A0);
  fsensor2 = (float) analogRead(A1);
  fsensor3 = (float) analogRead(A2);

}

void cubeLoop()
{
  unsigned long nowTime = millis();

  fsensor1 = fsensor1 + ((float) analogRead(A0) - fsensor1) / ((float)cubeData.samples);
  fsensor2 = fsensor2 + ((float) analogRead(A1) - fsensor2) / ((float)cubeData.samples);
  fsensor3 = fsensor3 + ((float) analogRead(A2) - fsensor3) / ((float)cubeData.samples);
  
  
  if ((nowTime - lastPublishTime) > publishInterval)
  {
    readSensor();
    lastPublishTime = nowTime;
    cubeData.watchdog = cubeData.watchdog + 1;
    if (cubeData.watchdog > 32760) cubeData.watchdog= 0 ;
    BlinkyPicoWCube::publishToServer();

  }  
  
}
void readSensor()
{
  cubeData.sensor1raw = (int16_t) fsensor1;
  cubeData.sensor2raw = (int16_t) fsensor2;
  cubeData.sensor3raw = (int16_t) fsensor3;
  cubeData.chipTemp = (int16_t) (analogReadTemp() * 100.0) - cubeData.chipTemp_off;
  cubeData.sensor1 = conductance(cubeData.sensor1raw, cubeData.sensor1open, cubeData.sensor1short);
  cubeData.sensor2 = conductance(cubeData.sensor2raw, cubeData.sensor2open, cubeData.sensor2short);
  cubeData.sensor3 = conductance(cubeData.sensor3raw, cubeData.sensor3open, cubeData.sensor3short);
/*
  Serial.print(cubeData.sensor1raw);
  Serial.print(",");
  Serial.print(cubeData.sensor2raw);
  Serial.print(",");
  Serial.println(cubeData.sensor3raw);

  Serial.print(cubeData.sensor1);
  Serial.print(",");
  Serial.print(cubeData.sensor2);
  Serial.print(",");
  Serial.println(cubeData.sensor3);
*/
}

int16_t conductance(int16_t raw, int16_t openVal, int16_t shortVal)
{
  float fraw = (float) raw;
  float fopenVal = (float) openVal;
  float fshortVal = (float) shortVal;
  float fcond = 0.0;
  if (raw <= shortVal) fraw = fshortVal + 1.0;
  if (raw >= openVal)  fraw = fopenVal  - 1.0;
  float traw = (fraw     - fshortVal) / (4096.0);
  float topen= (fopenVal - fshortVal) / (4096.0);
  fcond = ((float) cubeData.condMult) * (topen * (1.0 - traw) - traw * (1.0 - topen)) / (topen * traw);
  if (fcond < 0.0 ) fcond = 0.0;
  if (fcond > 32766.0 ) fcond = 32766.0;
  return (int16_t)  fcond; 
}


void handleNewSettingFromServer(uint8_t address)
{
  switch(address)
  {
    case 0:
      break;
    default:
      break;
  }
}
