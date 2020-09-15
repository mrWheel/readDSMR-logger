# readDSMR-logger

This program is a skeleton to be further enhanced for your purpose!

First define the type of board you are using by removeing the two slashes in front:
<code>
//======= define type of board ===========
//==== only define one (1) board type ====
// #define _IS_ARDUINO
// #define _IS_ESP8266
// #define _IS_ESP32
</code>

Define the IP address of your DSMR-logger:
<code>
#define _DSMR_IP_ADDRESS    "IP_ADDRESS_OF_YOUR_DSMR_LOGGER"
</code>

If your board has WiFi then enter the WiFi credentials:
<code>
#define _WIFI_SSID          "YOUR_WIFI_SSID"
#define _WIFI_PASSWRD       "YOUR_WIFI_PASSWRD"
</code>

Be aware that the program stretches the (memory) limits of the Arduino (UNI or Ethernet) board(s)!

For more information on the DSMR-logger 
<a href="https://willem.aandewiel.nl/index.php/2020/02/28/restapis-zijn-hip-nieuwe-firmware-voor-de-dsmr-logger/">see</a>
this post.

