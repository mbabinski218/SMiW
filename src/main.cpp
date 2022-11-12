#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "time.h"
#include <driver/rtc_io.h>
#include "webPage.h"	
#include "SPIFFS.h"
#include <sstream>
#include "xml.h"

// wifi
const char *ssid = "esp32";
const char *password = "esp32!!!";
WebServer server(80);

// pinout
const int speaker = 25;
const int pir = 32;
const int button = 27;
const int led = 2;

// time
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;
String startTimeString = "";
String endTimeString = "";
tm* localTime = nullptr;
tm* startTime = nullptr;
tm* endTime = nullptr;

// global variables
int buttonLastState = HIGH;
int buttonCurrentState = HIGH;
bool isSleep = false;
bool disable = true;
bool shouldSave = true;

// generates a number between min and max
int randomFrequencyGenerator(int min, int max)
{
	return rand() % (max - min + 1) + min;
}

// controls the esp32 built-in led
void enableLed(bool value)
{
	value == true ? digitalWrite(led, HIGH) : digitalWrite(led, LOW);
}

// starts the modem sleep mode
void powerOff()
{
	disable = true;
	enableLed(false);
}

// method that will be executed after exiting the sleep mode
void powerOn()
{
	disable = false;
	isSleep = false;
	enableLed(true);
	Serial.println(esp_sleep_get_wakeup_cause()); // ext0 = 2, wifi = 9
}

// starts the sleep mode
void sleep()
{
	disable = true;
	isSleep = true;
	enableLed(false);
	rtc_gpio_pullup_en(GPIO_NUM_27);
	esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, 0);
	esp_sleep_enable_wifi_wakeup();

	if(startTime != nullptr)
		//esp_sleep_enable_timer_wakeup();

	esp_light_sleep_start();
}

// split string and set time
void ParseStringToTm(String &str, tm *t)
{
	char buf[64];
	str.toCharArray(buf, 64);
	
	const char* delim = ":";
	char* nextToken = NULL;
	char* token = strtok_r(buf, delim, &nextToken);
	char* s1 = token;
	t->tm_hour = atoi(s1);
	token = strtok_r(NULL, delim, &nextToken);
	char* s2 = token;
	t->tm_min = atoi(s2);

	delete[] nextToken, token, s1, s2;
}

// get current time
void getLocalTimeFromNtpServer() 
{
  	if(!getLocalTime(localTime))
  	{
    	Serial.println("Failed to obtain time");
  	}
}

void time()
{	
	getLocalTimeFromNtpServer();

	if(startTime != nullptr)
	{
		int start = localTime->tm_hour * 60 + localTime->tm_min - startTime->tm_hour * 60 - startTime->tm_min;
		if(disable && start == 0)
		{
			powerOn();
			return;
		}
	}

	if(endTime != nullptr)
	{
		int end = localTime->tm_hour * 60 + localTime->tm_min - endTime->tm_hour * 60 - endTime->tm_min;
		if(!disable && end == 0)
		{
			powerOff();
			return;
		}
	}
}

void handleOnConnect()
{
	Serial.println("ESP: user connected");
	server.send(200, "text/html", mainPage);
}

void handleInfo()
{
	xml::sendInfo(server, disable, isSleep, startTimeString, endTimeString);
}

void handleData()
{
	File file = SPIFFS.open("/data.txt", FILE_READ);
	String data = file.readString();
	xml::sendData(server, data);
	file.close();
}

void handelPower()
{
	if(disable)
	{
		Serial.println("ESP: turn on");
		powerOn();
	}
	else
	{
		Serial.println("ESP: turn off");
		powerOff();
	}
	xml::sendInfo(server, disable, isSleep, startTimeString, endTimeString);
}

void handleSleep()
{
	Serial.println("ESP: sleep on");
	server.send(200, "text/plain", "");
	powerOff();
	delay(5000);
	sleep();
	xml::sendInfo(server, disable, isSleep, startTimeString, endTimeString);
}

void handleStartTime()
{
	startTimeString = server.arg("VALUE");
	if(startTimeString == "")
	{
		startTime = nullptr;
		Serial.println("ESP: start time: set to null");
	}
	else
	{
		Serial.println("ESP: start time: " + startTimeString);
		startTime = new tm();
		ParseStringToTm(startTimeString, startTime);
	}
	xml::sendInfo(server, disable, isSleep, startTimeString, endTimeString);
}

void handleEndTime()
{
	endTimeString = server.arg("VALUE");
	if(endTimeString == "")
	{
		endTime = nullptr;
		Serial.println("ESP: start time: set to null");
	}
	else
	{
		Serial.println("ESP: end time: " + endTimeString);
		endTime = new tm();
		ParseStringToTm(endTimeString, endTime);
	}
	xml::sendInfo(server, disable, isSleep, startTimeString, endTimeString);
}

void checkWifi()
{
	while (WiFi.status() != WL_CONNECTED)
	{
		WiFi.reconnect();
		delay(5000);
		Serial.print(".");
	}
}

void checkFlash() 
{
	if(SPIFFS.usedBytes() >= SPIFFS.totalBytes() - 300000)
	{
		SPIFFS.remove("/data.txt");
		Serial.println("ESP: data cleared");
	}
}

void setup()
{
	// disable bluetooth
	btStop();

	// configure time
	localTime = new tm();
	configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

	// set serial monitor
	Serial.begin(9600);
	//Serial.begin(115200);

	// enable wifi
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);
	checkWifi();

	// checking flash
	while(!SPIFFS.begin())
      Serial.println("An Error has occurred while mounting SPIFFS");

	// setting up the server
	server.on("/", handleOnConnect);
	server.on("/power", handelPower);
	server.on("/sleep", handleSleep);
	server.on("/startTime", handleStartTime);
	server.on("/endTime", handleEndTime);
	server.on("/info", handleInfo);
	server.on("/data", handleData);

	server.begin();
	Serial.println("\nHTTP server started");
	Serial.println(WiFi.localIP());

	// setting up pins mode
	pinMode(speaker, OUTPUT);
	pinMode(pir, INPUT);
	pinMode(button, INPUT);
	pinMode(led, OUTPUT);

	SPIFFS.remove("/data.txt");
}

void loop()
{
	// wifi
	checkWifi();
	server.handleClient();

	// time
	time();

	// button
	buttonCurrentState = digitalRead(button);
	if (buttonCurrentState == LOW && buttonLastState == HIGH)
	{
		if (!isSleep)
		{
			Serial.println("ESP: sleep on");
			sleep();
		}
		else
		{
			Serial.println("ESP: sleep off");
			powerOn();
		}
	}
	buttonLastState = buttonCurrentState;

	// is pir and buzzer enable
	if (!disable)
	{
		// pir and buzzer
		int detected = digitalRead(pir);
		if (detected == HIGH)
		{			
			// tone
			tone(speaker, 4000);

			// check flash
			checkFlash();

			// save data
			if(shouldSave)
			{
				shouldSave = false;		
				String hour = localTime->tm_hour < 10 ? "0" + (String)(localTime->tm_hour) : (String)(localTime->tm_hour);
				String min = localTime->tm_min < 10 ? "0" + (String)(localTime->tm_min) : (String)(localTime->tm_min);
				String temp = hour + ":" + min;
				Serial.println("ESP: detected - " + temp);

				File file = SPIFFS.open("/data.txt", FILE_APPEND);
				if(file)
					file.println(temp);
				else
					Serial.println("There was an error opening the file");
				file.close();
			}
		}
		else
		{
			tone(speaker, 0);
			shouldSave = true;
		}
	}
}