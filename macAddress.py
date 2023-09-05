import network
import ubinascii
from machine import Pin, unique_id, deepsleep, ADC, reset
from ubinascii import hexlify

wlan = network.WLAN(network.STA_IF)
wlan.active(True)
mac = hexlify(network.WLAN().config('mac'),':').decode()
print('mac = ' + mac)
