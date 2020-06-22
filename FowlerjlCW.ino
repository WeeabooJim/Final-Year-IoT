#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <rgb_lcd.h>
#include <SimpleDHT.h> 
#include <ESP8266mDNS.h>
#include <ESP8266WebServerSecure.h>
//#include <WiFiUdp.h>

/***************************************/ // Hardware Defines

#define LEDPIN 15  //LED PIN
#define Button 15  //Switch PIN

#define ButtonPollC 5
#define NetworkTransferC 10000
#define LCDUpdateC 1000
#define MinuteAverageC 60000
#define DeviceDiscoveryC 600000

#define SOCKET 8888

#define BUFFERLEN 255
char inBUFFER[BUFFERLEN];
char outBUFFER[BUFFERLEN];

SimpleDHT22 dht22;

/***************************************/ // Function Prototypes

void MQTTconnect ( void );
void respond ( void );
void servepage ( void );
void DeviceDiscovery ( void );
String IpAddress2String(const IPAddress& ipAddress);

/***************************************/ // Network Defines

#define ssid          "VM6870374"    // your hotspot SSID or name
#define password      "gnr4kbttBrDd"    // your hotspot password

/***************************************/ // Adafruit IO Defines

#define NOPUBLISH      // comment this out once publishing at less than 10 second intervals

#define ADASERVER     "io.adafruit.com"     // do not change this
#define ADAPORT       1883                  // do not change this 
#define ADAUSERNAME   "Fowlerjl"               // ADD YOUR username here between the qoutation marks
#define ADAKEY        "aio_QdQs57bOYsy8f9bcYjXtRf0fnwMj" // ADD YOUR Adafruit key here betwwen marks

/***************************************/ // Global instances / variables

BearSSL::ESP8266WebServerSecure SERVER(443);
WiFiClient client;    // create a class instance for the MQTT server
rgb_lcd LCD;          // create a class instance of the rgb_lcd class

// create an instance of the Adafruit MQTT class. This requires the client, server, portm username and
// the Adafruit key
Adafruit_MQTT_Client MQTT(&client, ADASERVER, ADAPORT, ADAUSERNAME, ADAKEY);

//WiFiUDP UDP;

int red = 0, green = 128, blue = 128;             // variables for the RGB backlight colour

boolean current;
boolean last = false;

int current_millis;
int timespressed = 0;

int ButtonPollLC = 0;
int NetworkTransferLC = 0;
int LCDUpdateLC = 0;
int MinuteAverageLC = 0;
int DeviceDiscoveryLC = 0;
int PerMinute = 0;
int LastMinute = 0;

int sensoridentifier = 10;

const char domainname[] = "board10";

static const char cert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIC8jCCAlugAwIBAgIUYOIOrtubcFsmeN3TrSfpvaJoMHAwDQYJKoZIhvcNAQEL
BQAwfjELMAkGA1UEBhMCVUsxFjAUBgNVBAgMDVdFU1QgTUlETEFORFMxEzARBgNV
BAcMCkJJUk1JTkdIQU0xGjAYBgNVBAoMEUFzdG9uIFVuaXZlcnNpdHkgMQ0wCwYD
VQQLDARFRVBFMRcwFQYDVQQDDA51c2VybmFtZS5sb2NhbDAeFw0yMDAyMTUxOTQ5
MzZaFw00NzA3MDMxOTQ5MzZaMH4xCzAJBgNVBAYTAlVLMRYwFAYDVQQIDA1XRVNU
IE1JRExBTkRTMRMwEQYDVQQHDApCSVJNSU5HSEFNMRowGAYDVQQKDBFBc3RvbiBV
bml2ZXJzaXR5IDENMAsGA1UECwwERUVQRTEXMBUGA1UEAwwOdXNlcm5hbWUubG9j
YWwwgZ8wDQYJKoZIhvcNAQEBBQADgY0AMIGJAoGBAKZPMudaME7JDhWSUhH+ZfOQ
dRZ6Pa4I1Xt0RJCb9F9PUX4vU7cyVWufoYpBlyOxzPyn1kMAzBmEQ0vAwXG0m7PU
urft8ZB/ZZallIfscgEI8HDcOVdwt/uEbfsONp7R4BAGKGhijx+niMgWLL3aoIF1
Tj7AqzjU85e/v2kPrdR5AgMBAAGjbTBrMB0GA1UdDgQWBBTAz79RjP3Mbs4Y32Ga
9p44Vycp+zAfBgNVHSMEGDAWgBTAz79RjP3Mbs4Y32Ga9p44Vycp+zAPBgNVHRMB
Af8EBTADAQH/MBgGA1UdEQQRMA+CDXVzZXJuYW1lLmxvY2EwDQYJKoZIhvcNAQEL
BQADgYEABmkelyDzfZRcqVGM8edRJM5dAQvBlHnjnsMPhvhgkG6MUisBa+vtVhKC
cOE4gAFvv8/hCaWIQwtFdWNfyle5meRX/Tj4L9N1aHASyPfTBcqKTGpjq6GiqtmL
rTINar7hi15E+adMVRRg/1VNfnBXKxh6IYch5hgeHj5TRgGwgAs=
-----END CERTIFICATE-----
)EOF";

static const char key[] PROGMEM =  R"EOF(
-----BEGIN PRIVATE KEY-----
MIICdwIBADANBgkqhkiG9w0BAQEFAASCAmEwggJdAgEAAoGBAKZPMudaME7JDhWS
UhH+ZfOQdRZ6Pa4I1Xt0RJCb9F9PUX4vU7cyVWufoYpBlyOxzPyn1kMAzBmEQ0vA
wXG0m7PUurft8ZB/ZZallIfscgEI8HDcOVdwt/uEbfsONp7R4BAGKGhijx+niMgW
LL3aoIF1Tj7AqzjU85e/v2kPrdR5AgMBAAECgYBgNFpW+JYPTUDne6AcJpSlY8BH
w2jgvt13r9dl68FeTQzwOMJtrCE7w7j3uF+M13KkCRbp5ZErhZZEQPnmI7sZSkal
3TIWmLQq9g9mIscdIjc6rsfWJ7DAdpVgdsClS3sHPxv3D3RTy6Z40vzPgb2977/y
26pUHHJVEG27dqthbQJBANJPu7fm8EOUcvg1/wwnp3p+dCHEmZ7snQlUerMGbw3+
fSJTqFhrwEXYhfzj2VQUqZyaDBM+tZ9vUQU+BreM+fcCQQDKcFUVeUh6PzJJ1uSJ
F3gabOc1IoIPyotQ9RWAU70vaj/aAuIXJd6PsUt+0DucYfcg1G5YteUqJGl/5Mpv
CokPAkADNLHw2LVa4l1qSTBtGAGmjVzp0txgnsy6Aq6oIfX5aaKwrkPHrUTOC8Hn
G/YJIROAzpxWgsMz/fdnNA3YKG77AkEAiA7NsJwOMVNuKiCLAvTKHQCauKSTw6c+
0U+XfuNJIKgJeC495I7oMa1Yb0fm+KkDHoaID4lZF2TXn0SXJeBv0wJBAKhRejgb
R/KkoohDnWQCNkI9TOMPtTohGjoqMakVcaFHB+SOYhKi7iI0+coxNv/1dMydaE5X
xkjlush/Q6zlliA=
-----END PRIVATE KEY-----
)EOF";

/***************************************/ // Feeds

Adafruit_MQTT_Subscribe ResetFootfall = Adafruit_MQTT_Subscribe(&MQTT, ADAUSERNAME "/feeds/resetfootfall");
Adafruit_MQTT_Publish Footfall = Adafruit_MQTT_Publish(&MQTT, ADAUSERNAME "/feeds/footfall");
Adafruit_MQTT_Publish AverageFootfall = Adafruit_MQTT_Publish(&MQTT, ADAUSERNAME "/feeds/averagefootfall");
Adafruit_MQTT_Publish SensorIdentifier = Adafruit_MQTT_Publish(&MQTT, ADAUSERNAME "/feeds/sensoridentifier");

/*********************************************************************************/

void setup() {
  // put your setup code here, to run once:
  pinMode(LEDPIN, OUTPUT);

  Serial.begin(115200);                         // open a serial port at 115,200 baud
  while(!Serial)                                // wait for serial peripheral to initialise
  {
    ;
  }
  delay(10);                                    // additional delay before attempting to use the serial peripheral
  Serial.print("Attempting to connect to ");    // Inform of us connecting
  Serial.print(ssid);                           // print the ssid over serial
  
  WiFi.begin(ssid, password);                   // attemp to connect to the access point SSID with the password
  
  while (WiFi.status() != WL_CONNECTED)         // whilst we are not connected
  {
    digitalWrite(LEDPIN, HIGH);
    delay(500);                                 // wait for 0.5 seconds (500ms)
    Serial.print(".");                          // print a .
  }
  Serial.print("\n");                           // print a new line
  digitalWrite(LEDPIN, LOW);
  
  Serial.println("Succesfully connected");      // let us now that we have now connected to the access point
  Serial.print("Mac address: ");                // print the MAC address
  Serial.println(WiFi.macAddress());            // note that the arduino println function will print all six mac bytes for us
  Serial.print("IP:  ");                        // print the IP address
  Serial.println(WiFi.localIP());               // In the same way, the println function prints all four IP address bytes
  Serial.print("Subnet masks: ");               // Print the subnet mask
  Serial.println(WiFi.subnetMask());
  Serial.print("Gateway: ");                    // print the gateway IP address
  Serial.println(WiFi.gatewayIP());
  Serial.print("DNS: ");                        // print the DNS IP address
  Serial.println(WiFi.dnsIP());
   
  LCD.begin(16, 2);                             // initialise the LCD
  LCD.setRGB(red, green, blue);                 // set the background colour

  //LCD.print(WiFi.localIP());                    // update the display with the local (i.e. board) IP address
  
  MQTT.subscribe(&ResetFootfall);                         // subscribe to the Footfall feed

  if (MDNS.begin(domainname))
  {
    Serial.print("Access your server at https://");
    Serial.print(domainname);
    Serial.println(".local");
  }
  SERVER.getServer().setRSACert(new BearSSL::X509List(cert), new BearSSL::PrivateKey(key));
  SERVER.on("/", respond);
  SERVER.begin();

  Serial.println("Server is now running");

  /*if(UDP.begin(SOCKET))
  {
    Serial.println("UDP Successfully Connected");  
  }
  else
  {
    Serial.println("UDP Failed to Connect");  
  }*/

  MDNS.addService("https", "tcp", 443);
}

/*********************************************************************************/

void loop() {
  // put your main code here, to run repeatedly:

  SERVER.handleClient();
  MDNS.update();
  
  current_millis = millis();

  if((current_millis - ButtonPollLC) >= ButtonPollC)
  {
    ButtonPoll();
    ButtonPollLC = current_millis;
  }
  if((current_millis - NetworkTransferLC) >= NetworkTransferC)
  {
    NetworkTransfer();
    NetworkTransferLC = current_millis;
  }
  if((current_millis - LCDUpdateLC) >= LCDUpdateC)
  {
    LCDUpdate();
    LCDUpdateLC = current_millis;
  }
  if((current_millis - MinuteAverageLC) >= MinuteAverageC)
  {
    MinuteAverage();
    MinuteAverageLC = current_millis;
  }
  if((current_millis - DeviceDiscoveryLC) >= DeviceDiscoveryC)
  {
    MinuteAverage();
    MinuteAverageLC = current_millis;
  }
  
}

/*********************************************************************************/

int checkbutton(void)
{
  pinMode(Button, INPUT);
  current = digitalRead(Button);
  if (current and not(last))
  {
    delay(5);
    if (digitalRead(Button))
    { 
      pinMode(Button, OUTPUT);
      last = current;
      return 1;     
    }
    else
    {
      pinMode(Button, OUTPUT);
      last = current;
      return 0;
    }
    
  }
  else
  {
    pinMode(Button, OUTPUT);
    last = current;
    return 0;
  }
  
}


/*********************************************************************************/

void ButtonPoll (void){
  if (checkbutton())
  {
    timespressed++; 
    PerMinute++;
  }
  
}

void NetworkTransfer (void){
  MQTTconnect();
  
  // an example of subscription code
  Adafruit_MQTT_Subscribe *subscription;                    // create a subscriber object instance
  while ( subscription = MQTT.readSubscription(1) )      // Read a subscription and wait for max of 5 seconds.
  {                                                         // will return 1 on a subscription being read.
    if (subscription == &ResetFootfall)                     // if the subscription we have receieved matches the one we are after
    {
      Serial.print("Recieved from the ResetFootfall subscription:");  // print the subscription out
      Serial.println((char *)ResetFootfall.lastread);                 // we have to cast the array of 8 bit values back to chars to print
      
      if( strcmp((char*)ResetFootfall.lastread,"1")==0)
      {
        timespressed = 0;    
      }
      /*else if ( strcmp((char*)ResetFootfall.lastread,"0")==0)
      {
        digitalWrite(LEDPIN, LOW);   
      }*/
      // ADD code here to compare (char *)LED.lastread against "HIGH" or "LOW". Refer to previous labs 
      // for how to use STRCMP. You should use two ifs rather than an if then else structure.
      // we could also convert characters received into integer variables via the use of atoi for passing values from the 
      // the broker to the ESP8266
    }
   
  }

  Footfall.publish(timespressed);
  AverageFootfall.publish(LastMinute);
  SensorIdentifier.publish(sensoridentifier);

   if(SERVER.hasArg("resetfootfall"))
  {
    timespressed = 0;
  }
   servepagewithdata();

  #ifdef NOPUBLISH
  if ( !MQTT.ping() ) 
  {
    MQTT.disconnect();
  }
  #endif  
}

void LCDUpdate (void){
  LCD.clear();
  LCD.setRGB(red, green, blue);
  LCD.setCursor(0,0);
  LCD.printf("PerMin: ");
  LCD.print(LastMinute);
  LCD.setCursor(0,1);
  LCD.print(WiFi.localIP());
}

void MinuteAverage (void){
  LastMinute = PerMinute;
  PerMinute = 0;
}

/*void DeviceDiscovery (void){
  int packetsize = 0;
  packetsize = UDP.parsePacket();

  if(packetsize)
  {
    UDP.read(inBUFFER,BUFFERLEN);
    inBUFFER[packetsize] = '\0';
    Serial.print("Received ");
    Serial.print(packetsize);
    Serial.println(" bytes");

    Serial.println("Contents:");
    Serial.println(inBUFFER);
    Serial.print("From IP");
    Serial.println(UDP.remoteIP());
    Serial.print("From port ");
    Serial.println(UDP.remotePort());

    UDP.beginPacket(UDP.remoteIP(),UDP.remotePort());

    IPAddress ip = WiFi.localIP();
    byte oct1 = ip[0];
    byte oct2 = ip[1];
    byte oct3 = ip[2];
    byte oct4 = ip[3];
    char s[16];
    sprintf(outBUFFER, s, "Device IP Address ", "%d.%d.%d.%d", oct1, oct2, oct3, oct4);
    UDP.write(outBUFFER );
    UDP.endPacket();
  }
}*/

/*********************************************************************************/

void MQTTconnect ( void ) 
{
  unsigned char tries = 0;

  // Stop if already connected.
  if ( MQTT.connected() ) 
  {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  while ( MQTT.connect() != 0 )                                        // while we are 
  {     
       pinMode(LEDPIN, OUTPUT);
       digitalWrite(LEDPIN, HIGH);
       Serial.println("Will try to connect again in five seconds");   // inform user
       MQTT.disconnect();                                             // disconnect
       delay(5000);                                                   // wait 5 seconds
       tries++;
       if (tries == 20) 
       {
          Serial.println("problem with communication, forcing WDT for reset");
          while (1)
          {
            ;   // forever do nothing
          }
       }
  }
  
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, LOW);
  Serial.println("MQTT succesfully connected!");
}

void respond ( void )
{
  servepagewithdata();
}

void servepage ( void )
{
    String reply;

    reply += "<!DOCTYPE HTML>";
    reply += "<html>";
    reply += "<head>";
    reply += "<title>Board10</title>";
    reply += "</head>";
    reply += "<body>";
    reply += "<h1>This is Board No. 10</h1>";

    reply += "You can add content here to represent various system parameters etc";
    reply += "</body>";
    reply += "</html>";
    
    SERVER.send(200, "text/html", reply);
}

void servepagewithdata ( void )
{
    String reply;

    reply += "<!DOCTYPE HTML>";
    reply += "<html>";
    reply += "<head>";
    reply += "<title>Board10</title>";
    reply += "</head>";
    reply += "<body>";
    reply += "<h1>This is Board No. 10</h1>";

    reply += "Footfall: ";
    reply += timespressed;
    reply += "Average Footfall: ";
    reply += LastMinute;
    reply += "Sensor Identifier: ";
    reply += sensoridentifier;
    reply += "</body>";
    reply += "</html>";
    
    SERVER.send(200, "text/html", reply);
}

/*String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ;
}*/
