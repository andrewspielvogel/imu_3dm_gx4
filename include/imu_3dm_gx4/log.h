/* ----------------------------------------------------------------------

   Logging thread for ROV control system

   Modification History:
   DATE         AUTHOR  COMMENT
   14-JUL-2000  LLW      Created and written.
   22-Apr-2001  LLW      Ported to WIN32 for DVLNAV
   13-Apr-2002  LLW      Modified to allow multiple log files open at once
                         new functions to log non-dsl format data
                         new functionality to enable daily rather than
                         hourly file closing

   01-01-08     LLW      Added PUJA data log file

---------------------------------------------------------------------- */
#ifndef LOGGING_PROCESS_INC
#define LOGGING_PROCESS_INC

// ----------------------------------------------------------------------
// DEBUG FLAG:  Uncomment this and recompile to get verbosr debugging
// ----------------------------------------------------------------------
// #define DEBUG_LOGGING
// ----------------------------------------------------------------------
extern int    log_this_now_dsl_format(int log_fid, char * record_name, char * record_data);
extern int    log_this_now(int log_fid, char * string);
extern int    log_this_now(int log_fid, char * string, int len);

extern void   log_clean_string(char * str);
extern void   log_flush_and_close_log_files(void);
extern void   log_flush_and_close_and_delete_current_log_files(void);

extern char * log_get_filename(int log_fid);
extern int    log_get_log_status(int log_fid);
extern int    log_bytes_per_sec(void);
extern void   log_one_hertz_update(void);

#define LOG_MAX_NUM_LOG_FILES        3

#define LOG_FID_KVH_FORMAT           0
#define LOG_FID_MST_FORMAT           1
#define LOG_FID_MST_BINARY_FORMAT    2
static char CSV_SCIENCE_LABEL_STR[] = "Col 1 label, Col 2 label, ....";

// extra junk
#define LOG_FID_CSV_FORMAT           3
#define LOG_FID_IMAGE_FORMAT         4
#define LOG_FID_RDI_BINARY_FORMAT    5

#endif

