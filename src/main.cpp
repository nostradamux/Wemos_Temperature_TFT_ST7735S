#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>      // include Adafruit graphics library
#include <Adafruit_ST7735.h>   // include Adafruit ST7735 TFT library
#include <ThingSpeak.h>
#include <ESP8266WiFi.h>
#include "..\..\Credentials\CredentialsPvarela.h"
#include "..\..\Credentials\ThingSpeak_Personal_Channels.h"
#include <ESP8266HTTPClient.h>
#include <WiFiEspUdp.h>
#include <EasyNTPClient.h>

// ST7735 TFT module connections
#define TFT_RST   D4
#define TFT_CS    D3
#define TFT_DC    D2
// initialize ST7735 TFT library with hardware SPI module
// SCK (CLK) ---> WemosD1 pin D5
// MOSI(DIN) ---> WemosD1 pin D7

#define NUM_TEMPERATURES		4
#define SIZE_AREA_TEMPERATURES 	ST7735_TFTHEIGHT_160/NUM_TEMPERATURES

#define LINE_1_POSITION 				1*(ST7735_TFTHEIGHT_160/(NUM_TEMPERATURES))-2
#define LINE_2_POSITION 				2*(ST7735_TFTHEIGHT_160/(NUM_TEMPERATURES))-2
#define LINE_3_POSITION 				3*(ST7735_TFTHEIGHT_160/(NUM_TEMPERATURES))-2
#define TITLE_1_POSITION				4 //8
#define TITLE_2_POSITION				LINE_1_POSITION + 4
#define TITLE_3_POSITION				LINE_2_POSITION + 4
#define TITLE_4_POSITION				LINE_3_POSITION + 4
#define SAMPLE_TIME_1_POSITION 			TITLE_1_POSITION
#define SAMPLE_TIME_2_POSITION 			TITLE_2_POSITION
#define SAMPLE_TIME_3_POSITION 			TITLE_3_POSITION
#define SAMPLE_TIME_4_POSITION 			TITLE_4_POSITION
#define TEMPERATURE_Y_1_POSITION 		LINE_1_POSITION - (SIZE_AREA_TEMPERATURES/2)
#define TEMPERATURE_Y_2_POSITION 		LINE_2_POSITION - (SIZE_AREA_TEMPERATURES/2)
#define TEMPERATURE_Y_3_POSITION 		LINE_3_POSITION - (SIZE_AREA_TEMPERATURES/2)
#define TEMPERATURE_Y_4_POSITION 		ST7735_TFTHEIGHT_160 - (SIZE_AREA_TEMPERATURES/2)
#define TEMPERATURE_X_1_POSITION		35
#define TEMPERATURE_X_2_POSITION		TEMPERATURE_X_1_POSITION
#define TEMPERATURE_X_3_POSITION		TEMPERATURE_X_1_POSITION
#define TEMPERATURE_X_4_POSITION		TEMPERATURE_X_1_POSITION
#define TEMPERATURE_X_CIRCLE_POSITION	TEMPERATURE_X_1_POSITION + 54
#define TEMPERATURE_X_UNITS_POSITION	TEMPERATURE_X_1_POSITION + 60

WiFiEspUDP udp;
EasyNTPClient ntpClient(udp, "pool.ntp.org", ((5*60*60)+(30*60))); // IST = GMT + 5:30
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

WiFiClient  client;
int statusCode;
enum enumTemp{
	enumExteriorTemp,
	enumSalonTemp,
	enumAlenTemp,
	enumEderTemp
};

uint16_t SetColorTempTxt(enumTemp selection, float temperatureToEvaluate)
{
	uint16_t resultColor;

	switch (selection)
	{
		case enumExteriorTemp:
		  if(temperatureToEvaluate >= 37.0)
		  {
			  resultColor = ST77XX_MAGENTA;
		  }
		  else if((temperatureToEvaluate < 37)&& (temperatureToEvaluate >= 33))
		  {
			  resultColor = ST77XX_RED;
		  }
		  else if((temperatureToEvaluate < 33) && (temperatureToEvaluate >=30))
		  {
			  resultColor = ST77XX_ORANGE;
		  }
		  else if((temperatureToEvaluate < 30) && (temperatureToEvaluate >=25))
		  {
			  resultColor = ST77XX_YELLOW;
		  }
		  else if((temperatureToEvaluate < 25) && (temperatureToEvaluate >=17))
		  {
			  resultColor = ST77XX_GREEN;
		  }
		  else if((temperatureToEvaluate < 17) && (temperatureToEvaluate >=5))
		  {
			  resultColor = ST77XX_CYAN;
		  }
		  else if((temperatureToEvaluate < 5) && (temperatureToEvaluate >=0))
		  {
			  resultColor = ST77XX_BLUE;
		  }
		  else
		  {
			  resultColor = ST77XX_WHITE;
		  }
		break;
	case enumSalonTemp:
	case enumAlenTemp:
	case enumEderTemp:
		if(temperatureToEvaluate >= 30.0)
		    {
		  	  	resultColor = ST77XX_MAGENTA;
		    }
		    else if((temperatureToEvaluate < 30) && (temperatureToEvaluate >= 27))
		    {
		    	resultColor = ST77XX_RED;
		    }
		    else if((temperatureToEvaluate < 27) && (temperatureToEvaluate >=25))
		    {
		    	resultColor = ST77XX_ORANGE;
		    }
			else if((temperatureToEvaluate < 25) && (temperatureToEvaluate >=22.5))
			{
				resultColor = ST77XX_YELLOW;
			}
			else if((temperatureToEvaluate < 22.5) && (temperatureToEvaluate >=20.5))
			{
				resultColor = ST77XX_GREEN;
			}
			else if((temperatureToEvaluate < 20.5) && (temperatureToEvaluate >=18))
			{
				resultColor = ST77XX_CYAN;
			}
			else if((temperatureToEvaluate < 18) && (temperatureToEvaluate >=14))
			{
				resultColor = ST77XX_BLUE;
			}
			else
			{
				resultColor = ST77XX_WHITE;
			}
		break;
	default:
		break;
	}
	return (resultColor);
}

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

  tft.drawFastHLine(0, LINE_1_POSITION ,  tft.width(), ST77XX_WHITE);   // draw horizontal white line at position (0, 30)
  tft.drawFastHLine(0, LINE_2_POSITION ,  tft.width(), ST77XX_WHITE);   // draw horizontal white line at position (0, 30)
  tft.drawFastHLine(0, LINE_3_POSITION ,  tft.width(), ST77XX_WHITE);   // draw horizontal white line at position (0, 30)

  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);  // set text color to red and black background
  tft.setCursor(14, TITLE_1_POSITION);
  tft.print("EXTERIOR ");
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);  // set text color to cyan and black background
  tft.setCursor(22, TITLE_2_POSITION);
  tft.print("SALON ");
  tft.setTextColor(ST77XX_WHITE, ST7735_BLACK);  // set text color to green and black background
  tft.setCursor(30, TITLE_3_POSITION);
  tft.print("ALEN ");
  tft.setTextColor(ST77XX_WHITE, ST7735_BLACK);  // set text color to green and black background
  tft.setCursor(30, TITLE_4_POSITION);
  tft.print("EDER ");

}

// main loop
void loop()
{
	char _buffer[8];
	uint16_t colorTempExterior;
	uint16_t colorTempSalon;
	uint16_t colorTempAlen;
	uint16_t colorTempEder;
	String tempExteriorTime;
	String tempSalonTime;
	String tempAlenTime;
	String tempEderTime;
	static float tempExterior = -1.9;
	static float tempSalon = 10;
	static float tempAlen = 10;
	static float tempEder = 10;
	int delimiter_1, delimiter_2;

    /*Get temperature exterior*/
	tempExterior = ThingSpeak.readFloatField(channelTempExterior, FieldNumber1, thingSpeakReadAPIKey_Exterior);
	statusCode = ThingSpeak.getLastReadStatus();
	if (statusCode == 200)
	{
	  Serial.print("Temperature exterior ");
	  Serial.print(tempExterior);
	  Serial.print("ºC at ");
	  tempExteriorTime = ThingSpeak.readCreatedAt(channelTempExterior, thingSpeakReadAPIKey_Exterior);
	  delimiter_1 = tempExteriorTime.indexOf("T");
	  delimiter_2 = tempExteriorTime.indexOf("+");
	  tempExteriorTime = tempExteriorTime.substring(delimiter_1 + 1, delimiter_2);
	  Serial.println(tempExteriorTime);
	}
	else
	{
	  Serial.println("Unable to read channel / No internet connection");
	}

    /*Get temperature Salon*/
	delay(100);
    tempSalon = ThingSpeak.readFloatField(channelTempSalon, FieldNumber1, thingSpeakReadAPIKey_Salon);
	statusCode = ThingSpeak.getLastReadStatus();
	if (statusCode == 200)
	{
	  Serial.print("Temperature Salon ");
	  Serial.print(tempSalon);
	  Serial.print("ºC at ");
	  tempSalonTime= ThingSpeak.readCreatedAt(channelTempSalon, thingSpeakReadAPIKey_Salon);
	  delimiter_1 = tempSalonTime.indexOf("T");
	  delimiter_2 = tempSalonTime.indexOf("+");
	  tempSalonTime = tempSalonTime.substring(delimiter_1 + 1, delimiter_2);
	  Serial.println(tempSalonTime);
	}
	else
	{
	  Serial.println("Unable to read channel / No internet connection");
	}

    /*Get temperature Alén*/
	delay(100);
    tempAlen  = ThingSpeak.readFloatField(channelTempAlen, FieldNumber1, thingSpeakReadAPIKey_Alen);
	statusCode = ThingSpeak.getLastReadStatus();
	if (statusCode == 200)
	{
	  Serial.print("Temperature Alén ");
	  Serial.print(tempAlen);
	  Serial.print("ºC at ");
	  tempAlenTime= ThingSpeak.readCreatedAt(channelTempAlen, thingSpeakReadAPIKey_Alen);
	  delimiter_1 = tempAlenTime.indexOf("T");
	  delimiter_2 = tempAlenTime.indexOf("+");
	  tempAlenTime = tempAlenTime.substring(delimiter_1 + 1, delimiter_2);
	  Serial.println(tempAlenTime);

	}
	else
	{
	  Serial.println("Unable to read channel / No internet connection");
	}

    /*Get temperature Eder*/
	delay(100);
    tempEder  = ThingSpeak.readFloatField(channelTempEder, FieldNumber1, thingSpeakReadAPIKey_Eder);
	statusCode = ThingSpeak.getLastReadStatus();
	if (statusCode == 200)
	{
	  Serial.print("Temperature Eder ");
	  Serial.print(tempEder);
	  Serial.print("ºC at ");
	  tempEderTime= ThingSpeak.readCreatedAt(channelTempEder, thingSpeakReadAPIKey_Eder);
	  delimiter_1 = tempEderTime.indexOf("T");
	  delimiter_2 = tempEderTime.indexOf("+");
	  tempEderTime = tempEderTime.substring(delimiter_1 + 1, delimiter_2);
	  Serial.println(tempEderTime);

	}
	else
	{
	  Serial.println("Unable to read channel / No internet connection");
	}

	colorTempExterior = SetColorTempTxt(enumExteriorTemp, tempExterior);
	colorTempSalon = SetColorTempTxt(enumSalonTemp, tempSalon);
	colorTempAlen = SetColorTempTxt(enumAlenTemp, tempAlen);
	colorTempEder = SetColorTempTxt(enumEderTemp, tempEder);

	//print temperatura exterior;
	tft.setTextSize(1);
	tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);     // set text color to red and black background
	tft.setCursor(68, SAMPLE_TIME_1_POSITION);
	tft.print(tempExteriorTime);
	tft.setTextSize(2);
	dtostrf(tempExterior, 2, 1, _buffer);
	tft.setTextColor(colorTempExterior, ST77XX_BLACK);  // set text color to yellow and black background
	tft.setCursor(TEMPERATURE_X_1_POSITION, TEMPERATURE_Y_1_POSITION);
	tft.print(_buffer);
	tft.drawCircle(TEMPERATURE_X_CIRCLE_POSITION, TEMPERATURE_Y_1_POSITION, 2, colorTempExterior);  // print degree symbol ( ° )
	tft.setCursor(TEMPERATURE_X_UNITS_POSITION, TEMPERATURE_Y_1_POSITION);
	tft.print("C");

	// print temperature Salon (in °C)
	tft.setTextSize(1);
	tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);     // set text color to red and black background
	tft.setCursor(60, SAMPLE_TIME_2_POSITION );
	tft.print(tempSalonTime);
	tft.setTextSize(2);
	dtostrf(tempSalon, 2, 1, _buffer);
	tft.setTextColor(colorTempSalon, ST77XX_BLACK);  // set text color to magenta and black background
	tft.setCursor(TEMPERATURE_X_2_POSITION, TEMPERATURE_Y_2_POSITION);
	tft.print(_buffer);
	tft.drawCircle(TEMPERATURE_X_CIRCLE_POSITION, TEMPERATURE_Y_2_POSITION, 2, colorTempSalon);  // print degree symbol ( ° )
	tft.setCursor(TEMPERATURE_X_UNITS_POSITION, TEMPERATURE_Y_2_POSITION);
	tft.print("C");

	// print temperature Alén (in °C)
	tft.setTextSize(1);
	tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);     // set text color to red and black background
	tft.setCursor(63,SAMPLE_TIME_3_POSITION);
	tft.print(tempAlenTime);
	tft.setTextSize(2);
	dtostrf(tempAlen, 2, 1, _buffer);
	tft.setTextColor(colorTempAlen, ST77XX_BLACK);  // set text color to orange and black background
	tft.setCursor(TEMPERATURE_X_3_POSITION,TEMPERATURE_Y_3_POSITION);
	tft.print(_buffer);
	tft.drawCircle(TEMPERATURE_X_CIRCLE_POSITION, TEMPERATURE_Y_3_POSITION, 2, colorTempAlen);  // print degree symbol ( ° )
	tft.setCursor(TEMPERATURE_X_UNITS_POSITION, TEMPERATURE_Y_3_POSITION);
	tft.print("C");

	// print temperature Eder (in °C)
	tft.setTextSize(1);
	tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);     // set text color to red and black background
	tft.setCursor(63,SAMPLE_TIME_4_POSITION);
	tft.print(tempEderTime);
	tft.setTextSize(2);
	dtostrf(tempEder, 2, 1, _buffer);
	tft.setTextColor(colorTempEder, ST77XX_BLACK);  // set text color to orange and black background
	tft.setCursor(TEMPERATURE_X_4_POSITION,TEMPERATURE_Y_4_POSITION);
	tft.print(_buffer);
	tft.drawCircle(TEMPERATURE_X_CIRCLE_POSITION, TEMPERATURE_Y_4_POSITION, 2, colorTempEder);  // print degree symbol ( ° )
	tft.setCursor(TEMPERATURE_X_UNITS_POSITION, TEMPERATURE_Y_4_POSITION);
	tft.print("C");

//  Serial.println(ntpClient.getUnixTime());

  delay(5000);    // wait a second
}
