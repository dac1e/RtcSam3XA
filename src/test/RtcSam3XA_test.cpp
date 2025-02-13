/*
  RtcSam3XA - Arduino libary for RtcSam3XA - builtin RTC Copyright (c)
  2024 Wolfgang Schmieder.  All right reserved.

  Contributors:
  - Wolfgang Schmieder

  Project home: https://github.com/dac1e/RcSwitchReceiver/

  This library is free software; you can redistribute it and/or modify it
  the terms of the GNU Lesser General Public License as under published
  by the Free Software Foundation; either version 3.0 of the License,
  or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
*/

#include <assert.h>
#include "../TM.h"
#include "../RtcSam3XA.h"
#include "../internal/core-sam-GapClose.h"
#include "RtcSam3XA_test.h"

#include "Arduino.h"

namespace {

const char* const sHrsMode[] = {"24hrs mode.", "12hrs mode."};

// Provide an example instance.
void makeCETdstBeginTime(TM& time, int second, int minute, int hour, int dst = -1) {
  // 2nd of March 2016 2:00:00h is an daylight savings begin in CET time zone.
  time.set(second, minute, hour, 27, 2,
      TM::make_tm_year(2016), dst);
}

// Provide an example instance.
void makeCETdstEndTime(TM& time, int second, int minute, int hour, int dst = 1 /* 1: still in dst */) {
  // 30th of October 2016 2:00:00h is an daylight savings end in CET time zone.
  time.set(second, minute, hour, 30, 9,
      TM::make_tm_year(2016), dst);
}

void logtime(Stream& stream, const TM& time, std::time_t localtime) {
  stream.print(time);
  stream.print(", "); stream.print(localtime);
  stream.print(", "); stream.println(time.tm_isdst);
}

void dumpTzInfo(Stream& stream) {
  __tzinfo_type *tz = __gettzinfo ();
  stream.print("__tznorth:"); stream.println(tz->__tznorth);
  for(size_t i = 0; i < 2; i++) {
    __tzrule_struct* tzrule = &tz->__tzrule[i];
    stream.print(i);
    stream.print(", ch:"); stream.print(tzrule->ch);
    stream.print(", d:"); stream.print(tzrule->d);
    stream.print(", m:"); stream.print(tzrule->m);
    stream.print(", n:"); stream.print(tzrule->n);
    stream.print(", s:"); stream.print(tzrule->s);
    stream.print(", offset:"); stream.println(tzrule->offset);
  }
}

uint32_t getHourMode( Rtc* pRtc )
{
    return pRtc->RTC_MR & 0x00000001;
}

} // anonymous namespace

namespace RtcSam3XA_test {

static void testBasicSetGet(Stream &log) {
  log.print("--- RtcSam3XA_test::"); log.println(__FUNCTION__);

  // Default contstructor: 1st of January 2000  00:00:00h
  TM stime;
  std::mktime(&stime); // Fix tm_yday, tm_wday and tm_isdst

#if RTC_MEASURE_ACKUPD
  uint32_t timestamp_setClock = millis();
#endif

  RtcSam3XA::clock.setTime(stime);
  log.println(stime);

  TM rtime;
  std::time_t localtime = RtcSam3XA::clock.getLocalTimeAndUTC(rtime);
  logtime(log, rtime, localtime);
  delay(100); // @100ms
  // Time hasn't immediately set. The RTC is set within RTC_SR_ACKUPD interrupt.
  // The latency is about 350msec.
  assert(rtime == stime);

  // After 1600 (100 + 1600) the time should be set and should be increased by one second.
  delay(1500);// @1600ms
  localtime = RtcSam3XA::clock.getLocalTimeAndUTC(rtime);
  logtime(log, rtime, localtime);
  delay(100); // @1700ms

  const std::time_t timestamp = std::mktime(&stime);
  assert(localtime == timestamp + 1);

#if RTC_MEASURE_ACKUPD
  // Measure the set clock - latency an print.
  uint32_t setClockLatency = RtcSam3XA::clock.getTimestampACKUPD() - timestamp_setClock;
  log.print("--- set clock latency: ");
  log.print(setClockLatency);
  log.println("ms");
#endif
}

static void testDstEntry(Stream& log) {
  log.print("--- RtcSam3XA_test::"); log.println(__FUNCTION__);
  delay(100); // @100ms

  constexpr int HOUR_START = 1;

  // Set RTC to 3 seconds before daylight savings starts
  TM stime;
  makeCETdstBeginTime(stime, 57, 59, HOUR_START);

  std::mktime(&stime);
  RtcSam3XA::clock.setTime(stime);
  log.println(stime);

  TM rtime;
  std::time_t localtime = RtcSam3XA::clock.getLocalTimeAndUTC(rtime);
  logtime(log, rtime, localtime);
  delay(100); // @100ms
  // Time hasn't immediately set. The RTC is set within RTC_SR_ACKUPD interrupt.
  // The latency is about 350msec.
  assert(rtime.tm_hour == HOUR_START);

  // After 1600 (100 + 1400) the time should be set and should be increased by one second.
  delay(1500); // @1600ms
  localtime = RtcSam3XA::clock.getLocalTimeAndUTC(rtime);
  logtime(log, rtime, localtime);
  delay(100);  // @1700ms
  assert(rtime.tm_hour == HOUR_START);

  delay(1900);  // @3600ms
  localtime = RtcSam3XA::clock.getLocalTimeAndUTC(rtime);
  logtime(log, rtime, localtime);
  delay(100); // @3700ms
  assert(rtime.tm_hour == HOUR_START + 2);
}

static void testDstExit(Stream& log) {
  log.print("--- RtcSam3XA_test::"); log.println(__FUNCTION__);
  delay(100); // @100ms

  constexpr int HOUR_START = 2;

  // Set RTC to 3 seconds before daylight savings starts
  TM stime;
  makeCETdstEndTime(stime, 57, 59, HOUR_START);

  std::mktime(&stime);
  RtcSam3XA::clock.setTime(stime);
  log.println(stime);

  TM rtime;
  std::time_t localtime = RtcSam3XA::clock.getLocalTimeAndUTC(rtime);
  logtime(log, rtime, localtime);
  delay(100); // @100ms
  // Time hasn't immediately set. The RTC is set within RTC_SR_ACKUPD interrupt.
  // The latency is about 350msec.
  assert(rtime.tm_hour == HOUR_START);

  // After 1600 (100 + 1500) the time should be set and should be increased by one second.
  delay(1500); // @1500ms
  localtime = RtcSam3XA::clock.getLocalTimeAndUTC(rtime);
  logtime(log, rtime, localtime);
  delay(100);  // @1700ms
  assert(rtime.tm_hour == HOUR_START);

  delay(1900);  // @3600ms
  localtime = RtcSam3XA::clock.getLocalTimeAndUTC(rtime);
  logtime(log, rtime, localtime);
  delay(100); // @3700ms
  assert(rtime.tm_hour == HOUR_START);
}

static void checkRTCisdst(TM stime, Sam3XA::RtcTime rtc) {
  std::time_t localtime = mktime(&stime);
  localtime_r(&localtime, &stime);
  rtc.set(stime);
  if(stime.tm_isdst) {
    Sam3XA::RtcTime buffer;
    const int isdst =  Sam3XA::RtcTime::isdst(buffer, rtc);
    assert(stime.tm_isdst == isdst);
  } else {
    Sam3XA::RtcTime buffer;
    const int isdst =  Sam3XA::RtcTime::isdst(rtc, buffer);
    assert(stime.tm_isdst == isdst);
  }

}

static void testRTCisdst(Stream& log) {
  log.print("--- RtcSam3XA_test::"); log.println(__FUNCTION__);
  delay(100); // @100ms

  Sam3XA::RtcTime rtc;
  TM stime;

  makeCETdstBeginTime(stime, 59, 59, 1);
  checkRTCisdst(stime, rtc);
  delay(100);

  makeCETdstBeginTime(stime, 00, 00, 2);
  checkRTCisdst(stime, rtc);
  delay(100);

  makeCETdstEndTime(stime, 59, 59, 1);
  checkRTCisdst(stime, rtc);
  delay(100);

  makeCETdstEndTime(stime, 00, 00, 2, 0);
  checkRTCisdst(stime, rtc);
  delay(100);
}

static void check12hourRepresentation(Stream& log, TM& stime, uint8_t expectedAMPM, uint8_t expectedHour) {
  std::mktime(&stime);

  RtcSam3XA::clock.setTime(stime);
  log.println(stime);

  TM rtime;
  std::time_t localtime = RtcSam3XA::clock.getLocalTimeAndUTC(rtime);
  logtime(log, rtime, localtime);
  delay(100); // @100ms
  // Time hasn't immediately set. The RTC is set within RTC_SR_ACKUPD interrupt.
  // The latency is about 350msec.
  assert(localtime == std::mktime(&stime));

  // After 1000 (100 + 900) the time should be set.
  delay(900);// @1000ms


  uint8_t hour; uint8_t AMPM;
  RTC_GetTimeAndDate(RTC, &AMPM, &hour,
      nullptr, nullptr, nullptr,
      nullptr, nullptr, nullptr);

  log.print("Hour: "); log.print(hour);
  log.println( (AMPM ? "PM" : "AM") );
  assert(hour == expectedHour);
  assert(AMPM == expectedAMPM);

  delay(100); // @700ms
}

static void test12hourRepresentation(Stream& log, TM& tm) {
  log.print("--- RtcSam3XA_test::"); log.println(__FUNCTION__);

  // Check midnight
  check12hourRepresentation(log, tm, 0, 12);

  // Check hours from 1:00AM..11:00 AM
  for(int hour = 1; hour < 12; hour++) {
    tm.set(0, 0, hour, 1, 0, TM::make_tm_year(2000), -1);
    check12hourRepresentation(log, tm, 0, hour);
  }

  // Check Noon
  tm.set(0, 0, 12, 1, 0, TM::make_tm_year(2000), -1);
  check12hourRepresentation(log, tm, 1, 12);

  // Check hours from 1:00PM..11:00 PM
  for(int hour = 13; hour < 24; hour++) {
    tm.set(0, 0, hour, 1, 0, TM::make_tm_year(2000), -1);
    check12hourRepresentation(log, tm, 1, hour-12);
  }
}

static void testAlarm(Stream& log, TM& stime, const RtcSam3XA_Alarm& salarm,
    uint32_t runtimeAfterSetByLocalTime) {
  log.print("--- RtcSam3XA_test::"); log.println(__FUNCTION__);

  // Default constructor: 1st of January 2000  00:00:00h
  std::mktime(&stime);
  RtcSam3XA::clock.setTime(stime);
  // After 500 the time should be set and should be increased by one second.
  delay(500);// @500ms
  runtimeAfterSetByLocalTime -= 500;

  TM rtime;
  RtcSam3XA::clock.getLocalTimeAndUTC(rtime);
  log.println(rtime);
  delay(500);// @1000ms
  runtimeAfterSetByLocalTime -= 500;

  RtcSam3XA::clock.setAlarm(salarm);

  RtcSam3XA_Alarm ralarm;
  RtcSam3XA::clock.getAlarm(ralarm);
  log.print("getAlarm() returned: ");
  log.println(ralarm);

  delay(runtimeAfterSetByLocalTime);

  log.print("Exiting "); log.print(__FUNCTION__); log.print(" @ ");
  RtcSam3XA::clock.getLocalTimeAndUTC(rtime);
  log.println(rtime);
  delay(100); // Additional delay just for log.print().

  RtcSam3XA::clock.clearAlarm();
}

class AlarmReceiver {
  Stream& mLog;

  size_t mExpectedAlarmsCount;
  const TM* mExpectedAlarms;
  size_t mExpectedAlarmIndex;

  static void alarmCallback(void* param) {
    static_cast<AlarmReceiver*>(param)->onAlarm();
  }
public:
  AlarmReceiver(Stream& log) : mLog(log), mExpectedAlarmsCount(0), mExpectedAlarms(nullptr), mExpectedAlarmIndex(0) {
    RtcSam3XA::clock.setAlarmCallback(alarmCallback, this);
  }

  /* Called within interrupt context */
  void onAlarm() {
    TM rtime;
    RtcSam3XA::clock.getLocalTimeAndUTC(rtime);
    mLog.print(__FUNCTION__);
    mLog.print(" @ ");
    mLog.println(rtime);
    if(mExpectedAlarmIndex < mExpectedAlarmsCount) {
      if( rtime == mExpectedAlarms[mExpectedAlarmIndex] ) {
        mExpectedAlarmIndex++;
        return;
      }
    }
    assert(false);
  }

  void setExpectedAlarms(size_t n, const TM* const expectedAlarms) {
    mExpectedAlarmsCount = n;
    mExpectedAlarms = expectedAlarms;
    mExpectedAlarmIndex = 0;
  }

  ~AlarmReceiver() {
    // All expected alarms must have appeared;
    assert(mExpectedAlarmIndex == mExpectedAlarmsCount);
    RtcSam3XA::clock.setAlarmCallback(nullptr, nullptr);
  }
};

static void testAlarm(Stream& log, TM& stime) {
  RtcSam3XA_Alarm salarm;
  salarm.setHour(0);
  salarm.setSecond(10);

  {
    AlarmReceiver alarmReceiver(log);
    TM expectedAlarms[2];

    expectedAlarms[0].tm_sec = 10;
    expectedAlarms[0].tm_min =  0;
    expectedAlarms[0].tm_mon =  stime.tm_mon;
    mktime(&expectedAlarms[0]);

    expectedAlarms[1].tm_sec = 10;
    expectedAlarms[1].tm_min =  1;
    expectedAlarms[1].tm_mon =  stime.tm_mon;
    mktime(&expectedAlarms[1]);

    alarmReceiver.setExpectedAlarms(2, expectedAlarms);

    testAlarm(log, stime, salarm, 75000 /* Give 75 seconds for the alarms to appear. */);
  }

  // Test alarm starting at 12:00h
  stime.tm_hour = 12;
  {
    AlarmReceiver alarmReceiver(log);
    alarmReceiver.setExpectedAlarms(0, nullptr);
    testAlarm(log, stime, salarm, 75000 /* Give 75 seconds for the alarms to appear. */);
  }
}

void test_toTimestamp(TM time) {
  Sam3XA::RtcTime rtcTime;
  rtcTime.set(time);
  std::time_t timeStamp = rtcTime.toTimeStamp();
  assert(mktime(&time) == timeStamp);
  Sam3XA::RtcTime rtcTime2;
  rtcTime2.set(timeStamp, true);
  TM time2;
  gmtime_r(&timeStamp, &time2);
  assert(rtcTime == rtcTime2);
}

void test_toTimeStamp(Stream& log) {
  log.print("--- RtcSam3XA_test::"); log.println(__FUNCTION__);
  RtcSam3XA::tzset(TZ::UTC);

  TM time;

  makeCETdstEndTime(time, 50, 59, 2, true);
  test_toTimestamp(time);

  makeCETdstBeginTime(time, 50, 59, 1, true);
  test_toTimestamp(time);
}

void run(Stream& log) {
  log.print("--- RtcSam3XA_test::"); log.println(__FUNCTION__);
  test_toTimeStamp(log);

  RtcSam3XA::clock.begin(TZ::CET);

  /* --- Run RTC in 24-hrs mode --- */
  {
    dumpTzInfo(log);
    delay(200);

    testRTCisdst(log);

    testBasicSetGet(log);
    testDstEntry(log);
    testDstExit(log);

    // Test 12 hour representation in RTC 24 hour mode.
    {
      TM tm;// Default constructor initializes Midnight 1.January 2000 that will put RTC into 24-hrs mode
      test12hourRepresentation(log, tm);
    }

    // Test alarm in 24 hrs mode.
    {
      TM tm; // Default constructor initializes Midnight 1.January 2000 that will put RTC into 24-hrs mode
      testAlarm(log, tm);
    }
  }

  /* --- Run RTC in 12-hrs mode --- */
  {
    // Test 12 hour representation in RTC 12 hour mode.
    {
      TM tm; // Default constructor initializes Midnight 1.January 2000
      tm.tm_mon = 3; // Choose April to get into daylight savings that will put RTC into 12-hrs mode.
      test12hourRepresentation(log, tm);
    }

    // Test alarm in 12 hrs mode.
    {
      TM tm; // Default constructor initializes Midnight 1.January 2000
      tm.tm_mon = 3; // Choose April to get into daylight savings that will put RTC into 12-hrs mode.
      testAlarm(log, tm);
    }
  }

  return;
}

static size_t i = 0;
static size_t j = 0;

void loop(Stream& log) {
  if(j < 800) {
    if(i  ==  0) {
      constexpr int HOUR_START = 1;
      TM stime;
      makeCETdstBeginTime(stime, 00, 59, HOUR_START);
      std::mktime(&stime);
      RtcSam3XA::clock.setTime(stime);
      log.print("Setting clock to dst time ");
      log.println(stime);
    }

    if(i == 200) {
      constexpr int HOUR_START = 2;
      // Set RTC to 3 seconds before daylight savings starts
      TM stime;
      makeCETdstEndTime(stime, 00, 59, HOUR_START);

      std::mktime(&stime);
      RtcSam3XA::clock.setTime(stime);
      log.print("Setting clock to std time ");
      log.println(stime);
    }

    {
      TM rtime;
      std::time_t localtime = RtcSam3XA::clock.getLocalTimeAndUTC(rtime);
      logtime(log, rtime, localtime);
    }

    i = (i+1) % 400;
    delay(999);

    if(++j == 800) {
      log.println("--- End of Test ---");
    }
  }
}

} // namespace RtcSam3XA_test
