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

#include <Arduino.h>
#include "RtcSam3XA.h"
#include "TM.h"

/*
 * Set the RTC to 1st of January 2000 00:00:00h. Read RTC time and
 * print on Serial.
 */

//The setup function is called once at startup of the sketch
void setup()
{
  Serial.begin(9600);

  // Set time zone to Central European Time.
  RtcSam3XA::clock.begin(TZ::CET);

  // Set time to 1st of January 2000 00:00:00h.
  Serial.println("**** 1st of January 2000 00:00:00h ****");
  TM time; // time is initialized with 1st of January 2000 00:00:00h by default constructor.
  RtcSam3XA::clock.setLocalTime(time);
}

// The loop function is called in an endless loop
void loop()
{
  // Print out the local time every second
  TM time;
  {
    /**
     * Read the local time and print it.
     * Then convert the local time to
     * UTC (Greenwich meantime) and
     * print it.
     */
    const std::time_t rawtime = RtcSam3XA::clock.getLocalTime(time);
    Serial.print("Local time: ");
    Serial.print(time);
    Serial.print(time.tm_isdst ? " Dayl. savg." : " Normal Time");

    TM utc;
    gmtime_r(&rawtime, &utc);
    Serial.print(", (UTC=");
    Serial.print(utc);
    Serial.println(')');
  }
  delay(1000);
}
