#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>      // include Adafruit graphics library
#include <Adafruit_ST7735.h>   // include Adafruit ST7735 TFT library
#include <ThingSpeak.h>
#include <ESP8266WiFi.h>
#include "..\..\Credentials\CredentialsPvarela.h"
#include "..\..\Credentials\ThingSpeak_Personal_Channels.h"
#include <UniversalTelegramBot.h>   // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ESP8266HTTPClient.h>
#include <WiFiEspUdp.h>
#include <Time.h>
#include "NTPClient.h"
#include <WiFiUdp.h>

// ST7735 TFT module connections
#define TFT_RST   D4
#define TFT_CS    D3
#define TFT_DC    D2
// initialize ST7735 TFT library with hardware SPI module
// SCK (CLK) ---> WemosD1 pin D5
// MOSI(DIN) ---> WemosD1 pin D7

#define NUM_TEMPERATURES		4
#define SIZE_AREA_TEMPERATURES 	ST7735_TFTHEIGHT_160/NUM_TEMPERATURES
#define MAX_SEC_ALLOWED_WITH_NO_SAMPLE	20*60 /*20 minutes with no update data will generate alert!*/

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

// Initialize Telegram BOT
#define BOTtoken "683023753:AAFpv8x1nRmf1QMXdRPvK--XL9S2sI3HFgc"

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
#define CHAT_ID "675668025"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);
WiFiClient  client2;

// Checks for new messages every 1 second.
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time
String formattedCurrentDate;
String dayStamp;
String timeStamp;

String botText;

bool alert_Telegram = true;

int statusCode;
enum enumTemp{
	enumExteriorTemp,
	enumSalonTemp,
	enumOficinaTemp,
	enumPasilloTemp
};

// Handle what happens when you receive new messages
void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }

    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/start") {
      String welcome = "Welcome, " + from_name + ".\n";
      welcome += "Use the following commands to control your alert\n\n";
      welcome += "/alert_on to send alert when any Sensor is detected as not updated \n";
      welcome += "/alert_off to not send alert \n";
      bot.sendMessage(chat_id, welcome, "");
    }

    if (text == "/alert_on") {
      bot.sendMessage(chat_id, "Alert will be sent when any Sensor is detected as not updated ", "");
      alert_Telegram = true;
    }

    if (text == "/alert_off") {
    	bot.sendMessage(chat_id, "Alert will not be sent when any Sensor is detected as not updated ", "");
    	alert_Telegram = false;
    }

  }
}

void botCheckMessages()
{
	if (millis() > lastTimeBotRan + botRequestDelay)
	{
	  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

	  while(numNewMessages) {
		Serial.println("got response");
		handleNewMessages(numNewMessages);
		numNewMessages = bot.getUpdates(bot.last_message_received + 1);
	  }
	  lastTimeBotRan = millis();
	}
}

time_t SetDateTime(int y, int m, int d, int h, int mi, int s  )
{
	tmElements_t DateTime ;
	DateTime.Second = s;
	DateTime.Minute = mi;
	DateTime.Hour = h;
	DateTime.Day = d ;
	DateTime.Month = m ;
	DateTime.Year = y -1970 ;

	return makeTime(DateTime);
}

time_t convertToTime(String calTimestamp) {
  struct tm tm;
  String year = calTimestamp.substring(0, 4);
  String month = calTimestamp.substring(5, 7);
  if (month.startsWith("0")) {
    month = month.substring(1);
  }
  String day = calTimestamp.substring(8, 10);
  if (day.startsWith("0")) {
    month = day.substring(1);
  }
  tm.tm_year = year.toInt();
  tm.tm_mon = month.toInt();
  tm.tm_mday = day.toInt();
  String hour = calTimestamp.substring(11, 13);
  tm.tm_hour = hour.toInt();
  String min = calTimestamp.substring(14, 16);
  tm.tm_min = min .toInt();
  String sec = calTimestamp.substring(17, 19);
  tm.tm_sec = sec.toInt();

  return SetDateTime(tm.tm_year,tm.tm_mon, tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);
}

String getCurrentTime() {
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  return timeClient.getFormattedDate();

}

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
	case enumOficinaTemp:
	case enumPasilloTemp:
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
		resultColor = ST77XX_RED;
		break;
	}
	return (resultColor);
}

void setup(void)
{
  Serial.begin(115200);

#ifdef ESP8266
  client.setInsecure();
#endif

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
		ThingSpeak.begin(client2);
		botText = "Sensor Display is connected to " + (String)ssid + " (IP:" + WiFi.localIP().toString() + ", MAC: " + WiFi.macAddress() + ")";
		Serial.println(botText);
		bot.sendMessage(CHAT_ID, botText, "");
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
  tft.print("Oficina ");
  tft.setTextColor(ST77XX_WHITE, ST7735_BLACK);  // set text color to green and black background
  tft.setCursor(30, TITLE_4_POSITION);
  tft.print("Pasillo ");

  timeClient.begin();
  timeClient.setTimeOffset(3600);

}

void printDateTime(time_t  t)
{
	Serial.print(day(t)) ;    Serial.print(+ "/") ;   Serial.print(month(t));   Serial.print(+ "/") ;
	Serial.print(year(t));    Serial.print( " ") ;
	Serial.print(hour(t));   Serial.print(+ ":") ;   Serial.print(minute(t));   Serial.print(":") ;
	Serial.println(second(t));
	delay(1000);
}


// main loop
void loop()
{
	time_t  currentT,lastSampleSent;        // Date-Time variables

	char _buffer[8];
	uint16_t colorTempExterior;
	uint16_t colorTempSalon;
	uint16_t colorTempOficina;
	uint16_t colorTempPasillo;
	String tempExteriorTime;
	String tempSalonTime;
	String tempOficinaTime;
	String tempPasilloTime;
	String tempExteriorTimeOld;
	String tempSalonTimeOld;
	String tempOficinaTimeOld;
	String tempPasilloTimeOld;
	bool alertDisconnectionTempExterior = true;
	bool alertDisconnectionTempSalon = true;
	bool alertDisconnectionTempOficina = true;
	bool alertDisconnectionTempPasillo = true;

	static float tempExterior = -20;
	static float tempSalon = 0.0;
	static float tempOficina = 0.0;
	static float tempPasillo = 0.0;
	static float tempExteriorOld = tempExterior;
	static float tempSalonOld = tempSalon;
	static float tempOficinaOld = tempOficina;
	static float tempPasilloOld = tempPasillo;
	int delimiter_1, delimiter_2;

	Serial.println("\n\r\n\r");

	botText = "";

	/*Current time*/
    formattedCurrentDate = getCurrentTime();
	currentT = convertToTime(formattedCurrentDate);
	Serial.print("Current time:");
	Serial.println(formattedCurrentDate);

    /*Get temperature exterior*/
	tempExterior = ThingSpeak.readFloatField(channelTempExterior, FieldNumber1, thingSpeakReadAPIKey_Exterior);
	statusCode = ThingSpeak.getLastReadStatus();
	if (statusCode == 200)
	{
	  tempExteriorTime = ThingSpeak.readCreatedAt(channelTempExterior, thingSpeakReadAPIKey_Exterior);
	  lastSampleSent = convertToTime(tempExteriorTime);
	  Serial.print("Temperature exterior ");
	  Serial.print(tempExterior);
	  Serial.print("ºC at ");
	  delimiter_1 = tempExteriorTime.indexOf("T");
	  delimiter_2 = tempExteriorTime.indexOf("+");
	  tempExteriorTime = tempExteriorTime.substring(delimiter_1 + 1, delimiter_2);
	  Serial.print(tempExteriorTime);
	  Serial.printf(". No sample during %d s\n\r",(int)(currentT-lastSampleSent));
	  if((int)(currentT-lastSampleSent)> MAX_SEC_ALLOWED_WITH_NO_SAMPLE)
	  {
		alertDisconnectionTempExterior = true;
		if (alert_Telegram == true)
		{
			botText += "Sensor Exterior seems to be disconnected!.";
			Serial.println(botText);
		}
	  }
	  else
	  {
		  alertDisconnectionTempExterior = false;
	  }
	  tempExteriorOld = tempExterior;
	  tempExteriorTimeOld = tempExteriorTime;
	}
	else
	{
	  Serial.println("Unable to read channel / No internet connection");
	  tempExterior = tempExteriorOld;
	  tempExteriorTime = tempExteriorTimeOld;
	}

    /*Get temperature Salon*/
	delay(100);
    tempSalon = ThingSpeak.readFloatField(channelTempSalon, FieldNumber1, thingSpeakReadAPIKey_Salon);
	statusCode = ThingSpeak.getLastReadStatus();
	if (statusCode == 200)
	{
	  tempSalonTime= ThingSpeak.readCreatedAt(channelTempSalon, thingSpeakReadAPIKey_Salon);
	  lastSampleSent = convertToTime(tempSalonTime);
	  Serial.print("Temperature Salon ");
	  Serial.print(tempSalon);
	  Serial.print("ºC at ");
	  delimiter_1 = tempSalonTime.indexOf("T");
	  delimiter_2 = tempSalonTime.indexOf("+");
	  tempSalonTime = tempSalonTime.substring(delimiter_1 + 1, delimiter_2);
	  Serial.print(tempSalonTime);
	  Serial.printf(". No sample during %d s\n\r",(int)(currentT-lastSampleSent));
	  if((int)(currentT-lastSampleSent)> MAX_SEC_ALLOWED_WITH_NO_SAMPLE)
	  {
		  alertDisconnectionTempSalon = true;
		  if (alert_Telegram == true)
			{
			    botText += "Sensor Salon seems to be disconnected!.";
				Serial.println(botText);
			}
	  }
	  else
	  {
		  alertDisconnectionTempSalon = false;
	  }
	  tempSalonOld = tempSalon;
	  tempSalonTimeOld = tempSalonTime;
	}
	else
	{
	  Serial.println("Unable to read channel / No internet connection");
	  tempSalon = tempSalonOld;
	  tempSalonTime = tempSalonTimeOld;
	}

    /*Get temperature Alén*/
	delay(100);
    tempOficina  = ThingSpeak.readFloatField(channelTempOficina, FieldNumber1, thingSpeakReadAPIKey_Oficina);
	statusCode = ThingSpeak.getLastReadStatus();
	if (statusCode == 200)
	{
	  tempOficinaTime= ThingSpeak.readCreatedAt(channelTempOficina, thingSpeakReadAPIKey_Oficina);
	  lastSampleSent = convertToTime(tempOficinaTime);
	  Serial.print("Temperature Alén ");
	  Serial.print(tempOficina);
	  Serial.print("ºC at ");
	  delimiter_1 = tempOficinaTime.indexOf("T");
	  delimiter_2 = tempOficinaTime.indexOf("+");
	  tempOficinaTime = tempOficinaTime.substring(delimiter_1 + 1, delimiter_2);
	  Serial.print(tempOficinaTime);
	  Serial.printf(". No sample during %d s\n\r",(int)(currentT-lastSampleSent));
	  if((int)(currentT-lastSampleSent)> MAX_SEC_ALLOWED_WITH_NO_SAMPLE)
	  {
		alertDisconnectionTempOficina = true;
		if (alert_Telegram == true)
		{
			botText += "Sensor Alén Room seems to be disconnected!.";
			Serial.println(botText);
			bot.sendMessage(CHAT_ID, botText, "");
		}
	  }
	  else
	  {
		  alertDisconnectionTempOficina = false;
	  }
	  tempOficinaOld = tempOficina;
	  tempOficinaTimeOld = tempOficinaTime;
	}
	else
	{
	  Serial.println("Unable to read channel / No internet connection");
	  tempOficina = tempOficinaOld;
	  tempOficinaTime = tempOficinaTimeOld;
	}

    /*Get temperature Pasillo*/
	delay(100);
    tempPasillo  = ThingSpeak.readFloatField(channelTempPasillo, FieldNumber1, thingSpeakReadAPIKey_Pasillo);
	statusCode = ThingSpeak.getLastReadStatus();
	if (statusCode == 200)
	{
	  tempPasilloTime= ThingSpeak.readCreatedAt(channelTempPasillo, thingSpeakReadAPIKey_Pasillo);
	  lastSampleSent = convertToTime(tempPasilloTime);
	  Serial.print("Temperature Pasillo ");
	  Serial.print(tempPasillo);
	  Serial.print("ºC at ");
	  delimiter_1 = tempPasilloTime.indexOf("T");
	  delimiter_2 = tempPasilloTime.indexOf("+");
	  tempPasilloTime = tempPasilloTime.substring(delimiter_1 + 1, delimiter_2);
	  Serial.print(tempPasilloTime);
	  Serial.printf(". No sample during %d s\n\r",(int)(currentT-lastSampleSent));
	  if((int)(currentT-lastSampleSent)> MAX_SEC_ALLOWED_WITH_NO_SAMPLE)
	  {
			alertDisconnectionTempPasillo = true;
			if (alert_Telegram == true)
			{
				botText += "Sensor Pasillo Room seems to be disconnected!.";
				Serial.println(botText);
				bot.sendMessage(CHAT_ID, botText, "");
			}
	  }
	  else
	  {
		  alertDisconnectionTempPasillo = false;
	  }
	  tempPasilloOld = tempPasillo;
	  tempPasilloTimeOld = tempPasilloTime;

	}
	else
	{
	  Serial.println("Unable to read channel / No internet connection");
	  tempPasillo = tempPasilloOld;
	  tempPasilloTime = tempPasilloTimeOld;
	}

	colorTempExterior = SetColorTempTxt(enumExteriorTemp, tempExterior);
	colorTempSalon = SetColorTempTxt(enumSalonTemp, tempSalon);
	colorTempOficina = SetColorTempTxt(enumOficinaTemp, tempOficina);
	colorTempPasillo = SetColorTempTxt(enumPasilloTemp, tempPasillo);

	//print temperatura exterior;
	tft.setTextSize(1);
	if(alertDisconnectionTempExterior)
	{
		tft.setTextColor(ST77XX_RED, ST77XX_BLACK);     // set text color to red and black background
	}
	else
	{
		tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);     // set text color to white and black background
	}
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
	if(alertDisconnectionTempSalon)
	{
		tft.setTextColor(ST77XX_RED, ST77XX_BLACK);     // set text color to red and black background
	}
	else
	{
		tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);     // set text color to white and black background
	}
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
	if(alertDisconnectionTempOficina)
	{
		tft.setTextColor(ST77XX_RED, ST77XX_BLACK);     // set text color to red and black background
	}
	else
	{
		tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);     // set text color to white and black background
	}
	tft.setCursor(63,SAMPLE_TIME_3_POSITION);
	tft.print(tempOficinaTime);
	tft.setTextSize(2);
	dtostrf(tempOficina, 2, 1, _buffer);
	tft.setTextColor(colorTempOficina, ST77XX_BLACK);  // set text color to orange and black background
	tft.setCursor(TEMPERATURE_X_3_POSITION,TEMPERATURE_Y_3_POSITION);
	tft.print(_buffer);
	tft.drawCircle(TEMPERATURE_X_CIRCLE_POSITION, TEMPERATURE_Y_3_POSITION, 2, colorTempOficina);  // print degree symbol ( ° )
	tft.setCursor(TEMPERATURE_X_UNITS_POSITION, TEMPERATURE_Y_3_POSITION);
	tft.print("C");

	// print temperature Pasillo (in °C)
	tft.setTextSize(1);
	if(alertDisconnectionTempPasillo)
	{
		tft.setTextColor(ST77XX_RED, ST77XX_BLACK);     // set text color to red and black background
	}
	else
	{
		tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);     // set text color to white and black background
	}
	tft.setCursor(63,SAMPLE_TIME_4_POSITION);
	tft.print(tempPasilloTime);
	tft.setTextSize(2);
	dtostrf(tempPasillo, 2, 1, _buffer);
	tft.setTextColor(colorTempPasillo, ST77XX_BLACK);  // set text color to orange and black background
	tft.setCursor(TEMPERATURE_X_4_POSITION,TEMPERATURE_Y_4_POSITION);
	tft.print(_buffer);
	tft.drawCircle(TEMPERATURE_X_CIRCLE_POSITION, TEMPERATURE_Y_4_POSITION, 2, colorTempPasillo);  // print degree symbol ( ° )
	tft.setCursor(TEMPERATURE_X_UNITS_POSITION, TEMPERATURE_Y_4_POSITION);
	tft.print("C");

//  Serial.println(ntpClient.getUnixTime());
	if(alert_Telegram == true)
	{
		bot.sendMessage(CHAT_ID, botText, "");
	}
	delay(5000);    // wait a second
	botCheckMessages();
}
