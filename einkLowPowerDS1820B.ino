/*  
   Catalin Batrinu bcatalin@gmail.com 2016 Feb
    
    Read temperature from a DALLAS DS18B20 sensor and
    1. Publish it to thingspeak.com 
    2. Send it as JSON ( along with other info) over MQTT to my BROKER
    3. Display it on E-ink with info about time, VCC and IP address

    Then go to sleep for 10 minutes. Wake up again and start over.

    Connections:
    DS18B20 is connected to pin GPIO4
    E-ink Display is connected:
    BS1 - GND
    RES - VCC
    SDA - GPIO13
    SCL - GPIO14
CS1(SS) - GPIO15
    D/C - GPIO5
    
*/
#include <ESP8266WiFi.h>
#include <EinkESP8266.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>

ADC_MODE(ADC_VCC);

unsigned int localPort = 2390;   
IPAddress timeServerIP; 
const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48; 
byte packetBuffer[ NTP_PACKET_SIZE]; 
WiFiUDP udp;

#define UTC_OFFSET 2

#define wifi_ssid "XXXXXX"
#define wifi_password "XXXXXXXX"
#define mqtt_server "XXX.XXX.XXX.XXX"
#define mqtt_port "XXXX"
#define mqtt_user "XXXXXX"
#define mqtt_password "XXXXX"
#define dth_topic "/62/dth"
#define device_status_topic "/62/device/status"

#define DEBUG false
#define Serial if(DEBUG)Serial
#define DEBUG_OUTPUT Serial

// Time to sleep (in seconds):
const int sleepTimeS = 600;

String apiKey = "....Your API KEY....";
const char* server = "api.thingspeak.com";

WiFiClient espClient;
PubSubClient client(espClient);

#define ONE_WIRE_BUS 4  // DS18B20 pin
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
float temp;

E_ink_ESP8266 Eink_ESP8266;
#define SPI_FREQ 1e4
#define CS 15
#define DC 5
 

char dev_name[50];
char temperature[6];
char display_vcc[40];
char convertTempChar[50];
char json_buffer[512];
char json_buffer_status[512];

char my_ip_s[16];
char my_ip_display[40];

char show_time[9];
char show_time_display[40];

char hour_c[2];
char mnt_c[2];
char sec_c[2];
StaticJsonBuffer<512> jsonBuffer;
JsonObject& root = jsonBuffer.createObject(); 

StaticJsonBuffer<512> jsonDeviceStatus;
JsonObject& jsondeviceStatus = jsonDeviceStatus.createObject();

int wifi_reconn = 0;
int mqtt_reconn = 0;

char *ftoa(char *a, double f, int precision)
{
 long p[] = {0,10,100,1000,10000,100000,1000000,10000000,100000000};
 
 char *ret = a;
 long heiltal = (long)f;
 itoa(heiltal, a, 10);
 while (*a != '\0') a++;
 *a++ = '.';
 long desimal = abs((long)((f - heiltal) * p[precision]));
 itoa(desimal, a, 10);
 return ret;
}
  
void gettemperature() 
{
  int runs=0;
  do {
    DS18B20.requestTemperatures(); 
    temp = DS18B20.getTempCByIndex(0);
    Serial.print("Temperature: ");
    Serial.println(temp);
  } while (temp == 85.0 || temp == (-127.0));
}



void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    wifi_reconn++;

    if(wifi_reconn == 30)
      ESP.deepSleep(sleepTimeS * 1000000);//10minutes
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client")) {
    if (client.connect(dev_name, mqtt_user, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());

      mqtt_reconn++;

     if(mqtt_reconn == 6)
       ESP.deepSleep(sleepTimeS * 1000000);//10minutes
      
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}
    
void setup()
{
  Serial.begin(115200);
  double vcc = ESP.getVcc();
  Serial.println(vcc);
  Serial.println(vcc/1000);
  
  sprintf(dev_name, "ESP_%d", ESP.getChipId());
  pinMode(DC,OUTPUT);
  pinMode(CS, OUTPUT);
  SPI.begin();
  SPI.setHwCs(true);

  Eink_ESP8266.clearScreen();// clear the screen
   Eink_ESP8266.E_Ink_P8x16Str(14,8,"  Wellcome! I am ");
   Eink_ESP8266.E_Ink_P8x16Str(8,8,"  ESP8266 with E-ink");
   Eink_ESP8266.E_Ink_P8x16Str(4,8,"by bcatalin@gmail.com");
   Eink_ESP8266.E_Ink_P8x16Str(0,8,"Wait! Connecting.....");
  Eink_ESP8266.refreshScreen(); 

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.connect(dev_name, mqtt_user, mqtt_password);
  
 if (!client.connected()) 
  {
    reconnect();
  }
  client.loop();

  
  IPAddress my_ip_addr = WiFi.localIP();
  sprintf(my_ip_s, "%d.%d.%d.%d", my_ip_addr[0],my_ip_addr[1],my_ip_addr[2],my_ip_addr[3]);
  jsondeviceStatus ["device_name"] = dev_name;
  jsondeviceStatus["type"] = "dth"; 
  jsondeviceStatus["ipaddress"] = String(my_ip_s).c_str();
  jsondeviceStatus["bgn"] = 3;
  jsondeviceStatus["sdk"] = ESP.getSdkVersion();//"1.4.0";
  jsondeviceStatus["version"] = "1";
  jsondeviceStatus["uptime"] = 0;
  
  jsondeviceStatus.printTo(json_buffer_status, sizeof(json_buffer_status));  
  client.publish(device_status_topic, json_buffer_status , false);
  Serial.println(json_buffer_status);



  gettemperature();

  root["device_name"] = dev_name;
  root["type"] = "dth";  
  root["temperature"] = String(temp).c_str();
   
  root.printTo(json_buffer, sizeof(json_buffer));  
  client.publish(dth_topic, json_buffer , false);
  Serial.println(json_buffer);

//publish
  if (espClient.connect(server,80)) {  //   "184.106.153.149" or api.thingspeak.com
    String postStr = apiKey;
           postStr +="&field1=";
           postStr += String(temp);
           //postStr +="&field2=";
           //postStr += String(0);
           //postStr +="&field3=";
           //postStr += String(0);//String(bmp.pressureToAltitude(SENSORS_PRESSURE_SEALEVELHPA, event.pressure)); 
           postStr +="&field4=";
           postStr += String(vcc);                     
           postStr += "\r\n\r\n";
            
     espClient.print("POST /update HTTP/1.1\n");
     espClient.print("Host: api.thingspeak.com\n");
     espClient.print("Connection: close\n");
     espClient.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
     espClient.print("Content-Type: application/x-www-form-urlencoded\n");
     espClient.print("Content-Length: ");
     espClient.print(postStr.length());
     espClient.print("\n\n");
     espClient.print(postStr);
     espClient.stop();
  }

  
  udp.begin(localPort);
  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP); 

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);
  
  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("no packet yet");
  }
  else {
    Serial.print("packet received, length=");
    Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;

    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;

    int hour_i = ((epoch  % 86400L) / 3600) + UTC_OFFSET;

    if (hour_i >= 24)
      hour_i = hour_i - 24;

    Serial.println(hour_i);
    
    sprintf(hour_c,"%s:", String(hour_i).c_str() );
    
    if ( ((epoch % 3600) / 60) < 10 ) 
      sprintf(mnt_c,"0%s:",String((epoch % 3600) / 60).c_str() );
    else
      sprintf(mnt_c,"%s:",String((epoch % 3600) / 60).c_str() );


    if ( (epoch % 60) < 10 ) 
      sprintf(sec_c, "0%s",String(epoch % 60).c_str() );
    else
      sprintf(sec_c, "%s",String(epoch % 60).c_str() );

    sprintf(show_time,"%s%s%s",hour_c, mnt_c, sec_c );
    Serial.println(show_time);
  }


   
   //show
   Eink_ESP8266.clearScreen();// clear the screen

    //Temperature
   sprintf(temperature,"Temperature: %s C",ftoa(convertTempChar,temp,2));
   Eink_ESP8266.E_Ink_P8x16Str(14,8, temperature);  

   //Battery
   sprintf(display_vcc,"Battery:      %s V", ftoa(convertTempChar,vcc/1000,2));
   Eink_ESP8266.E_Ink_P8x16Str(9,8,  display_vcc);
   
   //My IP   
   sprintf(my_ip_display,"My IP:   %s", String(my_ip_s).c_str());
   Eink_ESP8266.E_Ink_P8x16Str(4,8,  my_ip_display);
 
   sprintf(show_time_display,"Time:        %s",show_time);
   Eink_ESP8266.E_Ink_P8x16Str(0,8,  show_time_display);
    
   Eink_ESP8266.refreshScreen();

   ESP.deepSleep(sleepTimeS * 1000000);//10minutes
}


void loop()
{
//Nothing to do here....
}
