#define BLINKY_DIAG        0
#define CUBE_DIAG          0
#define COMM_LED_PIN       2
#define RST_BUTTON_PIN     3
#define COND_MULT         10.0
#include <BlinkyPicoW.h>

struct CubeSetting
{
  uint16_t publishInterval;
  uint16_t nsamples;
};
CubeSetting setting;

struct CubeReading
{
  uint16_t sensor1;
  uint16_t sensor2;
  uint16_t refVolts;
  uint16_t bandWidth;
};
CubeReading reading;

unsigned long lastPublishTime;


float fsensor1;
float fsensor2;
float fsensor3;
int digCount;

void setupBlinky()
{
  if (BLINKY_DIAG > 0) Serial.begin(9600);

  BlinkyPicoW.setMqttKeepAlive(15);
  BlinkyPicoW.setMqttSocketTimeout(4);
  BlinkyPicoW.setMqttPort(1883);
  BlinkyPicoW.setMqttLedFlashMs(100);
  BlinkyPicoW.setHdwrWatchdogMs(8000);

  BlinkyPicoW.begin(BLINKY_DIAG, COMM_LED_PIN, RST_BUTTON_PIN, true, sizeof(setting), sizeof(reading));
}

void setupCube()
{
  if (CUBE_DIAG > 0) Serial.begin(9600);
  setting.publishInterval = 4000;
  lastPublishTime = millis();
  setting.nsamples = 2000;

  analogReadResolution(12);
  analogRead(A0);
  analogRead(A1);
  analogRead(A2);
  delay(100);
  fsensor1 = (float) analogRead(A0);
  fsensor2 = (float) analogRead(A1);
  fsensor3 = (float) analogRead(A2);
  digCount = 1;

}

void loopCube()
{
  unsigned long now = millis();

  fsensor1 = fsensor1 + ((float) analogRead(A0) - fsensor1) / ((float) setting.nsamples);
  fsensor2 = fsensor2 + ((float) analogRead(A1) - fsensor2) / ((float) setting.nsamples);
  fsensor3 = fsensor3 + ((float) analogRead(A2) - fsensor3) / ((float) setting.nsamples);
 ++digCount;
  
  
  if ((now - lastPublishTime) > setting.publishInterval)
  {
    float fbandwidth = 500.0 * ( ((float) digCount) / ((float) setting.publishInterval) ) / ((float) setting.nsamples);
    reading.bandWidth = (uint16_t) fbandwidth;   
    digCount = 0;
    
    readSensor();
    lastPublishTime = now;
    boolean successful = BlinkyPicoW.publishCubeData((uint8_t*) &setting, (uint8_t*) &reading, false);
  }  
  boolean newSettings = BlinkyPicoW.retrieveCubeSetting((uint8_t*) &setting);
  if (newSettings)
  {
    if (setting.publishInterval < 1000) setting.publishInterval = 1000;
    if (setting.nsamples < 1) setting.nsamples = 1;
    fsensor1 = (float) analogRead(A0);
    fsensor2 = (float) analogRead(A1);
    fsensor3 = (float) analogRead(A2);
  }
  
}
void readSensor()
{
  
  reading.sensor1 = conductance(fsensor1, fsensor3);
  reading.sensor2 = conductance(fsensor2, fsensor3);
  reading.refVolts = (uint16_t) (3300 * fsensor3 / 4096.0);

  if (CUBE_DIAG > 0)
  {
    Serial.print(reading.sensor1);
    Serial.print(",");
    Serial.print(reading.sensor2);
    Serial.print(",");
    Serial.println(reading.refVolts);
  }

}
uint16_t conductance(float sensorVal, float openSensorVal)
{
    float traw = sensorVal / 4096.0;
    float topen = openSensorVal / 4096.0;
    float fcond = 10.0 * COND_MULT * (topen * (1.0 - traw) - traw * (1.0 - topen)) / (topen * traw);
    if (fcond < 0.1) fcond = 0.1;
    if (fcond > 65535) fcond = 65535.0;
    return (uint16_t) fcond;
}
