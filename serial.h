/*
 *  Copyright (C) 1999-2015 Jens Thoms Toerring
 *
 *  This file is part of fsc2.
 *
 *  Fsc2 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3, or (at your option)
 *  any later version.
 *
 *  Fsc2 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#pragma once
#if ! defined SERIAL_HEADER
#define SERIAL_HEADER

#include <termios.h>


/* Definition of log levels allowed in calls of fsc2_serial_exp_init().
   Since they already may have been defined in the GPIB module only
   define them if they aren't already known */

#if ! defined LL_NONE
#define  LL_NONE  0    /* log nothing */
#endif
#if ! defined LL_ERR
#define  LL_ERR   1    /* log errors only */
#endif
#if ! defined LL_CE
#define  LL_CE    2    /* log function calls and function exits */
#endif
#if ! defined LL_ALL
#define  LL_ALL   3    /* log calls with parameters and function exits */
#endif


/* Defines for the parity used by devices */

#define NO_PARITY         0
#define ODD_PARITY        1
#define EVEN_PARITY       2


/* Routines to be used from modules */

int fsc2_request_serial_port( const char * /* dev_file */,
                              const char * /* dev_name */  );

struct termios * fsc2_serial_open( int /* sn       */,
                                   int /* flags    */  );

void fsc2_serial_close( int /* sn */ );

ssize_t fsc2_serial_write( int          /* sn             */,
                           const void * /* buf            */,
                           size_t       /* count          */,
                           long         /* us_wait        */,
                           bool         /* quit_on_signal */  );

ssize_t fsc2_serial_read( int          /* sn             */,
                          void *       /* buf            */,
                          size_t       /* count          */,
                          const char * /* term           */,
                          long         /* us_wait        */,
                          bool         /* quit_on_signal */  );

int fsc2_tcgetattr( int              /* sn        */,
                    struct termios * /* termios_p */  );

int fsc2_tcsetattr( int              /* sn               */,
                    int              /* optional_actions */,
                    struct termios * /* termios_p        */  );

int fsc2_tcsendbreak( int /* sn       */,
                      int /* duration */  );

int fsc2_tcdrain( int /* sn */ );

int fsc2_tcflush( int /* sn             */,
                  int /* queue_selector */  );

int fsc2_tcflow( int /* sn     */,
                 int /* action */ );


/* Routines for internal use only */

bool fsc2_serial_exp_init( int /* log_level */ );

void fsc2_serial_cleanup( void );

void fsc2_serial_final_reset( void );


#endif   /* ! SERIAL_HEADER */


/*
 * Local variables:
 * tags-file-name: "../TAGS"
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
