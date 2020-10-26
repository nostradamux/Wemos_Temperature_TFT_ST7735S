#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>      // include Adafruit graphics library
#include <Adafruit_ST7735.h>   // include Adafruit ST7735 TFT library
#include <ThingSpeak.h>
#include <ESP8266WiFi.h>
#include "..\..\Credentials\CredentialsPvarela.h"
//#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiEspUdp.h>

#include <EasyNTPClient.h>
WiFiEspUDP udp;
EasyNTPClient ntpClient(udp, "pool.ntp.org", ((5*60*60)+(30*60))); // IST = GMT + 5:30

// ST7735 TFT module connections
#define TFT_RST   D4     // TFT RST pin is connected to NodeMCU pin D4 (GPIO2)
#define TFT_CS    D3     // TFT CS  pin is connected to NodeMCU pin D4 (GPIO0)
#define TFT_DC    D2     // TFT DC  pin is connected to NodeMCU pin D4 (GPIO4)

//CS D8
//MOSI D7
//SCK D5

// initialize ST7735 TFT library with hardware SPI module
// SCK (CLK) ---> NodeMCU pin D5 (GPIO14)
// MOSI(DIN) ---> NodeMCU pin D7 (GPIO13)
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

static float tempExterior = -1.9;
static float tempSalon = 10;
static float tempHabitacion = 10;

unsigned long channelTempExterior = 884965;            // Channel ID
const char * thingSpeakReadAPIKey1 = "K79HY3S0AFG0BJKB"; // Read API Key
unsigned long channelTempSalon = 220821;            // Channel ID
const char * thingSpeakReadAPIKey2 = "Q9R1021ONI8IOV5E"; // Read API Key
unsigned long channelTempInterior = 641875;            // Channel ID
const char * thingSpeakReadAPIKey3 = "NZMV7DRIIPZO6ART"; // Read API Key
const int FieldNumber1 = 1;  // The field you wish to read
WiFiClient  client;
int statusCode;

void setup(void)
{
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
   WiFi.begin(ssid, password);
   if (WiFi.waitForConnectResult() != WL_CONNECTED) {
 	Serial.println("Connection Failed! Rebooting...");
 	delay(5000);
 	ESP.restart();
   }
   else
   {
 	  Serial.println("WIFI OK!");
 	  ThingSpeak.begin(client);
   }

  tft.initR(INITR_BLACKTAB);     // initialize a ST7735S chip, black tab
  tft.fillScreen(ST77XX_BLACK);  // fill screen with black color
//  tft.drawFastHLine(0, 2 ,  tft.width(), ST77XX_CYAN);   // draw horizontal white line at position (0, 30)
  tft.drawFastHLine(0, 53 ,  tft.width(), ST77XX_CYAN);   // draw horizontal white line at position (0, 30)
  tft.drawFastHLine(0, 108 ,  tft.width(), ST77XX_CYAN);   // draw horizontal white line at position (0, 30)

  if( 0 )
  {  // connection error or device address wrong!
    tft.setTextColor(ST77XX_RED, ST77XX_CYAN);   // set text color to red and black background
    tft.setTextSize(2);         // text size = 2
    tft.setCursor(5, 76);       // move cursor to position (5, 76) pixel
    tft.print("Connection");
    tft.setCursor(35, 100);     // move cursor to position (35, 100) pixel
    tft.print("Error");
    while(1);  // stay here
  }

  //tft.drawFastHLine(0, 80,  tft.width(), ST77XX_CYAN);  // draw horizontal white line at position (0, 76)
  //tft.drawFastHLine(0, 120,  tft.width(), ST77XX_CYAN);  // draw horizontal white line at position (0, 122)
  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);     // set text color to red and black background
  tft.setCursor(14, 8);              // move cursor to position (25, 39) pixel
  tft.print("Exterior ");
  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);  // set text color to cyan and black background
  tft.setCursor(22, 61);              // move cursor to position (34, 85) pixel
  tft.print("Salon ");
  tft.setTextColor(ST77XX_CYAN, ST7735_BLACK);  // set text color to green and black background
  tft.setCursor(6, 112);              // move cursor to position (34, 131) pixel
  tft.print("Dormitorio ");
  tft.setTextSize(2);                 // text size = 2

}

// main loop
void loop()
{
  char _buffer[8];
  char _buffer_2[16];

  uint16_t colorTempExterior;
  uint16_t colorTempSalon;
  uint16_t colorTempHabitacion;
  String tempExteriorTime;
  String tempSalonTime;
  String tempHabitacionTime;
  int delimiter_1, delimiter_2;


    tempExterior = ThingSpeak.readFloatField(channelTempExterior, FieldNumber1, thingSpeakReadAPIKey1);
	statusCode = ThingSpeak.getLastReadStatus();
	if (statusCode == 200)
	{
	  Serial.print("Temperature exterior ");
	  Serial.print(tempExterior);
	  Serial.print("ºC at ");
	  tempExteriorTime = ThingSpeak.readCreatedAt(channelTempExterior, thingSpeakReadAPIKey1);
	  delimiter_1 = tempExteriorTime.indexOf("T");
	  delimiter_2 = tempExteriorTime.indexOf("+");
	  tempExteriorTime = tempExteriorTime.substring(delimiter_1 + 1, delimiter_2);
	  Serial.println(tempExteriorTime);
	}
	else
	{
	  Serial.println("Unable to read channel / No internet connection");
	}


	delay(100);
    tempSalon = ThingSpeak.readFloatField(channelTempSalon, FieldNumber1, thingSpeakReadAPIKey2);
	statusCode = ThingSpeak.getLastReadStatus();
	if (statusCode == 200)
	{
	  Serial.print("Temperature Salon ");
	  Serial.print(tempSalon);
	  Serial.print("ºC at ");
	  tempSalonTime= ThingSpeak.readCreatedAt(channelTempSalon, thingSpeakReadAPIKey2);
	  delimiter_1 = tempSalonTime.indexOf("T");
	  delimiter_2 = tempSalonTime.indexOf("+");
	  tempSalonTime = tempSalonTime.substring(delimiter_1 + 1, delimiter_2);
	  Serial.println(tempSalonTime);
	}
	else
	{
	  Serial.println("Unable to read channel / No internet connection");
	}
	delay(100);
    tempHabitacion  = ThingSpeak.readFloatField(channelTempInterior, FieldNumber1, thingSpeakReadAPIKey3);
	statusCode = ThingSpeak.getLastReadStatus();
	if (statusCode == 200)
	{
	  Serial.print("Temperature Interior ");
	  Serial.print(tempHabitacion);
	  Serial.print("ºC at ");
	  tempHabitacionTime= ThingSpeak.readCreatedAt(channelTempInterior, thingSpeakReadAPIKey3);
	  delimiter_1 = tempHabitacionTime.indexOf("T");
	  delimiter_2 = tempHabitacionTime.indexOf("+");
	  tempHabitacionTime = tempHabitacionTime.substring(delimiter_1 + 1, delimiter_2);
	  Serial.println(tempHabitacionTime);

	}
	else
	{
	  Serial.println("Unable to read channel / No internet connection");
	}
	delay(100);

  if(tempExterior >= 37.0)
  {
	  colorTempExterior = ST77XX_MAGENTA;
  }
  else if((tempExterior < 37) && (tempExterior >= 33))
  {
	  colorTempExterior = ST77XX_RED;
  }
  else if((tempExterior < 33) && (tempExterior >=30))
  {
	  colorTempExterior = ST77XX_ORANGE;
  }
  else if((tempExterior < 30) && (tempExterior >=25))
  {
	  colorTempExterior = ST77XX_YELLOW;
  }
  else if((tempExterior < 25) && (tempExterior >=17))
  {
	  colorTempExterior = ST77XX_GREEN;
  }
  else if((tempExterior < 17) && (tempExterior >=5))
  {
	  colorTempExterior = ST77XX_CYAN;
  }
  else if((tempExterior < 5) && (tempExterior >=0))
  {
	  colorTempExterior = ST77XX_BLUE;
  }
  else
  {
	  colorTempExterior = ST77XX_WHITE;
  }

  if(tempSalon >= 30.0)
    {
  	  	colorTempSalon = ST77XX_MAGENTA;
    }
    else if((tempSalon < 30) && (tempSalon >= 27))
    {
    	colorTempSalon = ST77XX_RED;
    }
    else if((tempSalon < 27) && (tempSalon >=25))
    {
    	colorTempSalon = ST77XX_ORANGE;
    }
	else if((tempSalon < 25) && (tempSalon >=22.5))
	{
		colorTempSalon = ST77XX_YELLOW;
	}
	else if((tempSalon < 22.5) && (tempSalon >=20.5))
	{
		colorTempSalon = ST77XX_GREEN;
	}
	else if((tempSalon < 20.5) && (tempSalon >=18))
	{
		colorTempSalon = ST77XX_CYAN;
	}
	else if((tempSalon < 18) && (tempSalon >=14))
	{
		colorTempSalon = ST77XX_BLUE;
	}
	else
	{
		colorTempSalon = ST77XX_WHITE;
	}

  if(tempHabitacion >= 30.0)
    {
  	  	colorTempHabitacion = ST77XX_MAGENTA;
    }
    else if((tempHabitacion < 30) && (tempHabitacion >= 27))
    {
    	colorTempHabitacion = ST77XX_RED;
    }
    else if((tempHabitacion < 27) && (tempHabitacion >=25))
    {
    	colorTempHabitacion = ST77XX_ORANGE;
    }
	else if((tempHabitacion < 25) && (tempHabitacion >=22.5))
	{
		colorTempHabitacion = ST77XX_YELLOW;
	}
	else if((tempHabitacion < 22.5) && (tempHabitacion >=20.5))
	{
		colorTempHabitacion = ST77XX_GREEN;
	}
	else if((tempHabitacion < 20.5) && (tempHabitacion >=18))
	{
		colorTempHabitacion = ST77XX_CYAN;
	}
	else if((tempHabitacion < 18) && (tempHabitacion >=14))
	{
		colorTempHabitacion = ST77XX_BLUE;
	}
	else
	{
		colorTempHabitacion = ST77XX_WHITE;
	}

  //print temperatura exterior;
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);     // set text color to red and black background
  tft.setCursor(68, 8);              // move cursor to position (25, 39) pixel
  tft.print(tempExteriorTime);
  tft.setTextSize(2);
  dtostrf(tempExterior, 2, 1, _buffer);
  tft.setTextColor(colorTempExterior, ST77XX_BLACK);  // set text color to yellow and black background
  tft.setCursor(35, 32);
  tft.print(_buffer);
  tft.drawCircle(89, 32, 2, colorTempExterior);  // print degree symbol ( ° )
  tft.setCursor(95, 32);
  tft.print("C");

  // print temperature Salon (in °C)
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);     // set text color to red and black background
  tft.setCursor(60, 61);              // move cursor to position (25, 39) pixel
  tft.print(tempSalonTime);
  tft.setTextSize(2);
  dtostrf(tempSalon, 2, 1, _buffer);
  tft.setTextColor(colorTempSalon, ST77XX_BLACK);  // set text color to magenta and black background
  tft.setCursor(35, 84);
  tft.print(_buffer);
  tft.drawCircle(89, 84, 2, colorTempSalon);  // print degree symbol ( ° )
  tft.setCursor(95, 84);
  tft.print("C");

  // print temperature Habitacion (in °C)
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);     // set text color to red and black background
  tft.setCursor(74, 112);              // move cursor to position (25, 39) pixel
  tft.print(tempHabitacionTime);
  tft.setTextSize(2);
  dtostrf(tempHabitacion, 2, 1, _buffer);
  tft.setTextColor(colorTempHabitacion, ST77XX_BLACK);  // set text color to orange and black background
  tft.setCursor(35, 132);
  tft.print(_buffer);
  tft.drawCircle(89, 132, 2, colorTempHabitacion);  // print degree symbol ( ° )
  tft.setCursor(95, 132);
  tft.print("C");

//  Serial.println(ntpClient.getUnixTime());


  delay(5000);    // wait a second
}
