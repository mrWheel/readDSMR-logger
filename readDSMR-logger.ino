/*
***************************************************************************  
**  Program  : readDSMR-logger (DSMRloggerAPI)
**
**  This program call's the DSMR-logger "actual" restAPI
**  and presents the value of the fields.
*/
#define _FW_VERSION "v2.0.1 (15-09-2020)"
/*
**  Copyright (c) 2020 Willem Aandewiel
**
**  TERMS OF USE: MIT License. See bottom of file.                                                            
***************************************************************************      
*/

//======= define type of board ===========
//==== only define one (1) board type ====
// #define _IS_ARDUINO_UNO  
#define _IS_ARDUINO_MEGA
// #define _IS_ESP8266 
// #define _IS_ESP32     
//========================================
//======= leave the rest unchanged =======

#include "setup.h"

#if defined( _IS_ARDUINO_UNO ) || defined( _IS_ARDUINO_MEGA )
  #include <ArduinoHttpClient.h>    // tested with version 0.4.0
  #include <Ethernet.h>
  #include <SPI.h>
#endif

#ifdef _IS_ESP8266
  #include <ESP8266WiFi.h>

  const char *ssid      = _WIFI_SSID;
  const char *password  = _WIFI_PASSWRD;
#endif

#ifdef _IS_ESP32
  #include <WiFi.h>
  #include <HTTPClient.h>

  const char *ssid      = _WIFI_SSID;
  const char *password  = _WIFI_PASSWRD;
#endif

#include <Arduino_JSON.h>   // install with Library Manager

#define _READINTERVAL 60000

const char *DSMRprotocol  = "http://";
const char *DSMRserverIP  = _DSMR_IP_ADDRESS;
const char *DSMRrestAPI   = "/api/v1/sm/actual";
String      payload;
int         httpResponseCode;
uint32_t    lastRead = 0;

#if defined( _IS_ARDUINO_UNO ) || defined( _IS_ARDUINO_MEGA )
//--------------------------------------------------------------------------
// Include in the main program:
//    #include <ArduinoHttpClient.h>    // version 0.4.0
//    #include <Ethernet.h>
//    #include <SPI.h>
//
// and in Setup() do something like:
//    // Initialize Ethernet library
//    byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
//
//    if (!Ethernet.begin(mac)) {
//      Serial.println("Failed to configure Ethernet");
//      return;
//    }
//    delay(1000);
//
//--------------------------------------------------------------------------
bool dsmrGETrequest() 
{
  EthernetClient ETHclient;
  HttpClient DSMRclient = HttpClient(ETHclient, DSMRserverIP, 80);

  payload = "{}"; 
   
  Serial.println(F("making GET request"));
  DSMRclient.get(DSMRrestAPI);

  // read the response code and body of the response
  httpResponseCode = DSMRclient.responseStatusCode();
  Serial.print(F("http Response Code: "));
  Serial.println(httpResponseCode);

  if (httpResponseCode <= 0)
  {
    return false;
  }

  payload    = DSMRclient.responseBody();
#ifndef _IS_ARDUINO_UNO
  Serial.print("Response: ");
  Serial.println(payload);
#endif
  
  // Free resources
  DSMRclient.stop();

  return true;
  
} // dsmrGETrequest()
#endif

#ifdef _IS_ESP8266
//--------------------------------------------------------------------------
// Include in the main program:
//    #include <ESP8266WiFi.h>
//--------------------------------------------------------------------------
bool dsmrGETrequest() 
{
  WiFiClient  DSMRclient;

  payload = ""; 

  Serial.print("DSMRclient.connect("); Serial.print(DSMRserverIP);
  Serial.println(", 80)");
  if (!DSMRclient.connect(DSMRserverIP, 80))
  {
    Serial.println(F("error connecting to DSMRlogger "));
    return false;
  }

  //-- normal operation 
  DSMRclient.print(F("GET "));
  DSMRclient.print(DSMRrestAPI);
  DSMRclient.println(" HTTP/1.1");
  DSMRclient.print(F("Host: "));
  DSMRclient.println(DSMRserverIP);
  DSMRclient.println(F("Connection: close"));
  DSMRclient.println();

  DSMRclient.setTimeout(2000);

  //-debug-Serial.println("find(HTTP/1.1)..");
  DSMRclient.find("HTTP/1.1");  // skip everything up-until "HTTP/1.1"
  //-debug-Serial.print("DSMRclient.parseInt() ==> ");
  httpResponseCode = DSMRclient.parseInt(); // parse status code
  
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);
  
  if (httpResponseCode <= 0) 
  {
    payload = "{\"actual\":[{\"name\":\"httpresponse\", \"value\": "+String(httpResponseCode)+"}]}";
    return false;
  }

  // Skip HTTP headers
  if (!DSMRclient.find("\r\n\r\n")) 
  {
    Serial.println(F("Invalid response"));
    payload = "{\"actual\":[{\"name\":\"httpresponse\", \"value\": "+String(httpResponseCode)+"}]}";
    return false;
  }

  while(DSMRclient.connected()) 
  {
    if (DSMRclient.available())
    {
      // read an incoming lines from the server:
      String line = DSMRclient.readStringUntil('\r');
      line.replace("\n", "");
      if (   (line[0] == '{') || (line[0] == ',') 
          || (line[0] == '[') || (line[0] == ']') )
      {
        //-debug-Serial.print(line);
        payload += line;
      }
    }
  }
  //-debug-Serial.println();
  
  // Free resources
  DSMRclient.stop();

  return true;
  
} // dsmrGETrequest()
#endif

#ifdef _IS_ESP32
//--------------------------------------------------------------------------
// Include in the main program:
//    #include <WiFi.h>
//    #include <HTTPClient.h>
//--------------------------------------------------------------------------
bool dsmrGETrequest() 
{
  HTTPClient DSMRclient;
    
  // Your IP address with path or Domain name with URL path 
  DSMRclient.begin(String(DSMRprotocol) + String(DSMRserverIP)+String(DSMRrestAPI));
  
  // Send HTTP GET request
  httpResponseCode = DSMRclient.GET();

  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);
  
  payload = "{}"; 
  
  if (httpResponseCode > 0) 
  {
    payload = DSMRclient.getString();
  }
  else 
  {
    payload = "{\"actual\":[{\"name\":\"httpresponse\", \"value\": "+String(httpResponseCode)+"}]}";
    // Free resources
    DSMRclient.end();
    return false;
  }

  // Free resources
  DSMRclient.end();
  
  return true;
  
} // dsmrGETrequest()
#endif

//--------------------------------------------------------------------------
String splitJsonArray(String data, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++)
  {
    if((data.charAt(i) == '}' && data.charAt(i+1) == ',') || i==maxIndex)
    {
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
        if (data.charAt(i+1) == ',') data[i+1] = ' ';
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]+1) : "";
  
} // splitJsonArray()


//--------------------------------------------------------------------------
String removeQuotes(JSONVar in)
{
  String in2 = JSON.stringify(in);
  in2.replace("\"", "");
  return in2;
    
} // removeQuotes()


//--------------------------------------------------------------------------
void readDsmrLogger()
{
  dsmrGETrequest();

  JSONVar myObject = JSON.parse(payload);
  
  // JSON.typeof(jsonVar) can be used to get the type of the var
  if (JSON.typeof(myObject) == "undefined") 
  {
    Serial.println(F("Parsing failed!"));
    return;
  }
  // This is how the "actual" JSON object looks like:
  //   {"actual":[
  //       {"name":"timestamp","value":"200911140716S"}
  //      ,{"name":"energy_delivered_tariff1","value":3433.297,"unit":"kWh"}
  //      ,{"name":"energy_delivered_tariff2","value":4453.041,"unit":"kWh"}
  //      ,{"name":"energy_returned_tariff1","value":678.953,"unit":"kWh"}
  //          ...
  //      ,{"name":"power_delivered_l2","value":0.071,"unit":"kW"}
  //      ,{"name":"power_delivered_l3","value":0,"unit":"kW"}
  //      ,{"name":"power_returned_l1","value":0,"unit":"kW"}
  //      ,{"name":"power_returned_l2","value":0,"unit":"kW"}
  //      ,{"name":"power_returned_l3","value":0.722,"unit":"kW"}
  //      ,{"name":"gas_delivered","value":2915.08,"unit":"m3"}
  //    ]}
  String topLevelData = JSON.stringify(myObject["actual"]);
  topLevelData.replace("[", "");
  topLevelData.replace("]", "");

#ifdef _IS_ARDUINO_UNO
  Serial.println(F("== Parsed Data =="));
#else
  Serial.println(F("== Parsed Data ===================================="));
#endif

  bool doParsing = true;
  int  fieldNr   = 0;
  while(doParsing)
  {
    yield();
    String field = splitJsonArray(topLevelData, fieldNr);
    fieldNr++;
    if (field.length() > 0)
    {
      //-debug-Serial.println(field);
      JSONVar nextObject = JSON.parse(field);
      JSONVar dataArray = nextObject.keys();
      for (int i = 0; i < dataArray.length(); i++) 
      {
        JSONVar value = nextObject[dataArray[i]];
        if (removeQuotes(dataArray[i]) == "name")
        {
          Serial.print(removeQuotes(value));
          Serial.print(F(" = "));
        }
        if (removeQuotes(dataArray[i]) == "value")
        {
          Serial.print(removeQuotes(value));
        }
        if (removeQuotes(dataArray[i]) == "unit")
        {
          Serial.print(F(" ("));
          Serial.print(removeQuotes(value));
          Serial.print(F(")"));
        }
        if (i == dataArray.length() -1)  Serial.println();
      }
    }
    else  // zero length, end of string
    {
      doParsing = false;
    }
  } // loop over all data

#ifdef _IS_ARDUINO_UNO
  Serial.println(F("================="));
#else
  Serial.println(F("=================================================="));
  Serial.print(F("Parsed [")); Serial.print(fieldNr-1); Serial.println(F("] fields"));
#endif
      
} // readDsmrLogger()


//--------------------------------------------------------------------------
void setup() 
{
  Serial.begin(115200);
  while(!Serial) { /* wait a bit */ }

#ifndef _IS_ARDUINO_UNO 
  Serial.println(F("And then it all begins ..."));
#endif

#ifdef _IS_ARDUINO_UNO
  // Initialize Ethernet library
  byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

  if (!Ethernet.begin(mac)) 
  {
    Serial.println(F("Failed to configure Ethernet"));
    return;
  }
  delay(1000);
#endif

#if defined(_IS_ESP8266) || defined(_IS_ESP32)
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
#endif

  lastRead = millis() + _READINTERVAL;

#ifndef _IS_ARDUINO_UNO 
  Serial.println(F("\r\nStart reading ..."));
#endif

} // setup()


//--------------------------------------------------------------------------
void loop() 
{
  if ((millis() - lastRead) > _READINTERVAL)
  {
    lastRead = millis();
    #ifndef _IS_ARDUINO_UNO 
      Serial.println(F("\r\nread API from DSMR-logger..."));
    #endif
    readDsmrLogger();
  }
  
} // loop()

/*
****************************************************************************
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to permit
* persons to whom the Software is furnished to do so, subject to the
* following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
* OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
* THE USE OR OTHER DEALINGS IN THE SOFTWARE.
* 
****************************************************************************
*/
