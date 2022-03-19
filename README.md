# SNTP_Time_sync
SNTP Time Synchronization implementation according ESP-IDF Programming Guide

To set the current time, you can use the POSIX functions settimeofday() and adjtime(). They are used internally in the lwIP SNTP library to set current time when a response from the NTP server is received. These functions can also be used separately from the lwIP SNTP library.

A function to use inside the lwIP SNTP library depends on a sync mode for system time. Use the function sntp_set_sync_mode() to set one of the following sync modes:

SNTP_SYNC_MODE_IMMED (default) updates system time immediately upon receiving a response from the SNTP server after using settimeofday().

SNTP_SYNC_MODE_SMOOTH updates time smoothly by gradually reducing time error using the function adjtime(). If the difference between the SNTP response time and system time is more than 35 minutes, update system time immediately by using settimeofday().

The lwIP SNTP library has API functions for setting a callback function for a certain event. You might need the following functions:

sntp_set_time_sync_notification_cb() - use it for setting a callback function that will notify of the time synchronization process

sntp_get_sync_status() and sntp_set_sync_status() - use it to get/set time synchronization status

# Timezones
To set local timezone, use the following POSIX functions:

Call setenv() to set the TZ environment variable to the correct value depending on the device location. The format of the time string is the same as described in the GNU libc documentation (although the implementation is different).

Call tzset() to update C library runtime data for the new time zone.

Once these steps are completed, call the standard C library function localtime(), and it will return correct local time taking into account the time zone offset and daylight saving time.

## Specifying the Time Zone with TZ
https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
In POSIX systems, a user can specify the time zone by means of the TZ environment variable. For information about how to set environment variables, see Environment Variables. The functions for accessing the time zone are declared in time.h.

You should not normally need to set TZ. If the system is configured properly, the default time zone will be correct. You might set TZ if you are using a computer over a network from a different time zone, and would like times reported to you in the time zone local to you, rather than what is local to the computer.

In POSIX.1 systems the value of the TZ variable can be in one of three formats. With the GNU C Library, the most common format is the last one, which can specify a selection from a large database of time zone information for many regions of the world. The first two formats are used to describe the time zone information directly, which is both more cumbersome and less precise. But the POSIX.1 standard only specifies the details of the first two formats, so it is good to be familiar with them in case you come across a POSIX.1 system that doesn’t support a time zone information database.
