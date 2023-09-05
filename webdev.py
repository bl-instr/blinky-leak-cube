import usocket as socket        #importing socket
import network            #importing network
from creds import creds
import gc
gc.collect()




ssid = creds['trayType'] + '-' + creds['trayName']                  #Set access point name 
password = 'blinky-lite'      #Set your access point password


ap = network.WLAN(network.AP_IF)
ap.config(essid=ssid, password=password)
ap.active(True)            #activating

while ap.active() == False:
  pass
print('Connection is successful')
print(ap.ifconfig())
def web_page():
  html = """
<html>
  <head>
    <title>Blinky-Lite Credentials</title>
    <style>
      body{background-color: #083357 !important;font-family: Arial;}
      .labeltext{color: white;font-size:250%;}
      .formtext{color: black;font-size:250%;}
      .cell{padding-bottom:25px;}
    </style>
  </head>
  <body>
    <h1 style=\"color:white;font-size:300%;text-align: center;\">Blinky-Lite Credentials</h1>
    <hr>
    <div>
      <form action=\"/disconnected\" method=\"POST\">
        <table align=\"center\">
          <tr>
            <td class=\"cell\"><label for=\"ssid\" class=\"labeltext\">SSID</label></td>
            <td class=\"cell\"><input name=\"ssid\" id=\"ssid\" type=\"text\" value=\"""" + creds['ssid'] + """\" class=\"formtext\"/></td>
          </tr>
          <tr>
            <td class=\"cell\"><label for=\"pass\" class=\"labeltext\">Wifi Password</label></td>
            <td class=\"cell\"><input name=\"pass\" id=\"pass\" type=\"password\" value=\"""" + creds['wifiPassword'] + """\" class=\"formtext\"/></td>
          </tr>
          <tr>
            <td class=\"cell\"><label for=\"serv\" class=\"labeltext\">MQTT Server</label></td>
            <td class=\"cell\"><input name=\"serv\" id=\"serv\" type=\"text\" value=\"""" + creds['mqttServer'] + """\" class=\"formtext\"/></td>
          </tr>
          <tr>
            <td class=\"cell\"><label for=\"unam\" class=\"labeltext\">MQTT Username</label></td>
            <td class=\"cell\"><input name=\"unam\" id=\"unam\" type=\"text\" value=\"""" + creds['mqttUsername'] + """\" class=\"formtext\"/></td>
          </tr>
          <tr>
            <td class=\"cell\"><label for=\"mpas\" class=\"labeltext\">MQTT Password</label></td>
            <td class=\"cell\"><input name=\"mpas\" id=\"mpas\" type=\"password\" value=\"""" + creds['mqttPassword'] + """\" class=\"formtext\"/></td>
          </tr>
          <tr>
            <td class=\"cell\"><label for=\"bbox\" class=\"labeltext\">Box</label></td>
            <td class=\"cell\"><input name=\"bbox\" id=\"bbox\" type=\"text\" value=\"""" + creds['box'] + """\" class=\"formtext\"/></td>
          </tr>
          <tr>
            <td class=\"cell\"><label for=\"tryt\" class=\"labeltext\">Tray Type</label></td>
            <td class=\"cell\"><input name=\"tryt\" id=\"tryt\" type=\"text\" value=\"""" + creds['trayType'] + """\" class=\"formtext\"/></td>
          </tr>
          <tr>
            <td class=\"cell\"><label for=\"tryn\" class=\"labeltext\">Tray Name</label></td>
            <td class=\"cell\"><input name=\"tryn\" id=\"tryn\" type=\"text\" value=\"""" + creds['trayName'] + """\" class=\"formtext\"/></td>
          </tr>
          <tr>
            <td class=\"cell\"><label for=\"cube\" class=\"labeltext\">Cube Type</label></td>
            <td class=\"cell\"><input name=\"cube\" id=\"cube\" type=\"text\" value=\"""" + creds['cubeType'] + """\" class=\"formtext\"/></td>
          </tr>
          <tr>
            <td></td>
            <td><input type=\"submit\" class=\"formtext\"/></td>
          </tr>
        </table>
      </form>
    </div>
  </body>
</html>
"""
  return html
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)   #creating socket object
s.bind(('', 80))
s.listen(5)
while True:
  conn, addr = s.accept()
  print('Got a connection from %s' % str(addr))
  request = conn.recv(1024)
  index = str(request).find("POST");
  if index >= 0:
      print('Content = %s' % str(request))
  
  response = web_page()
  conn.send(response)
  conn.close()