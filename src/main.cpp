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
const int speaker = 33;
const int pir = 32;
const int button = 25;
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
bool shouldSave = true;
bool programStart = true;
bool disable = true;
RTC_DATA_ATTR bool shouldSleep = false;

// controls the esp built-in led
void enableLed(bool value)
{
	value == true ? digitalWrite(led, HIGH) : digitalWrite(led, LOW);
}

// starts the sleep mode
void deepSleep()
{
	disable = true;
	enableLed(false);
	Serial.println("ESP: power off with deep sleep");
	xml::sendInfo(server, disable, shouldSleep, startTimeString, endTimeString);

	File file = SPIFFS.open("/times.txt", FILE_WRITE);
	if(file)
	{
		if(!file.println(startTimeString + "\n" + endTimeString))
			Serial.println("ESP: file write failed");
	}
	else
		Serial.println("ESP: there was an error opening the file");
	file.close();

	if(startTime != nullptr)
	{
		long long temp = (startTime->tm_hour * 60 + startTime->tm_min - localTime->tm_hour * 60 - localTime->tm_min) * 60000000;
		
		if(temp < 0)
			temp += 24ll * 60ll * 60000000ll;
		
		Serial.println("Sleep for: " + (String)temp);
		esp_sleep_enable_timer_wakeup(temp);
	}

	delay(5000);
	esp_deep_sleep_start();
}

// starts the modem sleep mode
void powerOff()
{
	disable = true;
	enableLed(false);
	Serial.println("ESP: power off");
	xml::sendInfo(server, disable, shouldSleep, startTimeString, endTimeString);
}

// method that will be executed after exiting the sleep mode
void powerOn()
{
	disable = false;
	enableLed(true);
	Serial.println("ESP: power on");
	xml::sendInfo(server, disable, shouldSleep, startTimeString, endTimeString);
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
    	Serial.println("Failed to obtain time");
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
			if(shouldSleep)
				deepSleep();					
			else
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
	xml::sendInfo(server, disable, shouldSleep, startTimeString, endTimeString);
}

void handleData()
{
	Serial.println("ESP: send data");
	File file = SPIFFS.open("/montion.txt", FILE_READ);
	String data = "";

	if(file)
		while(file.available()) 
        	data = file.readString();
	else
		Serial.println("ESP: there was an error opening the file");

	file.close();
	xml::sendData(server, data);
}

void handelPower()
{
	if(disable)
		powerOn();
	else
		powerOff();
}

void handleSleep()
{
	if(!shouldSleep)
	{
		Serial.println("ESP: sleep mode on");
		shouldSleep = true;
	}
	else
	{
		Serial.println("ESP: sleep mode off");
		shouldSleep = false;
	}
	xml::sendInfo(server, disable, shouldSleep, startTimeString, endTimeString);
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
	xml::sendInfo(server, disable, shouldSleep, startTimeString, endTimeString);
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
	xml::sendInfo(server, disable, shouldSleep, startTimeString, endTimeString);
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
		SPIFFS.remove("/montion.txt");
		Serial.println("ESP: data cleared");
	}
}

void setup()
{
	// set serial monitor
	Serial.begin(9600);

	// disable bluetooth
	btStop();

	// configure time
	localTime = new tm();
	configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

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

	// checks whether the first start or exit from deep sleep mode
	if(esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER)
	{
		Serial.println("ESP: wake up");

		File file = SPIFFS.open("/times.txt", FILE_READ);
		String data = "";

		while(file.available()) 
			data = file.readString();

		
		int temp = data.indexOf("\n");
		startTimeString = data.substring(0, temp);
		endTimeString = data.substring(temp + 1, data.length() - 2);

		if(startTimeString != "not set")
		{
			startTime = new tm();
			ParseStringToTm(startTimeString, startTime);
		}

		if(startTimeString != "not set")
		{
			endTime = new tm();
			ParseStringToTm(endTimeString, endTime);
		}

		file.close();		
		powerOn();
	}
	else
		Serial.println("ESP: start");
}

void loop()
{
	// wifi
	checkWifi();
	server.handleClient();

	// handle time
	time();

	// handle button click
	buttonCurrentState = digitalRead(button);
	if (buttonCurrentState == LOW && buttonLastState == HIGH && !programStart)
	{
		if (!disable)		
			powerOff();
		else
			powerOn();
	}
	buttonLastState = buttonCurrentState;

	// is pir and buzzer enable
	if (!disable)
	{
		// check if motion has been detected
		int detected = digitalRead(pir);
		if (detected == HIGH)
		{			
			// tone
			digitalWrite(speaker, HIGH);

			// check flash
			checkFlash();

			// save data
			if(shouldSave)
			{
				shouldSave = false;		
				String hour = localTime->tm_hour < 10 ? "0" + (String)(localTime->tm_hour) : (String)(localTime->tm_hour);
				String min = localTime->tm_min < 10 ? "0" + (String)(localTime->tm_min) : (String)(localTime->tm_min);
				String temp = (String)localTime->tm_mday + "." + (String)(localTime->tm_mon + 1) + "." + (String)(localTime->tm_year + 1900) + " " + hour + ":" + min;
				Serial.println("ESP: detected - " + temp);

				File file = SPIFFS.open("/montion.txt", FILE_APPEND);
				if(file)
					if(!file.println(temp))
						Serial.println("ESP: file append failed");

				file.close();
			}
		}
		else
		{
			// disable tone
			digitalWrite(speaker, LOW);
			shouldSave = true;
		}

	}

	programStart = false;
}