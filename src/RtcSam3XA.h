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

#pragma once

#ifndef RTCSAM3XA_SRC_RTCSAM3XA_H_
#define RTCSAM3XA_SRC_RTCSAM3XA_H_

#include <cstdlib>
#include <ctime>
#include <include/rtc.h>

#include "internal/RtcTime.h"
#include "RtcSam3XA_Alarm.h"

#ifndef RTC_MEASURE_ACKUPD
  #define RTC_MEASURE_ACKUPD false
#endif

/**
 * RtcSam3XA offers functions to operate the built in Real Time Clock
 * (RTC) and it's alarm features.
 * The RTC is represented as a single object named RtcSam3XA::clock.
 *
 * The standard structure std::tm and the standard type std::time_t
 * are used to operate the RTC.
 * There is the class TM that is derived from std::tm that enhances
 * structure std::tm with some convenience functions for std::tm.
 *
 * The class RtcSam3XA_Alarm is used to operate the alarm features
 * of the RTC.
 */
class RtcSam3XA {
public:
  /**
   * The static object RtcSam3XA::clock is the one and only clock object.
   *
   * Usage examples:
   *
   * include "TM.H
   *
   * // Read the local time and date from RTC.
   * TM time;
   * RtcSam3XA::clock.getLocalTime(time);
   *
   * // Write the local time and date to the RTC.
   * const TM time {24, 59, 11, 12, TM::make_tm_month(2), TM::make_tm_year(2016), false};
   * RtcSam3XA::clock.setLocalTime(time);
   */
  static RtcSam3XA clock;

  /**
   * Set time zone.
   *
   * Example 1 using predefined time zone string (see bottom of this file.):
   *  RtcSam3X::clock.tzset(TZ::CET);
   *
   * Example 2 using custom time zone string:
   *  (refer to https://man7.org/linux/man-pages/man3/tzset.3.html)
   *  RtcSam3X´::clock.tzset("CT+6:00:00+5:00:00,M3.2.0/2,M11.1.0/3");
   *
   * Setting the time zone is required for the correct collaboration of
   * the standard C++ entities std::tm, std::time_t, mktime(), gmtime(),
   * gmtime_r(), localtime(), localtime_r() along with the this RtcSam3XA class.
   * It is also needed for determining the daylight savings (also referred
   * to as dst) begin and end.
   *
   * Note: When the time zone changes, the local time and the alarms
   * still operate local.
   *
   * E.g. The local time is 15:00h and there is an alarm set at 16:00h.
   * After the time zone has changed, the local time within the new time
   * zone is still 15:00h and the alarm time will still appear at 16:00h
   * within the new time zone.
   *
   * @param timezone Refer to https://man7.org/linux/man-pages/man3/tzset.3.html
   *  for the explanation of the timezone string format.
   *
   * RTC alarms will be adjusted according to actual daylight savings
   * condition. That means that the RTC must hold the local daylight saving
   * time during the daylight saving time period. It must hold the
   * local standard time outside of the daylight savings period.
   * Hence the RTC will jump X hours forward when entering the
   * daylight savings period and X hours backward when leaving the daylight
   * savings condition. X is evaluated from the actual time zone information
   * (typically X is 1 hour.)
   * How to keep track of whether the RTC already jumped forward or
   * backward so that another jump woun't be applied upon a CPU reset?
   * This is done by using the 12-hrs mode of the RTC.
   * The decision was taken to let the RTC run in 12-hrs mode within the
   * daylight savings period and in the 24-hrs mode outside of it.
   *
   * Hence, the jump information isn't lost upon a CPU power fail, while
   * the RTC is powered by a backup battery.
   * There is also no problem reading a 24-hrs format from the RTC that
   * is running in a 12-hrs mode and vice versa, because it will be converted
   * via software. The same is valid for writing to the RTC.
   */
  static void tzset(const char* timezone) {
    setenv("TZ", timezone, true);
  }

  /**
   * Start RTC and optionally set time zone.
   *
   * @param timezone See description function tzset().
   * @param irqPrio RTC interrupt priority. [0..15] 0 is highest, 15 is lowest.
   * @param source The type of oscillator to be used for the clock.
   *
   */
  enum RTC_OSCILLATOR {RC = 0, XTAL = 1};
  void begin(const char* timezone = nullptr, const uint8_t irqPrio = 8,
      const RTC_OSCILLATOR source = RTC_OSCILLATOR::XTAL );

  /**
   * Set the RTC by passing the local time as std::tm struct.
   *
   * Note: The RTC does not support dates before 1st of January 2000.
   * Hence the tm_year that is holding the elapsed years since
   * 1900 must be greater than 100.
   *
   * @param localTime The local time.
   *    Note: time.tm_yday and time.wday fields may be random and
   *    tm_isdst (the daylight savings flag) can be set to -1. This is
   *    because all these fields will anyway be fixed before time is
   *    used to set the RTC. Fixing is performed by calling
   *    mktime(). This function uses the time zone information for
   *    the tm_isdst calculation. However, there is one situation
   *    when mktime() has to solve an ambiguity. This is when
   *    setting the time to the day and the hour, when time switches
   *    back from daylight savings to standard time. Typically that
   *    happens between 2:00h and 3:00h.
   *    So when setting the time to e.g. 2:30h at that day, the
   *    calculation assumes that you mean standard time 2:30h (after
   *    the hour has been switched back). If you mean 2:30h at daylight
   *    saving time (before the hour has been switched back), you have
   *    to set tm_dst to 1.
   *
   * @return true if successful. false, if date is lower than 1st of
   *    January 2000.
   *
   */
  bool setTime(const std::tm &localTime);

  /**
   * Set the RTC time by passing a UTC time stamp. Prerequisite:
   * time zone is set correctly.
   *
   * @param utcTimestamp UTC time.
   *
    * @return true if successful. false, if date is lower than 1st of
   *    January 2000.
   */
  bool setTime(std::time_t utcTimestamp);

  /**
   * Get the local time. Prerequisite: time zone is set correctly.
   *
   * @param[out] time The variable that will receive the local time.
   */
  void getLocalTime(std::tm &time) const {getLocalTimeAndUTC(time);}

  /**
   * Get the Unix time stamp from the RTC. Prerequisite: time zone
   * is set correctly.
   *
   * @return The UTC time (Greenwhich meantime).
   */
  time_t getUTC() const {tm time; return getLocalTimeAndUTC(time);}

  /**
   * Get the local time and UTC from the RTC.
   *
   * @param[out] time The variable that will receive the local time.
   *
   * @return The UTC time (Greenwhich meantime).
   */
  std::time_t getLocalTimeAndUTC(std::tm &time) const;

  /**
   * Set alarm time and date.
   *
   * @param alarm The alarm time and alarm date, to be set.
   */
  void setAlarm(const RtcSam3XA_Alarm& alarm);

  /**
   * Get the current RTC alarm time and date.
   *
   * @param alarm Reference to the variable receiving the alarm time
   *              and date.
   */
  void getAlarm(RtcSam3XA_Alarm &alarm);

  /**
   * Delete alarm settings.
   */
  void clearAlarm(){setAlarm(RtcSam3XA_Alarm());}

  /**
   * Set the callback to be called upon RTC alarm.
   *
   * @param alarmCallback The function to be called upon alarm.
   * @param alarmCallbackParam This parameter will be passed
   *  to the alarmCallback function.
   */
  void setAlarmCallback(void (*alarmCallback)(void* alarmCallbackParam),
      void *alarmCallbackParam = nullptr);

  /**
   * Set the callback being called upon every RTC second transition.
   *
   * @param secondCallback The function to be called upon second
   *  transition.
   * @param alarmCallbackParam This parameter will be passed
   *  to the secondCallback function.
   */
  void setSecondCallback(void (*secondCallback)(void*), void *secondCallbackParam = nullptr);

private:
  RtcSam3XA();
  friend void ::RTC_Handler();

  inline void RtcSam3XA_Handler();
  inline void RtcSam3XA_DstChecker();
  inline void RtcSam3XA_AckUpdHandler();

  enum SET_TIME_REQUEST {NO_REQUEST = 0, REQUEST, DST_RTC_REQUEST};
  volatile SET_TIME_REQUEST mSetTimeRequest;
  Sam3XA::RtcTime mSetTimeCache;

  void(*mSecondCallback)(void*);
  void* mSecondCallbackPararm;

  void(*mAlarmCallback)(void*);
  void* mAlarmCallbackPararm;

#if RTC_MEASURE_ACKUPD
  uint32_t mTimestampACKUPD;

public:
  uint32_t getTimestampACKUPD() const {
    return mTimestampACKUPD;
  }

#endif
};

namespace TZ {
  /**
   * Some predefined time zone strings for convenience.
   */
  constexpr char* UTC = "UTC+0:00:00"; // (Coordinated Universal Time)

  // Daylight savings start in the last week of march at 2:00h and ends in the last week of October at 3:00h
  constexpr char* UK  = "UK+0:00:00UKDST-1:00:00,M3.5.0/2,M10.5.0/3";   // (United Kingdom)
  constexpr char* CET = "CET-1:00:00CETDST-2:00:00,M3.5.0/2,M10.5.0/3"; // (Central Europe)

  // Daylight savings start in the second week of march at 2:00h and ends in the first week of November at 3:00h
  constexpr char* EST= "EST+5:00:00";                                // (Eastern Standard Time)
  constexpr char* ET = "ET+5:00:00ETDST+4:00:00,M3.2.0/2,M11.1.0/3"; // (Eastern Time)
  constexpr char* CST= "CST+6:00:00";                                // (Central Standard Time)
  constexpr char* CT = "CT+6:00:00+5:00:00,M3.2.0/2,M11.1.0/3";      // (Central Time)
  constexpr char* MST= "MST+7:00:00";                                // (Mountain Standard Time)
  constexpr char* MT = "MST+7:00:00+6:00:00,M3.2.0/2,M11.1.0/3";     // (Mountain Time)
  constexpr char* PST= "PST+8:00:00";                                // (Pacific Standard Time)
  constexpr char* PT = "PST+8:00:00+7:00:00,M3.2.0/2,M11.1.0/3";     // (Pacific Time)
}

#endif /* RTCSAM3XA_SRC_RTCSAM3XA_H_ */
