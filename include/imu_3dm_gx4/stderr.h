/* ----------------------------------------------------------------------

   Stderr header

   Modification History:
   DATE         AUTHOR  COMMENT
   22-Apr-2001  LLW      Created and written.
   13 Jan 2002  LLW      Modified for unix and win32 compile

---------------------------------------------------------------------- */
#ifndef STDERR_INC
#define STDERR_INC

#ifdef __WIN32__
  extern int stderr_printf( char * str, ...);
#else
  #define stderr_printf printf
#endif

#endif

 
