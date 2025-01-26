/*
 * RtcSam3XA_Alarm.cpp
 *
 *  Created on: 25.01.2025
 *      Author: Wolfgang
 */

#include <assert.h>
#include <print.h>
#include "internal/core-sam-GapClose.h"
#include "internal/RtcTime.h"
#include "RtcSam3XA_Alarm.h"

static inline void fillAlarmFraction(uint8_t& v) {
  if(v == RtcSam3XA_Alarm::INVALID_VALUE) { v = 0; }
}

size_t RtcSam3XA_Alarm::printTo(Print &p) const {
  size_t result = 0;
  result += p.print("sec:"); result += printMember(p, second);
  result += p.print(" min:"); result += printMember(p, minute);
  result += p.print(" hour:"); result += printMember(p, hour);
  result += p.print(" day:"); result += printMember(p, day);
  result += p.print(" month:"); result += printMember(p, month);
  return result;
}

size_t RtcSam3XA_Alarm::printMember(Print &p, const uint8_t m) {
  if (m != INVALID_VALUE) {
    return p.print(m);
  }
  return p.print("--");
}

RtcSam3XA_Alarm RtcSam3XA_Alarm::gaps2zero() const {
  RtcSam3XA_Alarm result = *this;

  // Fill all less significant values below the highest
  // significant valid value with 0, if set to
  // INVALID_VALUE.
  if(month != RtcSam3XA_Alarm::INVALID_VALUE) {
    // Fill all lower significant values with 0 if not set
    fillAlarmFraction(result.day);
    fillAlarmFraction(result.hour);
    fillAlarmFraction(result.minute);
    fillAlarmFraction(result.second);
  } else if(day != RtcSam3XA_Alarm::INVALID_VALUE) {
    // Fill all lower significant values with 0 if not set
    fillAlarmFraction(result.hour);
    fillAlarmFraction(result.minute);
    fillAlarmFraction(result.second);
  } else if(day != RtcSam3XA_Alarm::INVALID_VALUE) {
    // Fill all lower significant values with 0 if not set
    fillAlarmFraction(result.minute);
    fillAlarmFraction(result.second);
  } else if(day != RtcSam3XA_Alarm::INVALID_VALUE) {
    // Fill all lower significant values with 0 if not set
    fillAlarmFraction(result.second);
  }
  return result;
}

static inline void emptyAlarmFraction(uint8_t& v) {
  if(v == 0) { v = RtcSam3XA_Alarm::INVALID_VALUE; }
}

RtcSam3XA_Alarm RtcSam3XA_Alarm::zero2gaps() const {
  RtcSam3XA_Alarm result = *this;

  // Fill all less significant values below the highest
  // significant valid value with INVALID_VALUE if
  // set to 0.
  if(month != RtcSam3XA_Alarm::INVALID_VALUE) {
    // Fill all lower significant values with 0 if not set
    emptyAlarmFraction(result.day);
    emptyAlarmFraction(result.hour);
    emptyAlarmFraction(result.minute);
    emptyAlarmFraction(result.second);
  } else if(day != RtcSam3XA_Alarm::INVALID_VALUE) {
    // Fill all lower significant values with 0 if not set
    emptyAlarmFraction(result.hour);
    emptyAlarmFraction(result.minute);
    emptyAlarmFraction(result.second);
  } else if(day != RtcSam3XA_Alarm::INVALID_VALUE) {
    // Fill all lower significant values with 0 if not set
    emptyAlarmFraction(result.minute);
    emptyAlarmFraction(result.second);
  } else if(day != RtcSam3XA_Alarm::INVALID_VALUE) {
    // Fill all lower significant values with 0 if not set
    emptyAlarmFraction(result.second);
  }
  return result;
}

RtcSam3XA_Alarm::RtcSam3XA_Alarm() :
    second(INVALID_VALUE), minute(INVALID_VALUE), hour(INVALID_VALUE), day(INVALID_VALUE), month(INVALID_VALUE) {
}

RtcSam3XA_Alarm::RtcSam3XA_Alarm(int tm_sec, int tm_min, int tm_hour, int tm_mday, int tm_mon)
  : second(tm_sec < 60 ? tm_sec : INVALID_VALUE), minute(tm_min < 60 ? tm_min : INVALID_VALUE)
  , hour(tm_hour < 24 ? tm_hour : INVALID_VALUE), day(tm_mday < 32 ? tm_mday : INVALID_VALUE)
  , month(tm_mon < 12 ? tm_mon+1 : INVALID_VALUE) {
}

void RtcSam3XA_Alarm::readFromRtc() {
  RTC_GetTimeAlarm(RTC, &hour, &minute, &second);
  RTC_GetDateAlarm(RTC, &month, &day);
}

