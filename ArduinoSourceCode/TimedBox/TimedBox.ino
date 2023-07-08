
// Code written for Teensy 3.2 with RTC oscillator. See also https://github.com/PaulStoffregen/Time
// Epaper Display Waveshare 2.9" 296 x 128 pixels
// each update takes about 8s and the current consumed during the update is 41mA
// during standby mode the current consumption is only 0.043mA

//#define DEBUG

#include "Arduino.h"
#include <Snooze.h>
#include <TimeLib.h>

// E-Paper Display
#include <SPI.h>
#include "epd2in9_V2.h"
#include "epdpaint.h"
#include "imagedata.h"

#define COLORED     0
#define UNCOLORED   1

unsigned char image[4800];                                    // buffer big enough for everything
Paint paint(image, 0, 0);                                     // width should be the multiple of 8 
Epd epd;                                                      // allocate Epaper Display

// Load Snooze drivers
SnoozeAlarm alarm;
SnoozeBlock config_teensy32(alarm);

//tmElements_t endTimeElements = {second(), minute(), hour(), weekday(), day(), month(), ( year()-1970) };
#ifdef DEBUG
  const tmElements_t endTimeElements = {0, 16, 14, dowInvalid, 16, 4, CalendarYrToTm(2023) };
#else
  const tmElements_t endTimeElements = {0, 0, 13, dowInvalid, 6, 11, CalendarYrToTm(2021) };
#endif
const time_t endTime = makeTime(endTimeElements);             // unsigned 32bit, overflow expected at 4'294'967'295
enum States { countdownDays, countdownHours, countupDays, countupYears};
States state = countdownDays;

const int doorPin = 20;

#ifndef DEBUG
// undefined reference to _write is an issue on Teensy 3.2 if the program has no usage of any Print class members
// https://forum.pjrc.com/threads/71420-Undefined-reference-to-_write
extern "C" {
  int _write( int handle, char *buf, int count )
  {
    return 0;
      //return Serial.write(buf, count);
  }
}
#endif

void setup() {
  setSyncProvider(getTeensy3Time);                            // set the Time library to use Teensy 3.0's RTC to keep time

  #ifdef DEBUG
    Serial.begin(115200);
    Serial.println("startup");

    Serial.println("DEBUG active");
    /*
    if (Serial.available()) {
      time_t t = processSyncMessage();
      if (t != 0) {
        setTime(t);
        Teensy3Clock.set(t);                                  // set the RTC
      }
    }
    */
    /*
    tmElements_t unixTimeNowElements = {0, 04, 14, dowThursday, 16, 4, ( 2023-1970) };  // second(), minute(), hour(), weekday(), day(), month(), ( year()-1970)
    time_t unixTimeNow = makeTime(unixTimeNowElements);
    setTime(unixTimeNow);
    Teensy3Clock.set(unixTimeNow);
    */
    //setTime(22,41,0,4,11,21);                                 // int hr,int min,int sec,int day, int month, int yr
  #endif

  pinMode(doorPin, OUTPUT);
  digitalWrite(doorPin, LOW);

  if (epd.Init() != 0) {
      #ifdef DEBUG
        Serial.print("e-Paper init failed");
      #endif
      return;
  }
  epd.ClearFrameMemory(0xFF);                                 // bit set = white, bit reset = black
  epd.DisplayFrame();
  epd.ClearFrameMemory(0xFF);                                 // bit set = white, bit reset = black
  epd.DisplayFrame();

}

void loop() {
  setSyncProvider(getTeensy3Time);                            // resync the Time Library with RTC after hibernate
  refreshDisplay();

  // Berechne die Zeit bis zum nächsten Display-Refresh
  uint8_t rhour = (hour(endTime)+24-hour())%24;
  uint8_t rminute = (minute(endTime)+60-minute())%60;
  uint8_t rsecond = (second(endTime)+60-second())%60;
  
  // Set RtcTimer, define next wake up
  #ifdef DEBUG
    Serial.println("sleep");
    delay(5000);
    /*
    if(state == countdownHours)                               // refresh every hour
    {
      alarm.setRtcTimer(0, 0, 20);                            // hour, min, sec
    }
    else {
      alarm.setRtcTimer(0, 0, 40);                            // hour, min, sec
    }
    Snooze.hibernate( config_teensy32 );
    */
  #else
    if(state == countdownHours)                               // refresh every hour
    {
      alarm.setRtcTimer(0, rminute, rsecond);                 // hour, min, sec
    }
    else {
      alarm.setRtcTimer(rhour, rminute, rsecond);             // hour, min, sec
    }
    Snooze.hibernate( config_teensy32 );
  #endif
}

/*
void digitalClockDisplay() {
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year()); 
  Serial.println(); 
}*/

void refreshDisplay() {
  char tempCharArray[20];
  static uint32_t daysleft = 0;
  static uint32_t hoursleft = 0;
  static uint32_t yearsleft = 0;
  
  epd.Init();                                                 // wake up Epaper from sleep mode
  epd.ClearFrameMemory(0xFF);
  paint.SetRotate(ROTATE_90);
  
  if(day() == 03 && month() == 06)                            // Announce Special Event
  {
    paintText(148, "Happy Birthday", "Name");                 // 148 is the center of the 296 wide display
  }
  else if(day() == 1 && month() == 1)                         // Happy New Year
  {
    paintText(148, "Happy New", "Year");
  }
  else {
    if(now() > endTime && state < countupDays) {              // if endtime is in the past
      state = countupDays;                                    // jump directly to state "countupDays"
    }
    else if(state == countdownDays) {
      daysleft = (uint32_t) (endTime - now()) / SECS_PER_DAY; // calculate daysleft
      if((uint32_t) (endTime - now()) < SECS_PER_DAY / 2 && daysleft > 0) daysleft--;            // round off if less than half a day
      if(daysleft <= 1) {
        state = countdownHours;
      } else {
        paintNumber(148, daysleft, "Tag", "Tage");            // 148 center of 296 wide display
      }
    }

    if(state == countdownHours) {
      hoursleft = (uint32_t) (endTime - now()) / SECS_PER_HOUR;       // remaining seconds divided by seconds and minutes gives remaining hours
      hoursleft += 1;                                         // round up by one, so countdown does not display 0 hours during the last hour.

      if((uint32_t) endTime - now() <= SECS_PER_HOUR / 2)     // if end goal is closer then 30min, open the door
      {
        epd.ClearFrameMemory(0x00);                           // bit set = white, bit reset = black
        epd.DisplayFrame();
        openBox();
        state = countupDays;
      } else {
        paintNumber(148, hoursleft, "Stunde", "Stunden");     // 148 center of 296 wide display
      }
    }

    if(state == countupDays) {
      daysleft = (uint32_t) (now() - endTime) / SECS_PER_DAY;
      if((uint32_t) (now() - endTime) < SECS_PER_DAY / 2 && daysleft > 0) daysleft--;             // round off
      if(daysleft >= 365) {
        state = countupYears;
      } else {
        paintNumber(148, daysleft, "Tag", "Tage");            // 148 center of 296 wide display
      }
    }

    if(state == countupYears) {
      unsigned int yearOfLastAnniversary = year(now()) - 1;   // assume last anniversary was last year
      if(endTimeElements.Month <= month(now()) && endTimeElements.Day <= day(now())) {            // correct value if last anniversary was this year
        yearOfLastAnniversary++;
      }

      tmElements_t lastAnnivElements = {endTimeElements.Second, endTimeElements.Minute, endTimeElements.Hour, dowInvalid, endTimeElements.Day, endTimeElements.Month, (uint8_t) CalendarYrToTm(yearOfLastAnniversary) };
      time_t lastAnnivTime = makeTime(lastAnnivElements);

      yearsleft = (uint32_t) yearOfLastAnniversary - tmYearToCalendar(endTimeElements.Year);
      daysleft = (uint32_t) ((now() - lastAnnivTime) / SECS_PER_DAY);
      //if((uint32_t) (now() - endTime) < SECS_PER_DAY / 2 && daysleft > 0) daysleft--;             // round off

      int yearwidth = getNumberWidth(48, yearsleft);          // font80 is 48px wide per digit, calculate width depending on amount of digits
      int daywidth = getNumberWidth(48, daysleft);
      int emptywidth = 296 - yearwidth - daywidth;            // calculate empty space on 296px wide epaper display
      paintNumber((emptywidth / 4) + (yearwidth / 2), yearsleft, "Jahr", "Jahre");        // paint left number first
      paintNumber((emptywidth * 3 / 4) + yearwidth + (daywidth / 2), daysleft, "Tag", "Tage");
      paintDivider((emptywidth / 2) + yearwidth, 6);          // display 296 x 128

      if(yearsleft == 0) {                                    // go back in case this state got reached to early
        state = countupDays;
      }
    }

    #ifdef DEBUG
      Serial.print("mode: "); Serial.println(state);
      Serial.print("days: "); Serial.println(daysleft);
      Serial.print("hours: "); Serial.println(hoursleft);
      Serial.print("years: "); Serial.println(yearsleft);
      Serial.print("endTime: "); Serial.println(endTime);
      Serial.print("timeNow: "); Serial.println(now());

      // paint date
      paint.SetWidth(16);
      paint.SetHeight(296);
      paint.Clear(COLORED);
      paint.DrawStringAt(125, 2, "DEBUG", &Font16, UNCOLORED); // start 2px lower from the top
      sprintf(tempCharArray, "%02d/%02d/%04d",  day(), month(), year());
      paint.DrawStringAt(0, 2, tempCharArray, &Font16, UNCOLORED);
      sprintf(tempCharArray, "%02d:%02d:%02d",  hour(), minute(), second());
      paint.DrawStringAt(205, 2, tempCharArray, &Font16, UNCOLORED);
      epd.SetFrameMemory(paint.GetImage(), 112, 0, paint.GetWidth(), paint.GetHeight());
    #endif
  }

  epd.DisplayFrame();
  epd.Sleep();                                                // put Epaper in sleep mode
  delay(1);
  digitalWrite(RST_PIN, LOW);                                 // shut down powersupply inside epaper display
  
  return;
}

int getNumberWidth(int charwidth, int number) {
  if(number < 0) { return 0; }
  int width = 1;                                              // count digits
  if(number > 10) width++;
  if(number > 100) width++;
  if(number > 1000) width++;

  width *= charwidth;                                         // get width in pixels
  if(width < 80) {                                            // min width = 5 small chars with each 16px width for subtitle
    width = 80;
  }
  return width;
}

void paintNumber(int center, int number, const char* unit1, const char* unitx) {
  if(center < 0 || number < 0 || number > 999) { return; }

  char tempCharArray[14];
  int numberstart = center - 72;                              // shift number in y direction (to the right)
  if(number < 100) numberstart = center - 48;
  if(number < 10) numberstart = center - 24;
  
  paint.SetWidth(80);
  paint.SetHeight(144);                                       // allows up to 3 digits (each 48px)
  sprintf(tempCharArray, "%d", number);
  paint.Clear(UNCOLORED);
  paint.DrawStringAt(0, 0, tempCharArray, &Font80, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 32, numberstart, paint.GetWidth(), paint.GetHeight());
  
  int textwidth = strlen(number == 1 ? unit1 : unitx) * 17;   // each char is approx 16.5px wide
  paint.SetWidth(24);
  paint.SetHeight(textwidth);                                 // width and height is flipped, because of display rotation
  paint.Clear(UNCOLORED);
  paint.DrawStringAt(0, 0, number == 1 ? unit1 : unitx, &Font24, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 0, center - (textwidth / 2), paint.GetWidth(), paint.GetHeight());
}

void paintText(int center, const char* upperText, const char* lowerText) {
  if(center < 0) { return; }

  int textwidth = strlen(upperText) * 17;                     // each char is approx 16.5px wide
  paint.SetWidth(24);
  paint.SetHeight(textwidth);                                 // allows up to 3 digits (each 48px)
  paint.Clear(UNCOLORED);
  paint.DrawStringAt(0, 0, upperText, &Font24, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 88, center - (textwidth / 2), paint.GetWidth(), paint.GetHeight());
  
  textwidth = strlen(lowerText) * 41;                         // each char is approx 41px wide
  paint.SetWidth(64);
  paint.SetHeight(textwidth);                                 // width and height is flipped, because of display rotation
  paint.Clear(UNCOLORED);
  paint.DrawStringAt(0, 0, lowerText, &Font64, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 16, center - (textwidth / 2), paint.GetWidth(), paint.GetHeight());
}

void paintDivider(int center, int width) {
  paint.SetWidth(128);
  paint.SetHeight(width);
  paint.Clear(UNCOLORED);
  paint.DrawFilledRectangle(0, 6, width, 128, COLORED);       // start line 6px below top, because of mechanical situation
  epd.SetFrameMemory(paint.GetImage(), 0, center - (width/2), paint.GetWidth(), paint.GetHeight());
}

void openBox() {
  pinMode(doorPin, OUTPUT);
  digitalWrite(doorPin, HIGH);
  delay(1000);
  digitalWrite(doorPin, LOW);
  delay(4000);
  digitalWrite(doorPin, HIGH);
  delay(1000);
  digitalWrite(doorPin, LOW);
}

time_t getTeensy3Time()
{
  return Teensy3Clock.get();
}

#ifdef DEBUG                                                  // code to process time sync messages from the serial port
  unsigned long processSyncMessage() {
    unsigned long pctime = 0L;
    const unsigned long DEFAULT_TIME = 1357041600;            // Jan 1 2013 
  
    if(Serial.find("T")) {                                    // Header tag for serial time sync message
       pctime = Serial.parseInt();
       return pctime;
       if( pctime < DEFAULT_TIME) {                           // check the value is a valid time (greater than Jan 1 2013)
         pctime = 0L;                                         // return 0 to indicate that the time is not valid
       }
    }
    return pctime;
  }
  
  void printDigits(int digits) {                              // utility function for digital clock display: prints preceding colon and leading 0
    Serial.print(":");
    if(digits < 10)
      Serial.print('0');
    Serial.print(digits);
  }
#endif
