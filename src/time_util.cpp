/* ----------------------------------------------------------------------

   Time functions

   Modification History:
   DATE         AUTHOR  COMMENT
   23-JUL-2000  LLW     Created and written.
   22-Apr-2001  LLW     Modified for unix or win32 compile
   06-Jan-2004  LLW     Corrected several bugs which caused problems
                        when PC clock was not set to GMT
---------------------------------------------------------------------- */
#include <stdio.h>
#include <math.h>
#include <string.h>

#ifdef __WIN32__
  // win32 headers
  #include <time.h>
  #include <sys\timeb.h>
  #include <dos.h>

#else
  // unix headers
  #include <time.h>
  #include <sys/time.h>
  #include <unistd.h>
#endif

#include "time_util.h"		/* time utilities */

/* variables used for controlling time */
// #define ROV_TIME_MODE_SYSTEM 0  /* Normal time, use O/S time */
// #define ROV_TIME_MODE_RENAV  1  /* fake time, use atrificial time */

static int ROV_TIME_MODE = ROV_TIME_MODE_NORMAL;
static double  renav_time = {0.0};  /* seconds since 1970 */

/* ----------------------------------------------------------------------

   Modification History:
   DATE         AUTHOR  COMMENT
   11-Jan-2004  LLW      Created and written.

---------------------------------------------------------------------- */
int rov_time_mode_set( int mode)
{
  ROV_TIME_MODE = mode;

  return ROV_TIME_MODE;
}

/* ----------------------------------------------------------------------

   Modification History:
   DATE         AUTHOR  COMMENT
   11-Jan-2004  LLW      Created and written.

---------------------------------------------------------------------- */
int rov_time_mode_get( void )
{
  return ROV_TIME_MODE;
}


/* ----------------------------------------------------------------------

   Modification History:
   DATE         AUTHOR  COMMENT
   11-Jan-2004  LLW      Created and written.

---------------------------------------------------------------------- */
void rov_time_set( double secs_since_1970 )
{
  renav_time = secs_since_1970;
}

/* ----------------------------------------------------------------------

   Modification History:
   DATE         AUTHOR  COMMENT
   11-Jan-2004  LLW      Created and written.

---------------------------------------------------------------------- */
void rov_time_set( int year, int month, int day, int hour, int min, double sec )
{
  struct tm t;
  time_t    tt;

  // set up tm data structure
  t.tm_year = year - 1900;
  t.tm_mon  = month - 1;
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min  = min;
  t.tm_sec  = sec;
  t.tm_isdst = 0;

  // call mktime to compute time_t time integer seconds since 1970
  tt = mktime(&t);

  // compute sum of integer and fractional seconds since 1970
  renav_time = ((double) tt) +  fmod( sec, 1.0);

}

/* ----------------------------------------------------------------------

   Modification History:
   DATE         AUTHOR  COMMENT
   11-Jan-2004  LLW      Created and written.

---------------------------------------------------------------------- */
rov_time_t rov_time_compute( int year, int month, int day, int hour, int min, double sec )
{
  struct tm t;
  time_t    tt;
  rov_time_t rov_time;

  // set up tm data structure
  t.tm_year = year - 1900;
  t.tm_mon  = month - 1;
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min  = min;
  t.tm_sec  = sec;
  t.tm_isdst = 0;

  // call mktime to compute time_t time integer seconds since 1970
  tt = mktime(&t);

  // compute sum of integer and fractional seconds since 1970
  rov_time = ((double) tt) +  fmod( sec, 1.0);

  return rov_time;

}





/* ----------------------------------------------------------------------

   Modification History:
   DATE         AUTHOR  COMMENT
   23-JUL-2000  LLW      Created and written.

---------------------------------------------------------------------- */
int rov_sprintf_hour_min_only(char * str, rov_time_t t)
{

  // get secs today at surface
  double sec_today;
  double hr;
  double min;
  int    len;

  // compute secs
  sec_today = fmod(t, 3600.0 * 24.0);

  hr  = floor(sec_today / 3600.0);
  min = floor(fmod(sec_today, 3600.0) / 60.0);

  len = sprintf(str,"%02.0f:%02.0f",hr, min);

  return len;

}



/* ----------------------------------------------------------------------


   Modification History:
   DATE         AUTHOR  COMMENT
   23-JUL-2000  LLW      Created and written.

---------------------------------------------------------------------- */
int rov_sprintf_hour_min_sec_only(char * str, rov_time_t t)
{

  // get secs today at surface
  double sec_today;
  double hr;
  double min;
  double sec;
  int    len;

  // compute secs
  sec_today = fmod(t, 3600.0 * 24.0);

  hr  = floor(sec_today / 3600.0);
  min = floor(fmod(sec_today, 3600.0) / 60.0);
  sec = fmod(sec_today,60.0);

  len = sprintf(str,"%02.0f:%02.0f:%02.0f",hr, min, sec);

  return len;

}


/* ----------------------------------------------------------------------

   difference between two time values

   Modification History:
   DATE         AUTHOR  COMMENT
   23-JUL-2000  LLW      Created and written.

---------------------------------------------------------------------- */
double rov_diff_time(rov_time_t t1, rov_time_t t0)

{
  return (t1 - t0);
}


/* ----------------------------------------------------------------------

   get time (since 1970?) in seconds, LSD is microseconds, granularity is
   not known

   Modification History:
   DATE         AUTHOR  COMMENT
   23-JUL-2000  LLW     Created and written.
   22-Apr-2001  LLW     Modified for unix or win32 compile
   06-Jan-2004  LLW     Corrected bugs which caused problem if
                        WIN32 PC clock is not set to GMT time zone

---------------------------------------------------------------------- */
rov_time_t rov_get_time(void)
{

// ---------------------------------------------------------------------
// WIN32 code
// ---------------------------------------------------------------------
#ifdef __WIN32__

   rov_time_struct_t t;

   t = rov_get_time_struct();

   return t.sec_rov_time;


// ---------------------------------------------------------------------
// UNIX code
// ---------------------------------------------------------------------
#else
  struct timeval  tv;
  struct timezone tz;
  double t;

  time_t  current_time;
  struct tm  *tm;

  current_time = time(NULL);
  tm = gmtime(&current_time);

  gettimeofday(&tv, &tz);

  t = (((double)tv.tv_sec) + (((double)tv.tv_usec) *0.000001));

  // printf("TIME:  time=%ld sec, %ld usec, %f\n",tv.tv_sec, tv.tv_usec, t);

  return t;

#endif

}

/* ----------------------------------------------------------------------

   get time (since 1970?) in seconds, LSD is microseconds, granularity is
   not known

   Modification History:
   DATE         AUTHOR  COMMENT
   23-JUL-2000  LLW     Created and written.
   22-Apr-2001  LLW     Modified for unix or win32 compile
   06-Jan-2004  LLW     Corrected bugs which caused problem if
                        WIN32 PC clock is not set to GMT time zone

---------------------------------------------------------------------- */
rov_time_t rov_get_time(int time_mode)
{

// ---------------------------------------------------------------------
// WIN32 code
// ---------------------------------------------------------------------
#ifdef __WIN32__

   rov_time_struct_t t;

   t = rov_get_time_struct(time_mode);

   return t.sec_rov_time;


// ---------------------------------------------------------------------
// UNIX code
// ---------------------------------------------------------------------
#else
  struct timeval  tv;
  struct timezone tz;
  double t;

  time_t  current_time;
  struct tm  *tm;

  current_time = time(NULL);
  tm = gmtime(&current_time);

  gettimeofday(&tv, &tz);

  t = (((double)tv.tv_sec) + (((double)tv.tv_usec) *0.000001));

  // printf("TIME:  time=%ld sec, %ld usec, %f\n",tv.tv_sec, tv.tv_usec, t);

  return t;

#endif

}



/* ----------------------------------------------------------------------

   sprintfs dsl data time string based on computer system clock

   MODIFICATION HISTORY
   DATE         WHO             WHAT
   -----------  --------------  ----------------------------
   14 APR 1999  Louis Whitcomb  Created and Written
   22-Apr-2001  LLW     Modified for unix or win32 compile
   09 JAN 2004 LLW Modified to use time_util.cpp

   ---------------------------------------------------------------------- */
int rov_sprintf_dsl_time_string(char * str)
{

// ---------------------------------------------------------------------
// WIN32 code
// ---------------------------------------------------------------------
#ifdef __WIN32__

   // 09 JAN 2004 LLW Modified to use time_util.cpp
   rov_time_struct_t     now;

   int    num_chars;

   // 09 JAN 2004 LLW Modified to use time_util.cpp
   now = rov_get_time_struct();


   num_chars = sprintf(str,"%02d/%02d/%02d %02d:%02d:%02d.%03d",
        now.year,
        now.month,
        now.day,
        now.hour,
        now.min,
        now.sec_int,
        now.msec_int);

   num_chars = strlen(str);

   return num_chars;

// ---------------------------------------------------------------------
// UNIX code
// ---------------------------------------------------------------------
#else

   int    num_chars;

   // for min, sec
   double total_secs;
   double secs_in_today;
   // double day;
   // double hour;
   double min;
   double sec;

   // for date and hours
   struct tm  *tm;
   time_t current_time;

   // read gettimeofday() clock and compute min, and
   // sec with microsecond precision
   total_secs = rov_get_time();
   secs_in_today = fmod(total_secs,24.0*60.0*60.0);
   // hour = secs_in_today/3600.0;
   min  = fmod(secs_in_today/60.0,60.0);
   sec  = fmod(secs_in_today,60.0);

   // call time() and gmtime for hour and date
   current_time = time(NULL);
   tm = gmtime(&current_time);

   num_chars = sprintf(str,"%02d/%02d/%02d %02d:%02d:%8f",
		       (int) tm->tm_year%100,
		       (int) tm->tm_mon+1,
		       (int) tm->tm_mday,
		       (int) tm->tm_hour,
		       (int) min,
		       sec);


   return num_chars;

#endif

}



/* ----------------------------------------------------------------------

   gets a o/s independent time struct

   MODIFICATION HISTORY
   DATE         WHO             WHAT
   -----------  --------------  ----------------------------
   14 APR 1999  Louis Whitcomb  Created and Written
   06 Jan 2004  LLW             Revised to compute time properly
                                regardless of how the PC time zone
                                is set under WIN32
   ---------------------------------------------------------------------- */
rov_time_struct_t rov_get_time_struct(void)
{

// ---------------------------------------------------------------------
// WIN32 code
// ---------------------------------------------------------------------
#ifdef __WIN32__


   double total_secs;
   double secs_in_today;
   //  double day;
   double hour;
   double min;
   double sec;

   // time_t        time_time;
   struct tm     gmtime_time;
   struct timeb  ftime_time;

   // int tz, dl;
   // tz = _timezone;
   // dl = _daylight;

   rov_time_struct_t t;

   // get seconds since 1970
   // if in RENAV mode, use fake clock
   if (ROV_TIME_MODE == ROV_TIME_MODE_RENAV)
     {
       ftime_time.time      = floor(renav_time);
       ftime_time.millitm   = (fmod(renav_time,1.0) * 1000.0);
       ftime_time.timezone  =  0;
       ftime_time.dstflag   =  0;
     }
   else // default is to read system time from O/S
     {
       // get integer and fractional seconds since 1970 with ftime()
       ftime(&ftime_time);
     }

   // ------------------------------------------------------
   // BAD method uses seperate calls for secs ans millisecs,
   //     leavinf possibility of phase error
   //get integer secs since 1970 GMT with time()
   // time_time = time(NULL);
   // convert integer secs to a ymdhms structure with gmtime()
   // gmtime_time = *gmtime(&time_time);
   // ------------------------------------------------------

   // ------------------------------------------------------
   // GOOD method compues gmtime struct from ftime resiult,
   //      no chance of phase error
   // convert integer secs to a ymdhms structure with gmtime()
   gmtime_time = *gmtime(&ftime_time.time);
   // ------------------------------------------------------

   // assign results to data structures
   t.year         = gmtime_time.tm_year + 1900;
   t.month        = gmtime_time.tm_mon+1;
   t.day          = gmtime_time.tm_mday;

   t.hour         = gmtime_time.tm_hour;
   t.min          = gmtime_time.tm_min;
   t.sec_int      = gmtime_time.tm_sec;
   t.msec_int     = ftime_time.millitm;
   t.sec_double   = gmtime_time.tm_sec + (((double)ftime_time.millitm) / 1000.0);

   t.sec_today    = t.sec_double +
                    (60.0 * t.min) +
                    (3600.0* t.hour);

   t.sec_rov_time = ftime_time.time +  (((double)ftime_time.millitm) / 1000.0);

   return t;
#endif

}



/* ----------------------------------------------------------------------

   gets a o/s independent time struct

   MODIFICATION HISTORY
   DATE         WHO             WHAT
   -----------  --------------  ----------------------------
   14 APR 1999  Louis Whitcomb  Created and Written
   06 Jan 2004  LLW             Revised to compute time properly
                                regardless of how the PC time zone
                                is set under WIN32
   ---------------------------------------------------------------------- */
rov_time_struct_t rov_get_time_struct(int time_mode)
{

// ---------------------------------------------------------------------
// WIN32 code
// ---------------------------------------------------------------------
#ifdef __WIN32__


   double total_secs;
   double secs_in_today;
   //  double day;
   double hour;
   double min;
   double sec;

   // time_t        time_time;
   struct tm     gmtime_time;
   struct timeb  ftime_time;

   // int tz, dl;
   // tz = _timezone;
   // dl = _daylight;

   rov_time_struct_t t;

   // get seconds since 1970
   // if in RENAV mode, use fake clock
   if (time_mode == ROV_TIME_MODE_RENAV)
     {
       ftime_time.time      = floor(renav_time);
       ftime_time.millitm   = (fmod(renav_time,1.0) * 1000.0);
       ftime_time.timezone  =  0;
       ftime_time.dstflag   =  0;
     }
   else // default is to read system time from O/S
     {
       // get integer and fractional seconds since 1970 with ftime()
       ftime(&ftime_time);
     }

   // ------------------------------------------------------
   // BAD method uses seperate calls for secs ans millisecs,
   //     leavinf possibility of phase error
   //get integer secs since 1970 GMT with time()
   // time_time = time(NULL);
   // convert integer secs to a ymdhms structure with gmtime()
   // gmtime_time = *gmtime(&time_time);
   // ------------------------------------------------------

   // ------------------------------------------------------
   // GOOD method compues gmtime struct from ftime resiult,
   //      no chance of phase error
   // convert integer secs to a ymdhms structure with gmtime()
   gmtime_time = *gmtime(&ftime_time.time);
   // ------------------------------------------------------

   // assign results to data structures
   t.year         = gmtime_time.tm_year + 1900;
   t.month        = gmtime_time.tm_mon+1;
   t.day          = gmtime_time.tm_mday;

   t.hour         = gmtime_time.tm_hour;
   t.min          = gmtime_time.tm_min;
   t.sec_int      = gmtime_time.tm_sec;
   t.msec_int     = ftime_time.millitm;
   t.sec_double   = gmtime_time.tm_sec + (((double)ftime_time.millitm) / 1000.0);

   t.sec_today    = t.sec_double +
                    (60.0 * t.min) +
                    (3600.0* t.hour);

   t.sec_rov_time = ftime_time.time +  (((double)ftime_time.millitm) / 1000.0);

   return t;
#endif

}

