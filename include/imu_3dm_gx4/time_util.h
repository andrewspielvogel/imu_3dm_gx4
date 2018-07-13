/* ----------------------------------------------------------------------

   Time functions

   Modification History:
   DATE         AUTHOR  COMMENT
   23-JUL-2000  LLW     Created and written.
   22-Apr-2001  LLW     Modified for unix or win32 compile 

---------------------------------------------------------------------- */
#ifndef TIME_UTIL_INC
#define TIME_UTIL_INC

// ----------------------------------------------------------------------
// DEBUG FLAG:  Uncomment this and recompile to get verbosr debugging 
// ----------------------------------------------------------------------
// #define DEBUG_TIME
// ----------------------------------------------------------------------

typedef double    rov_time_t;

typedef struct
{
  int year;
  int month;
  int day;
  int hour;
  int min;
  int sec_int;
  int msec_int;
  double sec_double;
  double sec_today;
  double sec_rov_time; /* unix time - time since midnight GMT beginning Jan 1, 1970 */
} rov_time_struct_t;

extern rov_time_struct_t   rov_get_time_struct();
extern rov_time_struct_t   rov_get_time_struct(int time_mode);

extern rov_time_t          rov_get_time(void);
extern rov_time_t          rov_get_time(int time_mode);

extern double              rov_diff_time(rov_time_t t1, rov_time_t t0);
extern int                 rov_sprintf_dsl_time_string(char * str);

extern int                 rov_sprintf_hour_min_only(char * str, rov_time_t t);
extern int                 rov_sprintf_hour_min_sec_only(char * str, rov_time_t t);
extern int                 rov_sprintf_hour_min_sec_only(char * str, rov_time_t t);


extern int  rov_time_mode_set( int mode);
extern int  rov_time_mode_get( void );
extern void rov_time_set( double secs_since_1970 );
extern void rov_time_set( int year, int month, int day, int hour, int min, double sec );
extern rov_time_t rov_time_compute( int year, int month, int day, int hour, int min, double sec );


/* variables used for controlling time */
#define ROV_TIME_MODE_NORMAL 0  /* Normal time, use O/S time */
#define ROV_TIME_MODE_RENAV  1  /* fake time, use atrificial time */


#endif



