#include "FS.h"
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <EEPROM.h>
//#include "max6675.h"
#include "temp.h"
#include "oil.h"
#include "volts.h"
#include "music.h"
#include "settings.h"
#include "reset.h"
#include "back.h"

Adafruit_ADS1115 ads(0x48);
Adafruit_ADS1115 ads2(0x4A);
TFT_eSPI tft = TFT_eSPI();
//MAX6675 ktc(13, 10, 12);

#define CALIBRATION_FILE "/TouchCalData3"
#define REPEAT_CAL false
#define TFT_GREY 0x5AEB

float R1 = 110000.0, R2 = 11000.0, R3 = 110000.0, R4 = 11000.0;
float value1, value2, value3, value4, value5, value6, vout1, vout2, vout3;
float vin1, vin2, vmin1 = 20.0, vmin2 = 20.0, vmax1 = 0.0, vmax2 = 0.0, temp1, temp2, temp3;
double temp4;
int avg = 10, current, currentmax, power, powermax, rpt, screen;
int rptdelay;
bool main_filled = false;
bool settings_filled = false;

void setup()
{
  rpt = 999;
  while (!EEPROM.begin(4)) {
	true;
  }
  rptdelay = EEPROM.read(0);
  ads.begin();
  ads2.begin();
  digitalWrite(12, HIGH); // Touch controller chip select (if used)
  digitalWrite(14, HIGH); // TFT screen chip select
  digitalWrite(5, HIGH); // SD card chips select, must use GPIO 5 (ESP32 SS)
  tft.init();
  tft.setRotation(1);
  touch_calibrate();
  tft.setSwapBytes(true);
  fill_main_screen();
}

void loop()
{
	uint16_t x, y;

	if (screen == 0)
	{
		if (tft.getTouch(&x, &y))
		{
			if ((x > 10) && (x < (10 + 32)))
			{
				if ((y > 198) && (y <= (198 + 32)))
				{
					//tft.fillCircle(x, y, 2, TFT_WHITE);
					//ESP.restart();
					vmin1 = 20.0; vmax1 = 0.0; currentmax = 0; powermax = 0; rpt = 999;
				}
			}
			else if ((x > 278) && (x < (278 + 32)))
			{
				if ((y > 198) && (y <= (198 + 32)))
				{
					main_filled = false;
					screen = 1;
					return;
				}
			}
		}
		if (main_filled)
		{
			rpt++;
			main_screen();
		}
		else
		{
			fill_main_screen();
		}
	}
	else if (screen == 1)
	{
		if (tft.getTouch(&x, &y))
		{
			if ((x > 5) && (x < (5 + 38)))
			{
				if ((y > 5) && (y <= (5 + 32)))
				{
					settings_filled = false;
					rpt = 999;
					screen = 0;
					return;
				}
			}
			else if ((x > 170) && (x < (170 + 40)))
			{
				if ((y > 54) && (y <= (54 + 40)))
				{
					if (rptdelay > 0)
					{
						rptdelay--;
						redraw_delay();
					}
				}
			}
			else if ((x > 270) && (x < (270 + 40)))
			{
				if ((y > 54) && (y <= (54 + 40)))
				{
					if (rptdelay < 99)
					{
						rptdelay++;
						redraw_delay();
					}
				}
			}
			else if ((x > 220) && (x < (220 + 40)))
			{
				if ((y > 54) && (y <= (54 + 40)))
				{
					tft.setTextColor(TFT_GREEN, TFT_BLACK);
					redraw_delay();
					EEPROM.write(0, rptdelay);
					EEPROM.commit();
					delay(500);
					tft.setTextColor(TFT_CYAN, TFT_BLACK);
					redraw_delay();
				}
			}
		}
		if (!settings_filled)
		{
			fill_settings_screen();
		}
	}
}

void redraw_delay()
{
	tft.setTextPadding(tft.textWidth("88", 4));
	tft.drawNumber(rptdelay,226,75,4);
	tft.setTextPadding(0);
	delay(50);
}

void fill_settings_screen()
{
	tft.fillScreen(TFT_BLACK);
	tft.pushImage(5,5,38,32,back);
	tft.setTextColor(TFT_CYAN, TFT_BLACK);
	tft.drawString("USTAWIENIA",100,25,4);
	tft.drawRoundRect(170,54,40,40,10,TFT_WHITE);
	tft.drawRoundRect(270,54,40,40,10,TFT_WHITE);
	tft.drawString("Opoznienie",10,75,4);
	tft.drawString("-",186,75,4);
	tft.setTextPadding(tft.textWidth("88", 4));
	tft.drawNumber(rptdelay,226,75,4);
	tft.setTextPadding(0);
	tft.drawString("+",284,75,4);
	tft.drawLine(0,44,320,44,TFT_GREY);
	settings_filled = true;
}

void fill_main_screen()
{
	tft.fillScreen(TFT_BLACK);
	tft.pushImage(2,56,39,26,volts);
	tft.pushImage(5,5,32,30,temp);
	tft.pushImage(148,7,61,24,oil);
	tft.pushImage(5,101,30,32,music);
	tft.pushImage(10,198,32,32,reset);
	tft.pushImage(278,198,32,32,settings);
	tft.drawLine(0,46,320,46,TFT_GREY);
	tft.drawLine(0,93,320,93,TFT_GREY);
	tft.drawLine(0,141,320,141,TFT_GREY);
	main_filled = true;
}

void main_screen()
{
  if (rpt > rptdelay)
  {
	rpt = 0;
    int16_t adc0, adc1, adc2, adc3, adc20, adc21, adc22, adc23;
    adc0 = ads.readADC_SingleEnded(0);
    adc1 = ads.readADC_SingleEnded(1);
    //adc1 = ads.readADC_SingleEnded(2);
    adc20 = ads2.readADC_SingleEnded(0);
    adc21 = ads2.readADC_SingleEnded(1);
    adc22 = ads2.readADC_SingleEnded(2);
    value1 = adc0;
    //value2 = adc1;
    value3 = adc1;
    temp1 = (adc20 * 0.1875)/10;
    temp2 = (adc21 * 0.1875)/10;
    temp3 = (adc22 * 0.1875)/10;
    //temp4 = ktc.readCelsius();

  /*
    int16_t average1, average2, average3, average4, average5, average6;

    for (int i=0; i < avg; i++) {
	  average1 = average1 + ads.readADC_SingleEnded(0);
	  average2 = average2 + ads.readADC_SingleEnded(1);
	  average3 = average3 + ads.readADC_SingleEnded(2);
	  average4 = average4 + ads2.readADC_SingleEnded(0);
	  average5 = average5 + ads2.readADC_SingleEnded(1);
      average6 = average5 + ads2.readADC_SingleEnded(2);
    }

    value1 = average1/avg; value2 = average3/avg; value3 = average2/avg; value4 = average4/avg; value5 = average5/avg; value6 = average6/avg;
    temp1 = (value4 * 0.1875)/10; temp2 = (value5 * 0.1875)/10; temp3 = (value6 * 0.1875)/10;
  */

    vout1 = (value1 * 0.1875)/1000;
    vin1 = vout1 / (R2/(R1+R2));
    if (vin1 < 0.09) { vin1 = 0.0; }
    if (vin1 < vmin1) { vmin1 = vin1; }
    if (vin1 > vmax1) { vmax1 = vin1; }

  /*
    vout2 = (value2 * 0.1875)/1000;
    vin2 = vout2 / (R4/(R3+R4));
    if (vin2 < 0.09) { vin2 = 0.0; }
    if (vin2 < vmin2) { vmin2 = vin2; }
    if (vin2 > vmax2) { vmax2 = vin2; }
  */
    
    vout3 = (value3 * 0.1875)/1000;
    current = (vout3 - 2.528) / 0.02;
    if (current < 0.1) { current = 0; }
    if (current > currentmax) { currentmax = current; }
    power = vin1 * current / 2;
    if (power > powermax) { powermax = power; }
	
	tft.setTextColor(TFT_CYAN, TFT_BLACK);
	
    tft.setTextDatum(MR_DATUM);
    tft.setTextPadding(tft.textWidth("88.88", 2));
    tft.setTextFont(2);
    tft.drawFloat(vmin1,2,100,70,2);
    tft.setTextPadding(0);
    tft.setTextDatum(ML_DATUM);
    tft.drawString("V",100,70,2);
    
    tft.setTextDatum(MR_DATUM);
    tft.setTextPadding(tft.textWidth("88.88", 4));
    tft.drawFloat(vin1,2,200,73,4);
    tft.setTextPadding(0);
    tft.setTextDatum(ML_DATUM);
    tft.drawString("V",200,73,4);
    
    tft.setTextDatum(MR_DATUM);
    tft.setTextPadding(tft.textWidth("88.88", 2));
    tft.drawFloat(vmax1,2,300,70,2);
    tft.setTextPadding(0);
    tft.setTextDatum(ML_DATUM);
    tft.drawString("V",300,70,2);
    
    tft.setTextDatum(MR_DATUM);
    tft.setTextPadding(tft.textWidth("88", 4));
    tft.drawNumber(current,100,120,4);
    tft.setTextPadding(0);
    tft.setTextDatum(ML_DATUM);
    tft.drawString("A",100,120,4);
    
    tft.setTextDatum(MR_DATUM);
    tft.setTextPadding(tft.textWidth("88", 2));
    tft.drawNumber(currentmax,160,118,2);
    tft.setTextPadding(0);
    tft.setTextDatum(ML_DATUM);
    tft.drawString("A",160,118,2);

    tft.setTextDatum(MR_DATUM);
    tft.setTextPadding(tft.textWidth("888", 4));
    tft.drawNumber(power,234,120,4);
    tft.setTextPadding(0);
    tft.setTextDatum(ML_DATUM);
    tft.drawString("W",234,120,4);
    
    tft.setTextDatum(MR_DATUM);
    tft.setTextPadding(tft.textWidth("888", 2));
    tft.drawNumber(powermax,295,118,2);
    tft.setTextPadding(0);
    tft.setTextDatum(ML_DATUM);
    tft.drawString("W",295,118,2);

    tft.setTextDatum(MR_DATUM);
    tft.setTextPadding(tft.textWidth("888.8", 4));
    tft.drawFloat(temp1,1,105,23,4);
    tft.setTextPadding(0);
    tft.setTextDatum(ML_DATUM);
    tft.drawString("`C",105,23,4);
	
    tft.setTextDatum(MR_DATUM);
    tft.setTextPadding(tft.textWidth("888.8", 4));
    tft.drawFloat(temp2,1,280,23,4);
    tft.setTextPadding(0);
    tft.setTextDatum(ML_DATUM);
    tft.drawString("`C",280,23,4);
    
    tft.setTextDatum(MR_DATUM);
    tft.setTextPadding(tft.textWidth("888.8", 4));
    tft.drawFloat(temp3,1,178,215,4);
    tft.setTextPadding(0);
    tft.setTextDatum(ML_DATUM);
    tft.drawString("`C",178,215,4);
  }
}

void touch_calibrate()
{
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // check file system exists
  if (!SPIFFS.begin()) {
    Serial.println("Formating file system");
    SPIFFS.format();
    SPIFFS.begin();
  }

  // check if calibration file exists and size is correct
  if (SPIFFS.exists(CALIBRATION_FILE)) {
    if (REPEAT_CAL)
    {
      // Delete if we want to re-calibrate
      SPIFFS.remove(CALIBRATION_FILE);
    }
    else
    {
      File f = SPIFFS.open(CALIBRATION_FILE, "r");
      if (f) {
        if (f.readBytes((char *)calData, 14) == 14)
          calDataOK = 1;
        f.close();
      }
    }
  }

  if (calDataOK && !REPEAT_CAL) {
    // calibration data valid
    tft.setTouch(calData);
  } else {
    // data not valid so recalibrate
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.println("Touch corners as indicated");

    tft.setTextFont(1);
    tft.println();

    if (REPEAT_CAL) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.println("Set REPEAT_CAL to false to stop this running again!");
    }

    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Calibration complete!");

    // store data
    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f) {
      f.write((const unsigned char *)calData, 14);
      f.close();
    }
  }
}