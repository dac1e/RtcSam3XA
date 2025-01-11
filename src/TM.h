/*
 * TM.h
 *
 *  Created on: 11.01.2025
 *      Author: Wolfgang
 */

#pragma once

#ifndef RTCSAM3XA_SRC_TM_H_
#define RTCSAM3XA_SRC_TM_H_

#include <ctime>

// Class that extends struct std::tm with constructors and convenient functions and enums.
class TM : public std::tm {
public:
  // Enumeration that matches the tm_month representation of the tm struct.
  enum MONTH : int {
    January = 0, February, March, April, May, June, July, August, September, October, December
  };

  // Enumeration that matches the tm_wday representation of the tm struct.
  enum DAY_OF_WEEK : int {
    SUNDAY = 0, MONDAY, TUESDAY, WEDNESDAY, THURSDAY, FRIDAY, SATURDAY,
  };

  // make tm::tm_month from TM_MONTH enum.
  static int make_tm_month(MONTH month) {return static_cast<int>(month);}

  // make tm::tm_year format from a year.
  static int make_tm_year(int ad_year /* anno domini year */) { return ad_year - 1900; }

  static DAY_OF_WEEK wday(const std::tm& time) {return static_cast<DAY_OF_WEEK>(time.tm_wday);}
  static MONTH month(const std::tm& time) {return static_cast<MONTH>(time.tm_mon);}

  DAY_OF_WEEK wday() {return wday(*this);}
  MONTH month() {return month(*this);}

  TM();

  TM(int tm_sec, int tm_min, int tm_hour, int tm_mday /* 1..31 */, MONTH mon, int ad_year /* anno domini year */, bool tm_isdst)
  {
    set(*this, tm_sec, tm_min, tm_hour, tm_mday, make_tm_month(mon), make_tm_year(ad_year), tm_isdst);
  }

  TM(int tm_sec, int tm_min, int tm_hour, int tm_mday /* 1..31 */, int tm_mon /* 0..11 */, int tm_year /* year since 1900 */, bool tm_isdst)
  {
    set(*this, tm_sec, tm_min, tm_hour, tm_mday , tm_mon , tm_year, tm_isdst);
  }

  void set(int tm_sec, int tm_min, int tm_hour, int tm_mday /* 1..31 */, MONTH mon, int ad_year /* anno domini year */, bool tm_isdst)
  {
    set(*this, tm_sec, tm_min, tm_hour, tm_mday, make_tm_month(mon), make_tm_year(ad_year), tm_isdst);
  };

  void set(int _sec, int tm_min, int tm_hour, int tm_mday /* 1..31 */, int tm_mon /* 0..11 */, int tm_year /* year since 1900 */, bool tm_isdst)
  {
    set(*this, _sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year, tm_isdst);
  };
private:
  static void set(std::tm& time, int tm_sec, int tm_min, int tm_hour,
      int tm_mday /* 1..31 */, int tm_mon /* 0..11 */,
      int tm_year /* year since 1900 */, bool tm_isdst);
};


#endif /* RTCSAM3XA_SRC_TM_H_ */