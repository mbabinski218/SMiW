#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "time.h"
#include <driver/rtc_io.h>
#include "webPage.h"
#include <sstream>
#include <string>

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
tm* localTime = nullptr;
tm* startTime = nullptr;
tm* endTime = nullptr;

// global variables
int buttonLastState = HIGH;
int buttonCurrentState = HIGH;
bool isSleep = false;
bool disable = true;
bool calculating = false;

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
	isSleep = true;
	enableLed(false);
	// setCpuFrequencyMhz(80);
}

// starts the deep sleep mode
void deepSleep()
{
	disable = true;
	isSleep = true;
	enableLed(false);
	rtc_gpio_pulldown_en(GPIO_NUM_27);
	esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, 0);
	esp_light_sleep_start();
}

// method that will be executed after exiting the sleep mode
void afterSleep()
{
	disable = false;
	isSleep = false;
	enableLed(true);
	// setCpuFrequencyMhz(240);
	Serial.println(esp_sleep_get_wakeup_cause()); // ext0 = 2, wifi = 9
}

// split string and set time
void SetTime(String &str, tm *t)
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
}

// get current time
void getLocalTimeFromNtpServer() 
{
  	if(!getLocalTime(localTime))
  	{
    	Serial.println("Failed to obtain time");
  	}
}

void Time()
{	
	getLocalTimeFromNtpServer();

	if(startTime != nullptr)
	{
		int start = localTime->tm_hour * 60 + localTime->tm_min - startTime->tm_hour * 60 - startTime->tm_min;
		if(disable && start == 0)
		{
			afterSleep();
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

// void handleGetData()
// {
// 	server.send(200, "xml")
// }

void handelPower()
{
	if(disable)
	{
		Serial.println("ESP: turn on");
		afterSleep();
	}
	else
	{
		Serial.println("ESP: turn off");
		powerOff();
	}
}

void handleSleep()
{
	Serial.println("ESP: deep sleep on");
	server.send(200, "text/plain", "");
	powerOff();
	deepSleep();
}

void handleStartTime()
{
	String value = server.arg("VALUE");
	Serial.println("ESP: start time: " + value);
	startTime = new tm();
	SetTime(value, startTime);
}

void handleEndTime()
{
	String value = server.arg("VALUE");
	Serial.println("ESP: end time: " + value);
	endTime = new tm();
	SetTime(value, endTime);
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

	// enable wifi
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);

	// waiting for connection
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(1000);
		Serial.print(".");
	}

	// setting up the server
	server.on("/", handleOnConnect);
	server.on("/power", handelPower);
	server.on("/sleep", handleSleep);
	server.on("/startTime", handleStartTime);
	server.on("/endTime", handleEndTime);

	server.begin();
	Serial.println("\nHTTP server started");
	Serial.println(WiFi.localIP());

	// setting up pins mode
	pinMode(speaker, OUTPUT);
	pinMode(pir, INPUT);
	pinMode(button, INPUT);
	pinMode(led, OUTPUT);
}

void loop()
{
	Time();

	// wifi client
	server.handleClient();

	// button
	buttonCurrentState = digitalRead(button);
	if (buttonCurrentState == LOW && buttonLastState == HIGH)
	{
		if (!isSleep)
		{
			Serial.println("ESP: deep sleep on");
			deepSleep();
		}
		else
		{
			Serial.println("ESP: deep sleep off");
			afterSleep();
		}
	}
	buttonLastState = buttonCurrentState;

	// is pir and buzzer enable
	if (!disable)
	{
		// pir and buzzer
		int detected = digitalRead(pir);
		Serial.println(detected);
		if (detected == HIGH)
		{
			tone(speaker, 4000);
		}
		else
		{
			tone(speaker, 0);
		}
	}
}