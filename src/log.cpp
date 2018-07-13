/* ----------------------------------------------------------------------

   Logging thread for ROV control system

   Modification History:
   DATE         AUTHOR  COMMENT
   14-JUL-2000  LLW      Created and written.
   22-Apr-2001  LLW      Ported to WIN32 for DVLNAV
   13-Apr-2002  LLW      Modified to allow multiple log files open at once.
   24 April 2002 LLW modified to close out fid 2 at midnight
   01 AUG 2002  LLW      Added check on fopen(), not clear why we need this.
                         Added second attempt to fclose if first fails.
    02 JUN 2005 LLW  Added logging of all targets at top of new CSV file

---------------------------------------------------------------------- */
/* standard ansi C header files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <time.h>

// #include <vcl/syncobjs.hpp>


/* jason header files */
// #include "jasontalk.h"		/* jasontalk protocol and structures */
// #include "dvlnav.h"

#include "log.h"      	        /* log utils */
#include "time_util.h"		/* time utils */
#include "stderr.h"		/* stderr print util */

// TCriticalSection * LogCritSec = NULL;

typedef struct {
  int                log_flag;  /* 0=disabled, 1=enabled */
  char             * log_file_name_suffix;
  char  	     log_file_name[512];
  FILE  	   * log_file_pointer;
  unsigned           log_file_bytes_written;
  unsigned           log_file_bytes_written_last;
  int                log_file_bytes_per_sec;
  double             log_file_bytes_per_sec_lowpass;
  int                log_file_last_hour_or_day;
} logging_t;


// static logging_t log = {PTHREAD_MUTEX_INITIALIZER,1};

static logging_t log[LOG_MAX_NUM_LOG_FILES+1] = {{1, (char *) "KVH"},
						 {1, (char *) "MST"},
						 {0, NULL}
};

static char * cfg_data_log_dir[LOG_MAX_NUM_LOG_FILES+1] = {(char *) "~/log",
							   (char *) "~/log",
							   NULL};

char * PNS_LOG_STRING[65535];
char * PAS_LOG_STRING[65535];

/* ---------------------------------------------------------------------- */
int log_bytes_per_sec(void)

  /*

   MODIFICATION HISTORY
   DATE         WHO             WHAT
   -----------  --------------  ----------------------------
   20 Mar 2001  Louis Whitcomb  Created and Written
   13-Apr-2002  LLW      Modified to allow multiple log files open at once.

   ---------------------------------------------------------------------- */
{
  int i;
  int bps = 0;

  for(i=0; i<LOG_MAX_NUM_LOG_FILES; i++)
    bps += log[i].log_file_bytes_per_sec_lowpass ;

  return bps;

}


/* ---------------------------------------------------------------------- */
void log_one_hertz_update(void)

  /*

   MODIFICATION HISTORY
   DATE         WHO             WHAT
   -----------  --------------  ----------------------------
   20 Mar 2001  Louis Whitcomb  Created and Written

   ---------------------------------------------------------------------- */
{
  int i;

  for(i=0; i<LOG_MAX_NUM_LOG_FILES; i++)
    {
      log[i].log_file_bytes_per_sec = log[i].log_file_bytes_written - log[i].log_file_bytes_written_last;

      log[i].log_file_bytes_per_sec_lowpass =
        (0.75 * log[i].log_file_bytes_per_sec_lowpass) +
        (0.25 * log[i].log_file_bytes_per_sec);

      log[i].log_file_bytes_written_last = log[i].log_file_bytes_written;
    }

}



/* ---------------------------------------------------------------------- */
char * log_get_filename(int log_fid)

  /*

   MODIFICATION HISTORY
   DATE         WHO             WHAT
   -----------  --------------  ----------------------------
   23 Jun 1999  Louis Whitcomb  Created and Written

   ---------------------------------------------------------------------- */
{

#define inrange(x,lo,hi) (((x)>=(lo)) && ((x)<=(hi)))
  char * str;

  if inrange(log_fid, 0, LOG_MAX_NUM_LOG_FILES-1)
	      str =  log[log_fid].log_file_name;
  else
    str = (char *) "Uninitialized";

  return str;
}


/* ---------------------------------------------------------------------- */
int log_get_log_status(int log_fid)

  /*

   MODIFICATION HISTORY
   DATE         WHO             WHAT
   -----------  --------------  ----------------------------
   23 Jun 1999  Louis Whitcomb  Created and Written

   ---------------------------------------------------------------------- */
{
  if inrange(log_fid, 0, LOG_MAX_NUM_LOG_FILES-1)
	      return ((log[log_fid].log_flag != 0) && (log[log_fid].log_file_pointer != NULL));
  else
    return 0;
}

/* ---------------------------------------------------------------------- */
static int log_open_log_file(int log_fid)

  /*
    Maintains the binary log file pointer. Opens a new fiel each hour.

    MODIFICATION HISTORY
    DATE         WHO             WHAT
    -----------  --------------  ----------------------------
    18 Apr 1999  Louis Whitcomb  Created and Written based on Dana's original write_dvl
    24 April 2002 LLW modified to close out fid 2 at midnight
    09 JAN 2004 LLW Modified to use time_util.cpp
    02 JUN 2005 LLW  Added logging of all targets at top of new CSV file

    ---------------------------------------------------------------------- */

{
  static int last_hour[12] = {-1,-1,-1,-1,-1};
  static int last_day[12]  = {-1,-1,-1,-1,-1};
  int        time_to_open_new_log_file =0;

  if (cfg_data_log_dir[log_fid][0] == 0)
    return -1;

  // enter critical section
  //  LogCritSec->Acquire();

  // 09 JAN 2004 LLW Modified to use time_util.cpp
  rov_time_struct_t     now;
  static char           filename[512];
  int status = 0;

  //  if we are not logging, place a notice in the filename string
  if(log[log_fid].log_flag == 0)
    {
      strcpy(log[log_fid].log_file_name,"LOGGING DISABLED");

      if(log[log_fid].log_file_pointer!=NULL)
	{
	  fclose(log[log_fid].log_file_pointer);
	  log[log_fid].log_file_pointer = NULL;
	}
    }

  // fflush the data file
  //   if(log[log_fid].log_file_pointer != NULL)
  //     fflush(log[log_fid].log_file_pointer);

  // 09 JAN 2004 LLW Modified to use time_util.cpp
  now = rov_get_time_struct();

  // 24 April 2002 LLW modified to close out fid 2 at midnight
  //  if((log_fid == LOG_FID_DSL_FORMAT) ||   // dsl .dat data
  //     (log_fid == LOG_FID_RDI_BINARY_FORMAT))
  //    {
  time_to_open_new_log_file = (now.hour != last_hour[log_fid]);
  if(time_to_open_new_log_file != 0)
    time_to_open_new_log_file = time_to_open_new_log_file;
  //    }

  //   if (log_fid == LOG_FID_CSV_FORMAT) .csv data
  //    {
  //      time_to_open_new_log_file = (now.day != last_day[log_fid]);
  //    }

  /* if log_flag is true */
  /* open a new file on first call, and at the top of the hour thereafter */
  if( (time_to_open_new_log_file || (log[log_fid].log_file_pointer==NULL)))
    //       && (log[log_fid].log_flag != 0))
    {

      last_hour[log_fid] = now.hour;
      last_day[log_fid]  = now.day;

      /* close existing log file */
      if(log[log_fid].log_file_pointer != NULL)
	{
	  if(0== fclose(log[log_fid].log_file_pointer))
            {
              log[log_fid].log_file_pointer = NULL;
	      stderr_printf("LOG: Closed system     log file %s OK.",log[log_fid].log_file_name);
            }
	  else  // try again, one more time
           {

	      stderr_printf("LOG: ERROR closing system     log file %s !!",log[log_fid].log_file_name);

              // try again to close the file
              if(log[log_fid].log_file_pointer != NULL)
		fclose(log[log_fid].log_file_pointer);
	    }
	}


      /* create the new log file name */
      sprintf(filename,"%s\\%04d_%02d_%02d_%02d_%02d.%s",
	      cfg_data_log_dir[log_fid],
	      now.year,
	      now.month,
	      now.day,
	      now.hour,
	      now.min,
	      log[log_fid].log_file_name_suffix
	      );

      /* open the new file */
      if(log[log_fid].log_file_pointer == NULL)
        {
          if( log_fid == LOG_FID_RDI_BINARY_FORMAT)
            log[log_fid].log_file_pointer = fopen(filename,"ab");
          else
            log[log_fid].log_file_pointer = fopen(filename,"at");
        }

      /* check results of fopen operation */
      if(log[log_fid].log_file_pointer == NULL)
	{
	  log[log_fid].log_flag = 0;
	  // stderr_printf("ERROR: Log file %s failed to open.",log[log_fid].log_file_name);

	  strcpy(log[log_fid].log_file_name, "ERROR: ");
	  strcat(log[log_fid].log_file_name, filename);
	  status = 1;

	}
      else
	{
	  strcpy(log[log_fid].log_file_name, filename);
	  stderr_printf("LOG: Opened system     log file %s OK.",log[log_fid].log_file_name);

	  // if we have opened a new spreadsheet file, log column labels
	  if( log_fid == LOG_FID_CSV_FORMAT)
            {
	      log_this_now( LOG_FID_CSV_FORMAT, CSV_SCIENCE_LABEL_STR);
            }
	}

    }

  //   LogCritSec->Acquire();

  return(status);

}

/* ---------------------------------------------------------------------- */
void log_clean_string(char * str)

  /*


   MODIFICATION HISTORY
   DATE         WHO             WHAT
   -----------  --------------  ----------------------------
   18 Apr 1999  Louis Whitcomb  Created and Written based on Dana's original write_dvl

   ---------------------------------------------------------------------- */
{

  while(*str != '\0')
    {
      if((*str == '\n') || (*str == '\r'))
	*str = ' ';
      str++;
    }

}




/* ---------------------------------------------------------------------- */
int log_this_now_dsl_format(int log_fid,
			    char * record_name,
			    char * record_data)

  /*


   MODIFICATION HISTORY
   DATE         WHO             WHAT
   -----------  --------------  ----------------------------
   18 Apr 1999  Louis Whitcomb  Created and Written based on Dana's original write_dvl

   ---------------------------------------------------------------------- */

{
  char dsl_date_time_str[128];
  int len;

  if (inrange(log_fid, 0, LOG_MAX_NUM_LOG_FILES-1)==0)
    return -1;

  // create win32 critical section object as required
  //  if(LogCritSec == NULL)
  //    LogCritSec = new TCriticalSection();

  // open a log file if required
  log_open_log_file(log_fid);

  // enter critical section
  // LogCritSec->Acquire();

  // if the user provides a header string, then use it and timestamp
  // otherwise, just log the string verbatim

  if(log[log_fid].log_flag && (log[log_fid].log_file_pointer != NULL))
    if(record_name != NULL)
      {
	// generate a date time string
	rov_sprintf_dsl_time_string(dsl_date_time_str);

	/* prepend record name and timestamp and write it to the log file */
	len = fprintf(log[log_fid].log_file_pointer,"%s %s %s\n", record_name, dsl_date_time_str, record_data);

	// update the stats
	log[log_fid].log_file_bytes_written += len;

      }
    else
      {
	/* write to file */
	len = fprintf(log[log_fid].log_file_pointer,"%s\n",record_data);

	// update the stats
	log[log_fid].log_file_bytes_written += len;

      }

  // leave critical section
  //   LogCritSec->Release();

  return 0;

}


/* ---------------------------------------------------------------------- */
int log_this_now(int log_fid, char * record_data)

  /*


   MODIFICATION HISTORY
   DATE         WHO             WHAT
   -----------  --------------  ----------------------------
   13 Apr 2002   Louis Whitcomb  Created and Written

   ---------------------------------------------------------------------- */

{
  char dsl_date_time_str[128];
  int len;

  if (inrange(log_fid, 0, LOG_MAX_NUM_LOG_FILES-1)==0)
    return -1;

  // create win32 critical section object as required
  //  if(LogCritSec == NULL)
  //    LogCritSec = new TCriticalSection();

  // open a log file if required
  log_open_log_file(log_fid);

  // enter critical section
  //  LogCritSec->Acquire();

  // if the user provides a header string, then use it and timestamp
  // otherwise, just log the string verbatim

  if(log[log_fid].log_flag && (log[log_fid].log_file_pointer != NULL))
    {
      /* write to file */
      len = fprintf(log[log_fid].log_file_pointer,"%s\n",record_data);

      // update the stats
      log[log_fid].log_file_bytes_written += len;

    }

  // leave critical section
  //  LogCritSec->Release();

  return 0;

}

/* ---------------------------------------------------------------------- */
int log_this_now(int log_fid, char * record_data, int len)

  /*


   MODIFICATION HISTORY
   DATE         WHO             WHAT
   -----------  --------------  ----------------------------
   13 Apr 2002   Louis Whitcomb  Created and Written
   07 DEC 2005   LLW             Created this version to accomodate binary data

   ---------------------------------------------------------------------- */

{
  int i;
  int bytes_written;
  unsigned char * data;

  // for debug
  data = (unsigned char *) record_data;

  if (inrange(log_fid, 0, LOG_MAX_NUM_LOG_FILES-1)==0)
    return -1;

  // create win32 critical section object as required
  //  if(LogCritSec == NULL)
  //    LogCritSec = new TCriticalSection();

  // open a log file if required
  log_open_log_file(log_fid);

  // enter critical section
  //  LogCritSec->Acquire();

  // just log the string verbatim

  if(log[log_fid].log_flag && (log[log_fid].log_file_pointer != NULL))
    {
      /* write to file */
      //for(i=0; i<len; i++)
      //  fputc(record_data[i], log[log_fid].log_file_pointer);

      bytes_written = fwrite(record_data, 1,  len,   log[log_fid].log_file_pointer);

      // for debug
      // fflush(log[log_fid].log_file_pointer );
      //fclose(log[log_fid].log_file_pointer );


      // update the stats
      log[log_fid].log_file_bytes_written += bytes_written;


    }

  // leave critical section
  //  LogCritSec->Release();

  return 0;

}



/* ---------------------------------------------------------------------- */
void log_flush_and_close_log_files(void)

  /*


   MODIFICATION HISTORY
   DATE         WHO             WHAT
   -----------  --------------  ----------------------------
   18 Apr 1999  Louis Whitcomb  Created and Written based on Dana's original write_dvl

   ---------------------------------------------------------------------- */

{

  int log_fid;

  for(log_fid=0; log_fid<LOG_MAX_NUM_LOG_FILES; log_fid++)
    {

      // create win32 critical section object as required
      //      if(LogCritSec == NULL)
      //	LogCritSec = new TCriticalSection();

      // enter critical section
      // LogCritSec->Acquire();

      // flush all files();
      //_flushall();

      if(log[log_fid].log_file_pointer != NULL)
	{
	  fflush(log[log_fid].log_file_pointer);
	  fclose(log[log_fid].log_file_pointer);
	  log[log_fid].log_file_pointer = NULL;
	}

      // close any remaining open files, e.g. stdout, stdin
      //_fcloseall();

      // leave critical section
      //      LogCritSec->Release();

      // deallocate the critical section
      //delete LogCritSec;

    }

}


/* ---------------------------------------------------------------------- */
void log_flush_and_close_and_delete_current_log_files(void)

  /*


   MODIFICATION HISTORY
   DATE         WHO             WHAT
   -----------  --------------  ----------------------------
   18 Apr 1999  Louis Whitcomb  Created and Written based on Dana's original write_dvl

   ---------------------------------------------------------------------- */

{

  int log_fid;

  for(log_fid=0; log_fid<LOG_MAX_NUM_LOG_FILES; log_fid++)
    {

      // create win32 critical section object as required
      //      if(LogCritSec == NULL)
      //	LogCritSec = new TCriticalSection();

      // enter critical section
      //      LogCritSec->Acquire();

      // flush all files();
      //_flushall();

      if(log[log_fid].log_file_pointer != NULL)
	{
          // flush
          fflush(log[log_fid].log_file_pointer);

          // close
	  fclose(log[log_fid].log_file_pointer);
	  log[log_fid].log_file_pointer = NULL;

          //delete current files
          remove(log[log_fid].log_file_name);
	}

      // close any remaining open files, e.g. stdout, stdin
      //_fcloseall();

      // leave critical section
      //      LogCritSec->Release();

      // deallocate the critical section
      //delete LogCritSec;

    }

}
