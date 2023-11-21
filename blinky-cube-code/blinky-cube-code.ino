union CubeData
{
  struct
  {
    int16_t state;          
    int16_t watchdog;       
    int16_t chipTemp; 
    int16_t sensor1;       
    int16_t sensor2;        
    int16_t sensor3;        
  };
  byte buffer[12];
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
float fsamples = 2000;

void setupServerComm()
{
  // Optional setup to overide defaults
//  Serial.begin(115200);
  BlinkyPicoWCube.setChattyCathy(false);
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

  fsensor1 = fsensor1 + ((float) analogRead(A0) - fsensor1) / fsamples;
  fsensor2 = fsensor2 + ((float) analogRead(A1) - fsensor2) / fsamples;
  fsensor3 = fsensor3 + ((float) analogRead(A2) - fsensor3) / fsamples;
  
  
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
  cubeData.chipTemp = (int16_t) (analogReadTemp() * 100.0);
  cubeData.sensor1 = (int16_t) (fsensor1 * 8.0);
  cubeData.sensor2 = (int16_t) (fsensor2 * 8.0);
  cubeData.sensor3 = (int16_t) (fsensor3 * 8.0);

/*
  Serial.print(cubeData.chipTemp);
  Serial.print(",");
  Serial.print(cubeData.sensor1);
  Serial.print(",");
  Serial.print(cubeData.sensor2);
  Serial.print(",");
  Serial.println(cubeData.sensor3);
*/
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
