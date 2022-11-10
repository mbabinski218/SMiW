#include <WebServer.h>

class xml 
{
public:
    static void sendInfo(WebServer &server, bool &disable, bool &sleep, String &startTime, String &endTime);
    static void sendData(WebServer &server, String &data);
};

//////////IMPLEMENTATIONS//////////

void xml::sendInfo(WebServer &server, bool &disable, bool &sleep, String &startTime, String &endTime)
{
    char XML[2048];
    char temp[32];

    // header
    strcpy(XML, "<?xml version = '1.0'?>\n<Info>\n");

    // sleep
    if (sleep) 
        strcat(XML, "<Sleep>1</Sleep>\n");
    else 
        strcat(XML, "<Sleep>0</Sleep>\n");

    // mode
    if (disable) 
        strcat(XML, "<Mode>0</Mode>\n");
    else 
        strcat(XML, "<Mode>1</Mode>\n");

    // start time
    sprintf(temp, "<StartTime>%s</StartTime>\n", startTime);
    strcat(XML, temp);

    // end time
    sprintf(temp, "<EndTime>%s</EndTime>\n", endTime);
    strcat(XML, temp);

    // end 
    strcat(XML, "</Info>\n");

    // send 
    server.send(200, "text/xml", XML);
    Serial.println("ESP: send info:\n" + (String)XML);
}

void xml::sendData(WebServer &server, String &data)
{
    char XML[2048];
    char temp[32];

    // header
    strcpy(XML, "<?xml version = '1.0'?>\n<Data>\n");

    // data
    sprintf(temp, "%s\n", data);
    strcat(XML, temp);

    // end 
    strcat(XML, "</Data>\n");

    // send 
    server.send(200, "text/xml", XML);
    Serial.println("ESP: send data:\n" + (String)XML);
}