# ESP8266Eink

This is the ESP8266 connectivity to my E-ink display. 

At the end the module will stay in low power and every 10 minutes will do:

1.Measure the battery level
2.Measure the temperature from DS18B20 
3.Publish the temperature to the thinkspeak account
4.Publish the ip address, SDK version, B/G/N connectivity, it type as JSON over MQTT
5.Publish the temperature as JSON over MQTT on the temperature topic 
6.Read the current time and show it on E-ink display 
7.Go to sleep for another 10 minutes.

The purpose of this is to have a mobile device that can be moved around the house to publish temperature to my chronothermostat (of course ESP8266 based).

During the heating time device will update itself every minute otherwise every 10 minutes.

The chronothermostat will have an PID controller that will receive temperature from my module.

More details on myblog myesp8266.blogspot.com

