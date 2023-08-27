import rp2
import network
import ubinascii
from machine import Pin, unique_id, deepsleep, ADC, reset
from creds import creds
from umqttsimple import MQTTClient
from time import sleep as sleep, ticks_ms 
from ubinascii import hexlify
import ujson as json
import gc
gc.collect()

chitChat = False
deepSleepEnabled = True

commLed = Pin('LED', Pin.OUT)
#commLed = Pin(2, Pin.OUT)
commLed.on()

initSettings = {}
try:
    with open('initSettings.json', 'r') as f:
        initSettings = json.load(f)
except:
    if (chitChat): print("initSettings not found.")
    sleep(60)
    reset()
    

initSettings['watchdog'] = initSettings['watchdog'] + 1;
if initSettings['watchdog'] > 32767:
    initSettings['watchdog'] = 0
if (chitChat): print("watchdog: " + str(initSettings['watchdog']))
try:
    with open('initSettings.json', 'w') as f:
        json.dump(initSettings, f)
except:
    if (chitChat): print("Could not save initSettings.json")
    
def connectWifi(timeoutIn=20, chitChat=False, country='SE'):
    rp2.country(country)
    Pin(23, Pin.OUT).high() # turn on WiFi module
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    mac = hexlify(network.WLAN().config('mac'),':').decode()
    if (chitChat):
        print('mac = ' + mac)
        print('wifi channel = ' + str(wlan.config('channel')))
        print('wifi essid = ' + wlan.config('essid'))
        print('wifi txpower = ' + str(wlan.config('txpower')))
    wlan.connect(creds['ssid'], creds['wifiPassword'])
    timeout = timeoutIn
    while timeout > 0:
        if wlan.status() < 0 or wlan.status() >= 3:
            break
        timeout -= 1
        if (chitChat): print('Waiting for connection...')
        sleep(1)
    wlan_status = wlan.status()
    if wlan_status != 3:
        if (chitChat): print('Wi-Fi connection failed')
        return None
    else:
        if (chitChat): print('Wifi Connected')
        status = wlan.ifconfig()
        if (chitChat): print('ip = ' + status[0])
        return wlan
    return None

def oneShotMqttPublish(wlan, msg, chitChat=False, keepalive=0):
    if wlan == None: return None
    client_id   = hexlify(unique_id())
    topic = creds['box']
    topic = topic + "/" + creds['cubeType']
    topic = topic + "/" + creds['trayType']
    topic = topic + "/" + creds['trayName']
    topic = topic + "/reading"
    if (chitChat): print('MQTT topic: ' + str(topic))
    if (chitChat): print('MQTT msg: ' + str(msg))
    try:
        client = MQTTClient(client_id, creds['mqttServer'], 1883, creds['mqttUsername'], creds['mqttPassword'], keepalive=keepalive)
        client.connect()
        if (chitChat): print('Connected to %s MQTT broker' % (creds['mqttServer']))
        client.publish(topic, msg)
        client.disconnect()
        return client
    except:
        if (chitChat): print('Failed to connect to MQTT broker.')
        return None
    return None

def readAdc(pin):
    vread = pin.read_u16() * (3.3 / 65535)
    return vread
    
sleep(1)
adcPin1 = ADC(26)
adcPin2 = ADC(27)
adcPin3 = ADC(28)

sensor1 = readAdc(adcPin1)
sensor2 = readAdc(adcPin2)
sensor3 = readAdc(adcPin3)

tstart = ticks_ms()
icount = initSettings['adcSamples']
while icount > 0:
    sensor1 = sensor1 + (readAdc(adcPin1) - sensor1) / icount
    sensor2 = sensor2 + (readAdc(adcPin2) - sensor2) / icount
    sensor3 = sensor3 + (readAdc(adcPin3) - sensor3) / icount
    icount -= 1
sensor1 = round(sensor1,4)
sensor2 = round(sensor2,4)
sensor3 = round(sensor3,4)

vsys = readAdc(ADC(29))
vsys = round(vsys * 3.0,4)
ictemp = 27 - (readAdc(ADC(4)) - 0.706)/0.001721
ictemp = round(ictemp,4)
deltatT = ticks_ms() - tstart;
if (chitChat): print("")
if (chitChat): print("Num. of Samples: " + str(initSettings['adcSamples']))
if (chitChat): print("ADC acquire time (ms): " + str(deltatT))
if (chitChat): print("Sensor1 (V): " + str(sensor1))
if (chitChat): print("Sensor2 (V): " + str(sensor2))
if (chitChat): print("Sensor3 (V): " + str(sensor3))
if (chitChat): print("vsys    (V): " + str(vsys))
if (chitChat): print("ictemp  (C): " + str(ictemp))
if (chitChat): print("")

readings = {'state' : 0, 'watchdog': initSettings['watchdog'], 'chipTemp': ictemp, 'sensor1' : sensor1, 'sensor2' : sensor2, 'sensor3' : sensor3, 'vsys' : vsys}

wlan = connectWifi(timeoutIn=initSettings['wifiTimeout'], chitChat=chitChat, country='SE')    
oneShotMqttPublish(wlan=wlan, msg=json.dumps(readings), chitChat=chitChat, keepalive=initSettings['mqttKeepalive'])

commLed.off()
if deepSleepEnabled:
    sleep(0.1)
    if wlan != None: wlan.disconnect()
    if wlan != None: wlan.active(False)
    sleep(0.1)
    Pin("WL_GPIO1", Pin.OUT).low()  #smps low power mode
    sleep(0.1)
    if wlan != None: Pin(23, Pin.OUT).low() # turn off WiFi module
    sleep(0.1)
    deepsleep(initSettings['deepSleepTime']) # milliseconds
          
        

