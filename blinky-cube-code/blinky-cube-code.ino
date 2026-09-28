#include <BlinkyPicoW.h>

// --- Configuration Constants ---
constexpr int BLINKY_DIAG    = 0;
constexpr int CUBE_DIAG      = 0;
constexpr int COMM_LED_PIN   = 2;
constexpr int RST_BUTTON_PIN = 3;
constexpr int NUMCHAN        = 3;

constexpr int ADC_PINS[NUMCHAN] = {A0, A1, A2};
constexpr float ADC_REF_VOLTS   = 3.3f;
constexpr float ADC_MAX_COUNTS  = 4096.0f;
constexpr float COND_MULT  = 10.0f;

// --- Data Structures ---
struct CubeSetting 
{
  uint16_t publishInterval;
  uint16_t nsamples;
};

struct CubeReading
{
  float sensor[NUMCHAN];
  float bandWidth;
};

struct CubeArm {
  bool sensor[NUMCHAN];
  bool bandWidth = true;
};

// --- Global Variables ---
CubeSetting setting;
CubeReading reading;
CubeReading readingLow;
CubeReading readingHigh;
CubeArm readingArm;

unsigned long lastPublishTime = 0;
uint32_t digCount = 0;
float adc[NUMCHAN];

// --- Helper Functions ---
template <typename T>
inline bool outsideLimits(T current, T low, T high) {
  return (current < low) || (current > high);
}

inline float readADCInVolts(int pin) {
  return ADC_REF_VOLTS * (static_cast<float>(analogRead(pin)) / ADC_MAX_COUNTS);
}

// --- Setup Functions ---
void setupBlinky() {
  if (BLINKY_DIAG > 0) {
    Serial.begin(9600);
  }

  // MQTT & Hardware Configuration
  BlinkyPicoW.setMqttKeepAlive(15);
  BlinkyPicoW.setMqttSocketTimeout(4);
  BlinkyPicoW.setMqttPort(1883);
  BlinkyPicoW.setMqttLedFlashMs(100);
  BlinkyPicoW.setHdwrWatchdogMs(8000);

  BlinkyPicoW.begin(BLINKY_DIAG, COMM_LED_PIN, RST_BUTTON_PIN, true, sizeof(setting), sizeof(reading));
}


void setupCube() {
  if (BLINKY_DIAG < 1 && CUBE_DIAG > 0) {
    Serial.begin(9600);
  }

  analogReadResolution(12);
  setting.publishInterval = 2000;
  setting.nsamples = 2;

  for (int i = 0; i < NUMCHAN; ++i) {
    adc[i] = readADCInVolts(ADC_PINS[i]);
    readingArm.sensor[i] = true;
  }
  reading.sensor[0] = conductance(adc[0], adc[2]);
  reading.sensor[1] = conductance(adc[1], adc[2]);
  reading.sensor[2] = adc[2];

  digCount = 1;
  lastPublishTime = millis();
}


// --- Core Loop Functions ---
void loopCube() 
{
  const unsigned long now = millis();

  // 1. Regular Timed Publishing
  if ((now - lastPublishTime) >= setting.publishInterval) 
  {
    const float safeInterval = (setting.publishInterval > 0) ? static_cast<float>(setting.publishInterval) : 1.0f;
    const float safeSamples  = (setting.nsamples > 0) ? static_cast<float>(setting.nsamples) : 1.0f;

    reading.bandWidth = 500.0f * (static_cast<float>(digCount) / safeInterval) / safeSamples;
    lastPublishTime = now;

    if (BlinkyPicoW.publishCubeData(reinterpret_cast<uint8_t*>(&setting), reinterpret_cast<uint8_t*>(&reading), false)) 
    {
      for (int i = 0; i < NUMCHAN; ++i) 
      {
        if (!outsideLimits(reading.sensor[i], readingLow.sensor[i], readingHigh.sensor[i])) {
          readingArm.sensor[i] = true;
        }
      }
      digCount = 0;
    }
  }

 // 2. ADC Sampling & Threshold Breach Checks
  for (int i = 0; i < NUMCHAN; ++i) 
  {
    const float newAdc = readADCInVolts(ADC_PINS[i]);
    const float nsamples = (setting.nsamples > 0) ? static_cast<float>(setting.nsamples) : 1.0f;
    adc[i] += (newAdc - adc[i]) / nsamples;
  }
  reading.sensor[0] = conductance(adc[0], adc[2]);
  reading.sensor[1] = conductance(adc[1], adc[2]);
  reading.sensor[2] = adc[2];
  ++digCount;
  
  for (int i = 0; i < NUMCHAN; ++i) 
  {
    if (BlinkyPicoW.isInitialized()) {  
      if (outsideLimits(reading.sensor[i], readingLow.sensor[i], readingHigh.sensor[i])) {
        if (readingArm.sensor[i]) {
          const bool published = BlinkyPicoW.publishCubeData(
            reinterpret_cast<uint8_t*>(&setting), 
            reinterpret_cast<uint8_t*>(&reading), 
            true
          );
          
          readingArm.sensor[i] = !published;
          if (published) {
            lastPublishTime = now;
          }
        }
      }
    }
  }


  // 3. Check for New MQTT Settings
  const bool newSettings = BlinkyPicoW.retrieveCubeSetting(
    reinterpret_cast<uint8_t*>(&setting), 
    reinterpret_cast<uint8_t*>(&readingLow), 
    reinterpret_cast<uint8_t*>(&readingHigh)
  );

  if (newSettings) 
  {
    if (setting.publishInterval < 1000) setting.publishInterval = 1000;
    if (setting.nsamples < 1)         setting.nsamples = 1;
    for (int i = 0; i < NUMCHAN; ++i) 
    {
      adc[i] = readADCInVolts(ADC_PINS[i]);
    }
    reading.sensor[0] = conductance(adc[0], adc[2]);
    reading.sensor[1] = conductance(adc[1], adc[2]);
    reading.sensor[2] = adc[2];
    digCount = 1;
  }
  
}
float conductance(float sensorVal, float openSensorVal)
{
    float traw = sensorVal / ADC_REF_VOLTS;
    float topen = openSensorVal / ADC_REF_VOLTS;
    float fcond = COND_MULT * (topen * (1.0 - traw) - traw * (1.0 - topen)) / (topen * traw);
    if (fcond < 0.1) fcond = 0.1;
    if (fcond > 65535) fcond = 65535.0;
    return fcond;
}
