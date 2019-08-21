#include <EEPROM.h>
#include <SPI.h>
#include <DMD.h>
#include <DS3231.h>
#include <TimerOne.h>
#include "SystemFont5x7.h"
//#include "Arial_black_16.h"
#include "Arial_Black_16_ISO_8859_1.h"
#include "System6x7.h"
#include "Wendy3x5.h"

#define DISPLAYS_ACROSS 5
#define DISPLAYS_DOWN 1

#define TINY_TEXT_LONGDISP  20000
#define BOLD_TEXT_LONGDISP  30000
#define SECOND_BLINK_TIME   500
#define DELAY_SPEED         30
#define MAIN_DISPLAY        0
#define SECOND_DISPLAY      1

DMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN);
DS3231  rtc(SDA, SCL);

Time t;

uint8_t sLength;
uint8_t mode = 0;
uint8_t cursorPos;
uint8_t centerTxtAdd  = 0;
uint8_t tinyTxtAdd    = 18;
uint8_t boldTxtAdd    = 78;
uint8_t tmrAdd        = 255;
uint32_t tinyTxtLongDisp = 20000;
uint32_t boldTxtLongDisp = 20000;

String stringBuff;
char StrToChr_Buff[10];
char date_dmdBuff[10];

long tempClockShift = 0;
long startTime      = 0;
long blinkTime      = 0;
long tempBlink      = 0;

boolean ret   = false;
boolean blank = false;
boolean isSet = false;

boolean timeStop    = false;
boolean newCtrTxt   = false;
boolean newTinyTxt  = false;
boolean newBoldTxt  = false;

String incomingStr;
char changeCtrTopTxt[14] = "RELOS LABS";
char changeTinyTxt[61]   = "6x7 Scrolling Text";
char changeBoldTxt[176]  = "Arial Bold Scrolling Text";

void ScanDMD()
{ 
  dmd.scanDisplayBySPI();
}

void setup(void)
{
  if (EEPROM[centerTxtAdd] == 0)
  {
    defaultCenterTxt();
  }
  if (EEPROM[tinyTxtAdd] == 0)
  {
    defaultTinyTxt();
  }
  if (EEPROM[boldTxtAdd] == 0)
  {
    defaultBoldTxt();
  }
  if (EEPROM[tmrAdd] == 0)
  {
    defaultTmr();
  }

  rtc.begin();
  Serial.begin(9600);
  getDataEEPROM();
  Timer1.initialize(5000);           //period in microseconds to call ScanDMD. Anything longer than 5000 (5ms) and you can see flicker.
  Timer1.attachInterrupt(ScanDMD);   //attach the Timer1 interrupt to ScanDMD which goes to dmd.scanDisplayBySPI()

  dmd.clearScreen( true );
  //rtc.setDOW(SATURDAY);          // Set Day-of-Week to SUNDAY
  //rtc.setTime(00, 46, 00);     // Set the time to 12:00:00 (24hr format)
  //rtc.setDate(15, 6, 2019);
  
  byte len = strlen(changeTinyTxt);
  tinyTxtLongDisp = 6210 + (DELAY_SPEED * (len * 7));
  
  Serial.println(len);
  Serial.println(tinyTxtLongDisp);
  
  displayClock();
  displayDate();
  cursorPosCalc();
}

void cursorPosCalc()
{
  byte len = strlen(changeCtrTopTxt);
  //uint16_t dispxl = len * 7;
  //cursorPos = 48 - dispxl;
  //cursorPos += 32;
  switch (len)
  {
    case 1:
      cursorPos = 78;
      break;
    case 2:
      cursorPos = 73;
      break;
    case 3:
      cursorPos = 70;
      break;
    case 4:
      cursorPos = 66;
      break;
    case 5:
      cursorPos = 63;
      break;
    case 6:
      cursorPos = 60;
      break;
    case 7:
      cursorPos = 56;
      break;
    case 8:
      cursorPos = 53;
      break;
    case 9:
      cursorPos = 49;
      break;
    case 10:
      cursorPos = 46;
      break;
    case 11:
      cursorPos = 42;
      break;
    case 12:
      cursorPos = 39;
      break;
    case 13:
      cursorPos = 35;
      break;
    case 14:
      cursorPos = 32;
      break;
  }
  Serial.print("ctr len : ");
  Serial.print(len);
  Serial.print("\t");
  Serial.print("Cursor pos : ");
  Serial.println(cursorPos);
}

void loop(void)
{
  dmd.selectFont(System6x7);
  dmd.drawString(cursorPos ,0 , changeCtrTopTxt, strlen(changeCtrTopTxt) , GRAPHICS_NORMAL);
  dmd.drawMarquee(changeTinyTxt, strlen(changeTinyTxt), 160, 8);
  mode = MAIN_DISPLAY;
  blinkTime = millis();
  while ((millis() - blinkTime) < (tinyTxtLongDisp * 2))
  {
    if (newTinyTxt)
    {
      newTinyTxt = false;
      for (byte i = 0; i < 160; i++)
      {
        dmd.drawMarquee(" ", 1, i, 8);
      }
      dmd.drawMarquee(changeTinyTxt, strlen(changeTinyTxt), 160, 8);
    }
    BTCheck();
    displayClock();
    if (millis() - tempClockShift >= 3000 && !blank)
    {
      blank = true;
      dmd.drawString(129, 0, "                              ", 5, GRAPHICS_NORMAL );
      displayTemp();
      tempClockShift = millis();
    }
    else if (millis() - tempClockShift >= 3000 && blank)
    {
      blank = false;
      dmd.drawString(129, 0, "                              ", 5, GRAPHICS_NORMAL );
      displayDate();
      tempClockShift = millis();
    }
    dmd.selectFont(System6x7);
    dmd.stepSplitMarquee(8, 15);
    delay(DELAY_SPEED);
  }
  mode = SECOND_DISPLAY;
  dmd.clearScreen(true);
  dmd.selectFont(Arial_Black_16_ISO_8859_1);
  dmd.drawMarquee(changeBoldTxt, strlen(changeBoldTxt), 160 - 1, 0);
  mode = SECOND_DISPLAY;
  blinkTime = millis();
  while ((millis() - blinkTime) < boldTxtLongDisp)
  {
    if (newBoldTxt)
    {
      newBoldTxt = false;
      dmd.clearScreen(true);
      dmd.drawMarquee(changeBoldTxt, strlen(changeBoldTxt), 160 - 1, 0);
    }
    BTCheck();
    dmd.stepMarquee(-1,0); 
    delay(DELAY_SPEED);
  }
  dmd.clearScreen(true);
}

String convertTime(int Num)
{
  if(Num < 10)
    return "0"+String(Num);
  else
    return String(Num);
}

void displayTemp()
{ 
  int temp = rtc.getTemp();
  stringBuff = convertTime(temp);
  sLength = stringBuff.length() + 1;
  StrToChr_Buff[sLength];
  stringBuff.toCharArray(StrToChr_Buff, sLength);  
  dmd.selectFont(System6x7);
  //dmd.clearScreen(true);
  dmd.drawString(128, 0, "T", 1, GRAPHICS_NORMAL );
  dmd.drawString(136, 0, ":", 1, GRAPHICS_NORMAL );
  dmd.drawString(141, 0, StrToChr_Buff, sLength, GRAPHICS_NORMAL );
  dmd.drawString(155, 0, "~", 1, GRAPHICS_NORMAL ); //draw degree character
}

void displayClock()
{
  t = rtc.getTime();
  uint8_t jam = t.hour;
  uint8_t menit = t.min;
  
  dmd.selectFont(System6x7);
  stringBuff = convertTime(jam);
  sLength = stringBuff.length() + 1;
  StrToChr_Buff[sLength];
  stringBuff.toCharArray(StrToChr_Buff, sLength);
  dmd.drawString(1, 0, StrToChr_Buff, sLength, GRAPHICS_NORMAL);

  if (millis() - startTime >= SECOND_BLINK_TIME && !timeStop)
  {
    timeStop = true;
    startTime = millis();
    dmd.drawString(15 ,0 , ":", 1 ,GRAPHICS_NORMAL);
  }
  else if (millis() - startTime >= SECOND_BLINK_TIME && timeStop)
  {
    timeStop = false;
    startTime = millis();
    dmd.drawString(15 ,0 , " ", 1 ,GRAPHICS_NORMAL);
  }
  // Display Menit
  stringBuff = convertTime(menit);
  sLength = stringBuff.length() +  1;
  StrToChr_Buff[sLength];
  stringBuff.toCharArray(StrToChr_Buff, sLength);
  dmd.drawString(18 ,0 ,StrToChr_Buff, sLength, GRAPHICS_NORMAL);
}

void displayDate()
{
  t = rtc.getTime();
  uint8_t tgl = t.date;
  uint8_t bulan = t.mon;
  uint16_t tahun = t.year;
  
  dmd.selectFont(Wendy3x5);
  stringBuff = convertTime(tgl);
  sLength = stringBuff.length() + 1;
  date_dmdBuff[sLength];
  stringBuff.toCharArray(date_dmdBuff, sLength);
  dmd.drawString(128, 2, date_dmdBuff, sLength, GRAPHICS_NORMAL );
  dmd.drawString(136, 2, "-", 1, GRAPHICS_NORMAL );

  stringBuff = convertTime(bulan);
  sLength = stringBuff.length() +  1;
  date_dmdBuff[sLength];
  stringBuff.toCharArray(date_dmdBuff, sLength);
  dmd.drawString(140 ,2 , date_dmdBuff, sLength, GRAPHICS_NORMAL);
  
  dmd.drawString(149, 2, "-", 1, GRAPHICS_NORMAL );

  stringBuff = convertTime(tahun - 2000);
  sLength = stringBuff.length() +  1;
  date_dmdBuff[sLength];
  stringBuff.toCharArray(date_dmdBuff, sLength);
  dmd.drawString(153 ,2 , date_dmdBuff, sLength, GRAPHICS_NORMAL);
}

void incomingStrBT()
{
  char stringBuff[255];
  int strLen = incomingStr.length() + 1;
  int strSign = strLen - 4;
  incomingStr.toCharArray(stringBuff, strLen);
  byte arrPointer = 0;
  char sign[4]; //clk or date
  for (byte x = strSign; x < strLen; x++)
  {
    sign[arrPointer++] = stringBuff[x];
    Serial.println(sign[x]);
  }
  Serial.println(stringBuff);
  Serial.print("Sign is : ");
  Serial.println(sign);
  
  if (strcmp(sign, "clk") == 0)
  {
    char changeHour[3];
    char changeMinute[3];
    for (byte i = 0; i < strLen - 6; i++)
    {
      changeHour[i] = stringBuff[i];
      //Serial.println(clockstringBuff[i]);
    }
    arrPointer = 0;
    for (byte i = 2; i < strLen - 4; i++)
    {
      changeMinute[arrPointer++] = stringBuff[i];
      //Serial.println(clockstringBuff[i]);
    }
    //Serial.print("Hour is : ");
    //Serial.println(changeHour);
    //Serial.print("Minute is : ");
    //Serial.println(changeMinute);
    int iHour = atoi(changeHour);
    int iMinute = atoi(changeMinute);
    //Serial.println(iHour);
    //Serial.println(iMinute);
    rtc.setTime(iHour, iMinute, 00);     // Set the time to 12:00:00 (24hr format)
  }
  else if (strcmp(sign, "dte") == 0)
  {
    char changeDate[3];
    char changeMon[3];
    char changeYear[5];
    for (byte i = 0; i < strLen - 10; i++)
    {
      changeDate[i] = stringBuff[i];
      //Serial.println(changeDate[arrPointer++]);
    }
    arrPointer = 0;
    for (byte i = 2; i < strLen - 8; i++)
    {
      changeMon[arrPointer++] = stringBuff[i];
      //Serial.println(changeMon[arrPointer++]);
    }
    arrPointer = 0;
    for (byte i = 4; i < strLen - 4; i++)
    {
      changeYear[arrPointer++] = stringBuff[i];
      //Serial.println(changeYear[arrPointer++]);
    }
    //Serial.print("Date is : ");
    //Serial.println(changeDate);
    //Serial.print("Month is : ");
    //Serial.println(changeMon);
    //Serial.print("Year is : ");
    //Serial.println(changeYear);
    
    int iDate = atoi(changeDate);
    int iMon = atoi(changeMon);
    int iYear = atoi(changeYear);
    //Serial.println(iDate);
    //Serial.println(iMon);
    //Serial.println(iYear);

    rtc.setDate(iDate, iMon, iYear);
  }
  else if (strcmp(sign, "tmr") == 0)
  {
    char tmr[3];
    for (byte i = 0; i < strLen - 4; i++)
    {
      tmr[i] = stringBuff[i];
    }
    boldTxtLongDisp = atoi(tmr);
    EEPROM.update(tmrAdd, boldTxtLongDisp);
    boldTxtLongDisp *= 1000;
    Serial.print(F("boldTxtLongDisp : "));
    Serial.println(boldTxtLongDisp);
  }
  else if (strcmp(sign, "tx1") == 0)
  {
    memset(changeCtrTopTxt, 0, sizeof(changeCtrTopTxt));
    byte len = strLen - 4;
    arrPointer = 0;
    for (byte i = 0; i < sizeof(changeCtrTopTxt) - 1; i++)
      EEPROM.update(centerTxtAdd + i, 0);
      
    for (byte i = 0; i < len; i++)
    {
      changeCtrTopTxt[arrPointer++] = stringBuff[i];
      EEPROM.update(centerTxtAdd + i, changeCtrTopTxt[i]);
    }
    cursorPosCalc();
    Serial.print("Ctr Top Txt is : ");
    Serial.println(changeCtrTopTxt);
    Serial.println("EEPROM Changed");
    if (mode == MAIN_DISPLAY)
    {
      dmd.clearScreen(true);
      dmd.selectFont(System6x7);
      dmd.drawString(cursorPos ,0 , changeCtrTopTxt, strlen(changeCtrTopTxt) , GRAPHICS_NORMAL);
    }
    newCtrTxt = true;
  }
  else if (strcmp(sign, "tx2") == 0)
  {
    memset(changeTinyTxt, 0, sizeof(changeTinyTxt));
    byte len = strLen - 4;
    arrPointer = 0;
    for (byte i = 0; i < sizeof(changeTinyTxt) - 1; i++)
      EEPROM.update(tinyTxtAdd + i, 0);
      
    for (byte i = 0; i < len; i++)
    {
      changeTinyTxt[arrPointer++] = stringBuff[i];
      EEPROM.update(tinyTxtAdd + i, changeTinyTxt[i]);
    }
    Serial.print("String Tiny is : ");
    Serial.println(changeTinyTxt);
    byte chrlen = strlen(changeTinyTxt) - 1;
    tinyTxtLongDisp = 6210 + (DELAY_SPEED * (chrlen * 7));
    newTinyTxt = true;
  }
  else if (strcmp(sign, "tx3") == 0)
  {
    memset(changeBoldTxt, 0, sizeof(changeBoldTxt));
    byte len = strLen - 4;
    arrPointer = 0;
    
    for (byte i = 0; i < sizeof(changeBoldTxt) - 1; i++)
      EEPROM.update(boldTxtAdd + i, 0);
      
    for (byte i = 0; i < len; i++)
    {
      changeBoldTxt[arrPointer++] = stringBuff[i];
      EEPROM.update(boldTxtAdd + i, changeBoldTxt[i]);
    }
    newBoldTxt = true;
    Serial.print("Bold Txt is : ");
    Serial.println(changeBoldTxt);
    Serial.println("EEPROM Changed");
  }
  else
  {
    Serial.println("Insert clock time, date or texts, stupid!!!");
  }
}

void BTCheck()
{
  while (Serial.available() > 0) 
  {
    incomingStr = Serial.readStringUntil('\n');
    isSet = true;
    delay(1000);
  }
  if (isSet)
  {
    isSet = false;
    incomingStrBT();
  }
  incomingStr = "";
}

void defaultCenterTxt()
{
  byte chrLen = sizeof(changeCtrTopTxt);
  for (byte i = 0; i < chrLen; i++)
  {
    EEPROM.write(centerTxtAdd + i, changeCtrTopTxt[i]);
  }
}

void defaultTinyTxt()
{
  byte chrLen = sizeof(changeTinyTxt);
  for (byte i = 0; i < chrLen; i++)
  {
    EEPROM.write(tinyTxtAdd + i, changeTinyTxt[i]);
  }
}
void defaultBoldTxt()
{
  byte chrLen = sizeof(changeBoldTxt);
  for (byte i = 0; i < chrLen; i++)
  {
    EEPROM.write(boldTxtAdd + i, changeBoldTxt[i]);
  }
}
void defaultTmr()
{
  boldTxtLongDisp /= 1000;
  EEPROM.write(tmrAdd, boldTxtLongDisp);
}
void getDataEEPROM()
{
  Serial.println("Getting data from EEPROM...");
  for (byte i = 0; i < sizeof(changeCtrTopTxt); i++)
  {
    changeCtrTopTxt[i] = EEPROM.read(centerTxtAdd + i);
  }
  Serial.print("changeCtrTopTxt : ");
  Serial.println(changeCtrTopTxt);
  for (byte i = 0; i < sizeof(changeTinyTxt); i++)
  {
    changeTinyTxt[i] = EEPROM.read(tinyTxtAdd + i);
  }
  Serial.print("changeTinyTxt : ");
  Serial.println(changeTinyTxt);
  for (byte i = 0; i < sizeof(changeBoldTxt); i++)
  {
    changeBoldTxt[i] = EEPROM.read(boldTxtAdd + i);
  }
  Serial.print("changeBoldTxt : ");
  Serial.println(changeBoldTxt);
  
  boldTxtLongDisp = EEPROM.read(tmrAdd) * 1000;
  Serial.print("boldTxtLongDisp : ");
  Serial.println(boldTxtLongDisp);
}
