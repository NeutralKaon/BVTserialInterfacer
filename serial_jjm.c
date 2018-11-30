#include "serial_jjm.h" 


/*------------------------------------------------------------------*
 *  Spit out an error message on communication failure
 *------------------------------------------------------------------*/

void         bvt3000_comm_fail( ) { 
    fprintf(stderr,"Communication failure!\n"); 
}

/*------------------------------------------------------------------*
 * If make debug with -DDEBUG, spit out calling info. NB: the order 
 * of these functions matters
 *------------------------------------------------------------------*/


void bvt3000_comm_fail_debug( char const * caller_name ) { 
    if(verboseFlag){
        fprintf(stderr, "Communication fail was called from %s", caller_name );
    }
    bvt3000_comm_fail();
}

#ifdef DEBUG
    #define bvt3000_comm_fail() bvt3000_comm_fail_debug(__func__) 
#endif 
/* This is a moderately dodgy hack to provide a "caller" trace in debug 
 * mode without having to use GDB / examine the stack properly. 
 *
 * The syntax is essentially as follows: 
 * void a() {
 *  real_function_code(); 
 *  } 
 *  
 *  void a_special_for_debug(char const *caller_name)
 *  { printf("A called from %s\n", caller_name); 
 *  }
 *
 *  #ifdef DEBUG 
 *  #define a() a_special_for_debug(__func__) 
 *  #endif
 *
 *  void b() { 
 *      a(); 
 *      }
 */



char * bvt3000_query( const char * cmd, struct sp_port* port_choice )
{ 
	static char buf[ 100 ];
	unsigned char bc;
	ssize_t len;

	assert( cmd[ 2 ] == '\0' );

	/* Assemble string to be send */

	len = sprintf( buf, "%c%02d%02d%s%c", EOT, GROUP_ID, DEVICE_ID, cmd, ENQ );
    
    if (verboseFlag) { 
        printf("Query string: 0x'"); 
        for (int i=0; i<len; i++) { 
            printf("%02x",buf[i]);
        } 
        printf("' \n"); 
    }

	/* Send string and read and analyze response. The response must start
	   with STX, followed by the 2-char command, then data and finally an
	   ETX and the BCC (block check character) gets send. */

    enum sp_return error = sp_blocking_write(port_choice, &buf, len, SERIAL_WAIT); 

    if (error <0 || sp_drain(port_choice) ) { 
        fprintf(stderr, "Error writing to serial port: error is %d\n",error); 
        bvt3000_comm_fail( );
    }

    len = sp_blocking_read(port_choice, &buf, sizeof buf -1, SERIAL_WAIT) ;

    if (len  < 0 || sp_drain(port_choice) ) { 
        fprintf(stderr, "Error reading from serial port\n"); 
        bvt3000_comm_fail( );
    }
    #ifdef DEBUG
    if(verboseFlag){
        printf("DEBUG: received serial string: '"); 
        for (int i = 0; i<len; i++) { 
            printf("%02x",buf[i]); 
        }
        printf("'\n"); 
    }
    #endif 

    if(len == sizeof buf - 1                        // reply too long //
		 || buf[ 0 ] != STX                              // missing STX //
         || buf[ len - 2 ] != ETX                        // missing ETX //
         || strncmp( buf + 1, cmd, 2 ) ) {                 // wrong command //
        fprintf(stderr, "Comunication may be degraded\n") ; 
        fprintf(stderr, "Received bytes: 0x'"); 
        for (int i=0; i<len; i++) { 
            fprintf(stderr, "%02x",buf[i]);
        } 
        fprintf(stderr, "' \n"); 

    } 

    bc = buf[ len - 1 ];
    buf[ len - 1 ] = '\0';

    if ( ! bvt3000_check_bcc( ( unsigned char * ) ( buf + 1 ), bc ) )
        bvt3000_comm_fail( );
    
	/* Return just the data as a '\0'-terminated string */

    buf[ len - 2 ] = '\0';
	return buf + 3;
}

char* bvt3000_query_debug(const char * cmd, struct sp_port* port_choice , char const * caller_name) { 
    if(verboseFlag){
    printf("Calling bvt3000 query from %s\n", caller_name); }
    return(bvt3000_query(cmd, port_choice)); 
}


#ifdef DEBUG
    #define bvt3000_query(cmd, port_choice) bvt3000_query_debug(cmd, port_choice,__func__) 
#endif

/******************************************************************
 * Boring, un-call-stack reporting if we're making debug functions
 * are below. Their order, alas unlike those above, does not matter.
 ******************************************************************
 */


void list_ports() {
    //List detected ports via the library
    int i;
    struct sp_port **ports;

    enum sp_return error = sp_list_ports(&ports);
    if (error == SP_OK) {
        for (i = 0; ports[i]; i++) {
            char* name = sp_get_port_name(ports[i]); 
            printf("Found serial device: '%s'\n", name);
        }
        sp_free_port_list(ports);
    } else {
        printf("No serial devices detected\n");
    }
    printf("\n");
}


struct sp_port* open_and_init_port(char* desired_port, struct sp_port *port_choice) { 
    enum sp_return error = sp_get_port_by_name(desired_port,&port_choice);
    if (error == SP_OK) {
        error = sp_open(port_choice,SP_MODE_READ_WRITE);
        if (error == SP_OK) {
            sp_set_baudrate(port_choice,BAUD_RATE);
            sp_set_bits(port_choice, NUM_DATA_BITS); 
            sp_set_stopbits(port_choice, NUM_STOP_BITS); 
            sp_set_parity(port_choice, SP_PARITY_EVEN); 
        }
        else {
            printf("Error opening serial device\n");
            #ifndef DEBUG
                fprintf(stderr,"Unable to open and initialise serial port %s. Quitting.\n", desired_port); 
                exit(EXIT_FAILURE); 
             #else 
                fprintf(stderr,"WARNING: Error opening serial port %s, attempting to carry on...\n", desired_port); 
            #endif
        }
    } else {
        printf("Error finding serial device\n");
    }
    return(port_choice); 
}

void bvt3000_send_command( const char * cmd , struct sp_port *port_choice )
{
	char buf[ 100 ];
	ssize_t len;

    assert(sizeof cmd < 70); 

	/* Assemble the string to be sent */

	sprintf( buf, "%c%02d%02d%c%s%c",
             EOT, GROUP_ID, DEVICE_ID, STX, cmd, ETX );
	len = bvt3000_add_bcc( ( unsigned char * ) buf + 6 ) + 6;

	/* Send string and check for ACK */

    enum sp_return error = sp_blocking_write(port_choice, buf, len, SERIAL_WAIT); 
    sp_drain(port_choice); 

    #ifdef DEBUG
    if(verboseFlag){
        printf("DEBUG: sent serial string: '"); 
        for (int ix = 0; ix<len; ix++) { 
            printf("%02x",buf[ix]); 
        }
        printf("'\n"); 
    }
    #endif 

    if (error > SP_OK) { // Check for ACK 
        if(bvt3000_check_ack(port_choice) != OK ) {
            bvt3000_comm_fail(); 
        }
    } else { 
        fprintf(stderr,"WARNING: Error sending command %s\n", cmd); 
    }
}

size_t  bvt3000_add_bcc( unsigned char * data )
{
	size_t len = 2;
	unsigned char bcc = *data++;


	while ( *data != ETX ) {
		bcc ^= *data++;
		len++;
	} 
    /* Append the last ETX */
    bcc ^= *data++; 
    len++; 


	/* Append the resulting BCC and return the total length */

	*data = bcc;
	return len;
}



/*------------------------------------------------------------------*
 * Tests if the ACK string sent from the device is present and correct
 *------------------------------------------------------------------*/

bool bvt3000_check_ack( struct sp_port* port_choice)
{
	unsigned char r;
    enum sp_return error = sp_blocking_read(port_choice, &r, 1, ACK_WAIT); 
    sp_drain(port_choice); 
    
    #ifdef DEBUG
    if(verboseFlag){
        printf("Received ack char: '%02x'\n",r);
    }
    #endif

    if (error < SP_OK) {
        return FAIL;
    }
    if ( r == ACK )
        return OK;

    if ( r == NAK )
        bvt3000_query( "EE" , port_choice);    

    return FAIL;

}



/*------------------------------------------------------------------*
 * Tests if the BCC (block check character) for a string is correct
 *------------------------------------------------------------------*/

bool
bvt3000_check_bcc( unsigned char * data,
                   unsigned char   bcc )
{
	while ( *data != ETX )
		bcc ^= *data++;

	return bcc == ETX;
}

/*----------------------------------------------------*
 * Returns information about the status of the device
 *----------------------------------------------------*/

unsigned int bvt3000_get_interface_status( struct sp_port* port_choice)
{
    unsigned int is = 0xF802;
    char *reply = bvt3000_query( "IS" , port_choice);


    if (    reply[ 0 ] != '>'
         || sscanf( reply + 1, "%x", &is ) != 1
         || is & 0xF802 )
        bvt3000_comm_fail( );

    return is;
}

/*--------------------------------------------*
 * Returns if the heater is ok or overheating
 *--------------------------------------------*/

int bvt3000_check_heater( struct sp_port* port_choice)
{
    return ( bvt3000_get_interface_status(port_choice ) & BVT3000_HEATER_OVERHEATING ) ?
           HEATER_OVERHEATING : HEATER_OK;
}

/*------------------------------------*
 * Returns if the heater is on or off
 *------------------------------------*/

bool bvt3000_get_heater_state( struct sp_port* port_choice)
{
    char *reply = bvt3000_query( "HP" , port_choice);


    if ( reply[ 0 ] != '1' && reply[ 0 ] != '0' )
        bvt3000_comm_fail( );
    return reply[ 0 ] == '1' ? SET : UNSET;
}


/*-------------------------------*
 * Switches the heater on or off 
 *-------------------------------*/

void bvt3000_set_heater_state( bool state, struct sp_port* port_choice)
{
    char buf[ 4 ] = "HP*";


    /* Before switching on the heater check if gas flow is present */

    if ( state && bvt3000_get_interface_status(port_choice) & BVT3000_MISSING_GAS_FLOW )
    {
        printf("FATAL: Can't switch on heater, gas flow is missing.\n" );
    }

    buf[ 2 ] = state ? '1' : '0';
    bvt3000_send_command( buf, port_choice);
    usleep( 500000 );
}


/*-----------------------------------------------*
 * Asks the device for the current gas flow rate 
 *-----------------------------------------------*/

unsigned int
bvt3000_get_flow_rate( struct sp_port* port_choice )
{
    unsigned int flow_rate = 0;
    char *reply = bvt3000_query( "AF", port_choice); 
    unsigned int i;


    if ( *reply++ != '>' )
        bvt3000_comm_fail( );

    for ( i = 1; i < 5; reply++, i++ )
    {
        if ( *reply != '1' && *reply != '0' )
            bvt3000_comm_fail( );

        if ( *reply == '1' )
            flow_rate |= 1 << ( 4 - i );
    }

    return flow_rate;
}

/*----------------------------------------*
 * Sends command to set the gas flow rate
 *----------------------------------------*/

void
bvt3000_set_flow_rate( unsigned int flow_rate, struct sp_port* port_choice )
{
    char buf[ 20 ];


    assert( flow_rate <= 0x0F );

    sprintf( buf, "AF>%c%c%c%c", flow_rate & 0x8 ? '1' : '0',
                                 flow_rate & 0x4 ? '1' : '0',
                                 flow_rate & 0x2 ? '1' : '0',
                                 flow_rate & 0x1 ? '1' : '0' );
    bvt3000_send_command( buf , port_choice);
}


/*---------------------------------------*
 * Reads from port 1 to 4 (not used yet)
 *---------------------------------------*/

unsigned char
bvt300_get_port( int port , struct sp_port* port_choice )
{
    char buf[ ] = "P0";
    unsigned int x = 0xFF;
    char *reply;


    assert( port >= 1 && port <= 4 );

    buf[ 1 ] += port;
    reply = bvt3000_query( buf , port_choice);

    if (    reply[ 0 ] != '>'
         || sscanf( reply + 1, "%x", &x ) != 1
         || x > 0x0F )
        bvt3000_comm_fail( );

    return ( unsigned char ) x;
}

/*--------------------------------------------------------------*
 * Sets temperature controller operation mode (normal operation or configuration mode)
 *--------------------------------------------------------------*/

void eurotherm902s_set_operation_mode( int op_mode, struct sp_port* port_choice )
{
    char cmd[ ] = "IM *";


    assert(    op_mode == NORMAL_OPERATION
                 || op_mode == CONFIGURATION_MODE );

    cmd[ 3 ] = '0' + op_mode;
    bvt3000_send_command( cmd , port_choice);
}

/*-----------------------------------*
 * Returns if there's a sensor break on the thermocouple / PT100 lines
 *-----------------------------------*/

bool eurotherm902s_check_sensor_break( struct sp_port* port_choice  )
{
    return eurotherm902s_get_sw(port_choice ) & SENSOR_BREAK_FLAG ? SET : UNSET;

}

/*--------------------------------------------------*
 * Returns the operation mode (for normal operation
 * 0 is returned, for configuration mode 2)
 *--------------------------------------------------*/

int eurotherm902s_get_operation_mode( struct sp_port* port_choice )
{
    return atoi( bvt3000_query( "IM" , port_choice) );
}

/*-----------------------------------------------------------------*
 * Sets the mode to either automatic or manual (in manual mode the
 * output power can be controlled via the "UP" and "DOWN" keys).
 *-----------------------------------------------------------------*/

void eurotherm902s_set_mode( int mode, struct sp_port* port_choice )
{
    unsigned sw = eurotherm902s_get_sw(port_choice );


    assert( mode == AUTOMATIC_MODE || mode == MANUAL_MODE );

    if ( mode == AUTOMATIC_MODE )
        sw &= ~ MANUAL_MODE_FLAG;
    else
        sw |= MANUAL_MODE_FLAG;

    eurotherm902s_set_sw( sw, port_choice );
}

/*---------------------------------------------------------*
 * Returns the mode (automatic or manual) the device is in
 *---------------------------------------------------------*/

int eurotherm902s_get_mode( struct sp_port* port_choice )

{
    return eurotherm902s_get_sw(port_choice ) & MANUAL_MODE_FLAG ?
           MANUAL_MODE : AUTOMATIC_MODE;
}

/*--------------------------------------------*
 * Returns the current (measured) temperature
 *--------------------------------------------*/

double eurotherm902s_get_temperature( struct sp_port* port_choice  )
{
    if(verboseFlag){printf("DEBUG: requested temperature\n"); }
    return atof( bvt3000_query( "PV",port_choice ) );
}
/*------------------------------*
 * Sets the setpoint to be used
 *------------------------------*/

void eurotherm902s_set_active_setpoint( int sp, struct sp_port* port_choice )
{
    if(verboseFlag){printf("DEBUG: setting active setpoint\n"); }
    unsigned int sw = eurotherm902s_get_sw(port_choice );


    assert( sp == SP1 || sp == SP2 );

    if ( sp == SP1 )
        sw &= ~ ACTIVE_SETPOINT_FLAG;
    else
        sw |= ACTIVE_SETPOINT_FLAG;

    eurotherm902s_set_sw( sw , port_choice);
}

/*-------------------------------------*
 * Returns the currently used setpoint
 *-------------------------------------*/

int eurotherm902s_get_active_setpoint( struct sp_port* port_choice )
{
    if(verboseFlag){printf("DEBUG: getting setpoint\n"); }
    return eurotherm902s_get_sw(port_choice ) & 0x2000 ? SP2 : SP1;
}

/*----------------------------------------------*
 * Sets the setpoint value of either SP1 or SP2
 *----------------------------------------------*/

void eurotherm902s_set_setpoint( int    sp,
                            double temp,
                            struct sp_port* port_choice )
{

    if(verboseFlag){printf("DEBUG: setting setpoint value to %lf\n", temp); }
    char buf[ 20 ];

    assert( sp == SP1 || sp == SP2 );
    sprintf( buf, "S%c%6.1f", sp == SP1 ? 'L' : '2', temp );
    bvt3000_send_command( buf, port_choice );
}


/*-------------------------------------------------*
 * Returns the setpoint value of either SP1 or SP2
 *-------------------------------------------------*/

double eurotherm902s_get_setpoint( int sp, struct sp_port* port_choice )
{
    assert( sp == SP1 || sp == SP2 );
    return atof( bvt3000_query( sp == SP1 ? "SL" : "S2" , port_choice) );
}


/*--------------------------------------------------------*
 * Returns the value of the setpoint currently being used
 * (i.e. the temperature for the currently used setpoint)
 *--------------------------------------------------------*/

double eurotherm902s_get_working_setpoint( struct sp_port* port_choice )
{
    return atof( bvt3000_query( "SP" , port_choice) );
}

/*----------------------*
 * Sets the status word
 *----------------------*/

void eurotherm902s_set_sw( unsigned int sw, struct sp_port* port_choice )
{
    char buf[ 8 ];


    assert( sw <= 0xFFFF );

    sprintf( buf, "SW>%04x", sw & 0xE005 );
    bvt3000_send_command( buf , port_choice);
}

/*-------------------------*
 * Returns the status word
 *-------------------------*/

unsigned int eurotherm902s_get_sw( struct sp_port* port_choice )
{
    unsigned long sw = 0x10000;
    char *reply = bvt3000_query( "SW" ,port_choice);


    if (    *reply != '>'
         || sscanf( reply + 1, "%lx", &sw ) != 1
         || sw > 0xFFFF )
        bvt3000_comm_fail( );
    return sw;
}

/*-------------------------------*
 * Sets the optional status word
 *-------------------------------*/

void eurotherm902s_set_os( unsigned int os, struct sp_port* port_choice )
{
    char buf[ 8 ];


    assert( os <= 0xFFFF );

    sprintf( buf, "OS>%04x", os & 0x30BF );
    bvt3000_send_command( buf ,port_choice);
}


/*----------------------------------*
 * Returns the optional status word
 *----------------------------------*/

unsigned int eurotherm902s_get_os( struct sp_port* port_choice )
{
    unsigned long os = 0x10000;
    char *reply = bvt3000_query( "OS" ,port_choice);


    if (    *reply != '>'
         || sscanf( reply + 1, "%lx", &os ) != 1
         || os > 0xFFFF )
        bvt3000_comm_fail( );

    return os;
}

/*--------------------------------*
 * Sets the extension status word
 *--------------------------------*/

void eurotherm902s_set_xs( unsigned int xs,  struct sp_port* port_choice )
{
    char buf[ 8 ];


    assert( xs <= 0xFFFF );

    sprintf( buf, "XS>%04x", xs & 0xFFB7 );
    bvt3000_send_command( buf ,port_choice);
}

/*-----------------------------------*
 * Returns the extension status word
 *-----------------------------------*/

unsigned int eurotherm902s_get_xs( struct sp_port* port_choice )
{
    unsigned long xs = 0x10000;
    char *reply = bvt3000_query( "XS" , port_choice);


    if (    *reply != '>'
         || sscanf( reply + 1, "%lx", &xs ) != 1
         || xs > 0xFFFF )
        bvt3000_comm_fail( );

    return xs;
}

/*---------------------------*
 * Returns if an alarm is on
 *---------------------------*/

bool eurotherm902s_get_alarm_state( struct sp_port* port_choice )
{
    return eurotherm902s_get_xs( port_choice) & ALARMS_STATE_FLAG ? SET : UNSET;
}

/*-------------------------------*
 * Sets self tune stat on or off
 *-------------------------------*/

void eurotherm902s_set_self_tune_state( bool on_off , struct sp_port* port_choice )
{
    unsigned int xs = eurotherm902s_get_xs(port_choice );


    if ( on_off )
        xs |= SELF_TUNE_FLAG;
    else
        xs &= ~ SELF_TUNE_FLAG;

    eurotherm902s_set_xs( xs , port_choice);
}

/*-----------------------------------------*
 * Returns if device is in self tune state
 *-----------------------------------------*/

bool eurotherm902s_get_self_tune_state( struct sp_port* port_choice )
{
    return eurotherm902s_get_xs(port_choice ) & SELF_TUNE_FLAG ? SET : UNSET;
}

/*------------------------------*
 * Sets adaptive tune on or off
 *------------------------------*/

void eurotherm902s_set_adaptive_tune_state( bool on_off, struct sp_port* port_choice )
{
    unsigned int xs = eurotherm902s_get_xs(port_choice );


    if ( on_off )
        xs |= ADAPTIVE_TUNE_FLAG;
    else
        xs &= ~ ADAPTIVE_TUNE_FLAG;

    eurotherm902s_set_xs( xs , port_choice);
}

/*---------------------------------------*
 * Returns if adaptive tune is on or off
 *---------------------------------------*/

bool eurotherm902s_get_adaptive_tune_state( struct sp_port* port_choice )
{
    return eurotherm902s_get_xs(port_choice ) & ADAPTIVE_TUNE_FLAG ? SET : UNSET;
}



/*----------------------------------*
 * Sets adaptive tune trigger level
 *----------------------------------*/

void eurotherm902s_set_adaptive_tune_trigger( double tr, struct sp_port* port_choice )
{
    char buf[ 20 ];


    assert( tr >= 0.0 && tr <= MAX_AT_TRIGGER_LEVEL );

    sprintf( buf, "TR%3.2f", tr );
    bvt3000_send_command( buf, port_choice );
}


/*-------------------------------------------------*
 * Returns the adaptive tune trigger level (in K?)
 *-------------------------------------------------*/

double eurotherm902s_get_adaptive_tune_trigger( struct sp_port* port_choice )
{
    return atof( bvt3000_query( "TR",port_choice ) );
}


/*-------------------------------------------------------------------------*
 * Returns the minimum value that can be set for the setpoints SP1 and SP2
 *-------------------------------------------------------------------------*/

double eurotherm902s_get_min_setpoint( int sp, struct sp_port* port_choice )
{
    assert( sp == SP1 || sp == SP2 );
    return atof( bvt3000_query( sp == SP1 ? "LS" : "L2" , port_choice) );
}

/*-------------------------------------------------------------------------*
 * Returns the maximum value that can be set for the setpoints SP1 and SP2
 *-------------------------------------------------------------------------*/

double eurotherm902s_get_max_setpoint( int sp, struct sp_port* port_choice )
{
    assert( sp == SP1 || sp == SP2 );
    return atof( bvt3000_query( sp == SP1 ? "HS" : "H2", port_choice ) );
}

/*------------------------------------*
 * Sets the maximum heater power in %
 *------------------------------------*/

void eurotherm902s_set_heater_power_limit( double power, struct sp_port* port_choice )
{
    char buf[ 20 ];


    assert( power >= 0.0 && power <= 100.0 );
    sprintf( buf, "HO%6.1f", power );
    bvt3000_send_command( buf ,port_choice);
}

/*---------------------------------------*
 * Returns the maximum heater power in %
 *---------------------------------------*/

double eurotherm902s_get_heater_power_limit( struct sp_port* port_choice )
{
    return atof( bvt3000_query( "HO" , port_choice) );
}

/*---------------------------------------*
 * Sets the heater power in % - can only
 * be set when device is in AUTO mode
 *---------------------------------------*/

void eurotherm902s_set_heater_power( double power, struct sp_port* port_choice )
{
    char buf[ 20 ];

    assert( power >= 0.0 && power <= 100.0 );
    assert( eurotherm902s_get_mode(port_choice) == AUTOMATIC_MODE );

    sprintf( buf, "OP%6.1f", power );
    bvt3000_send_command( buf , port_choice);
}

/*-------------------------------*
 * Returns the heater power in %
 *-------------------------------*/

double eurotherm902s_get_heater_power( struct sp_port* port_choice )
{
    return atof( bvt3000_query( "OP" ,port_choice) );
}

/*---------------------------------------------------------------*
  Set proportional part of the PID controller
 *---------------------------------------------------------------*/

void eurotherm902s_set_proportional_band( double pb,  struct sp_port* port_choice )
{
    char buf[ 20 ];


    assert( pb <= MAX_PROPORTIONAL_BAND );

    sprintf( buf, "XP%6.2f", pb );
    bvt3000_send_command( buf , port_choice);
}
/*---------------------------------------------------------------*
 * Get the proportional part of the PID controller
 *---------------------------------------------------------------*/

double eurotherm902s_get_proportional_band( struct sp_port* port_choice )
{
    return atof( bvt3000_query( "XP", port_choice ) );
}

/*------------------------------------------*
 * Sets the integration time in seconds (?)
 *------------------------------------------*/

void eurotherm902s_set_integral_time( double it,  struct sp_port* port_choice )
{
    char buf[ 20 ];

    assert( it <= MAX_INTEGRAL_TIME );

    if ( it < 999.5 )
        assert( sprintf( buf, "TI%6.2f", it ) <= 8 );
    else
        assert( sprintf( buf, "TI%6.1f", it ) <= 8 );
    bvt3000_send_command( buf , port_choice);
}

/*---------------------------------------------*
 * Returns the integration time in seconds (?)
 *---------------------------------------------*/

double eurotherm902s_get_integral_time( struct sp_port* port_choice )
{
    return atof( bvt3000_query( "TI" , port_choice) );
}

/*-----------------------------------------*
 * Sets the derivative time in seconds (?)
 *-----------------------------------------*/

void eurotherm902s_set_derivative_time( double dt , struct sp_port* port_choice )
{
    char buf[ 20 ];


    assert( dt <= MAX_DERIVATIVE_TIME );

    sprintf( buf, "TD%6.2f", dt );
    bvt3000_send_command( buf , port_choice);
}

/*--------------------------------------------*
 * Returns the derivative time in seconds (?)
 *--------------------------------------------*/

double eurotherm902s_get_derivative_time(  struct sp_port* port_choice )
{
    return atof( bvt3000_query( "TD" , port_choice) );
}

/*-----------------------------*
 * Sets the high cutback value 
 *-----------------------------*/

void eurotherm902s_set_cutback_high( double cb,  struct sp_port* port_choice  )
{
    char buf[ 20 ];


    if ( cb < 999.5 )
        assert( sprintf( buf, "HB%6.2f", cb ) <= 8 );
    else
        assert( sprintf( buf, "HB%6.1f", cb ) <= 8 );
    bvt3000_send_command( buf , port_choice);
}

/*--------------------------------*
 * Returns the high cutback value 
 *--------------------------------*/

double eurotherm902s_get_cutback_high(  struct sp_port* port_choice )
{
    return atof( bvt3000_query( "HB", port_choice ) );
}


/*----------------------------*
 * Sets the low cutback value 
 *----------------------------*/

void eurotherm902s_set_cutback_low( double cb,  struct sp_port* port_choice  )
{
    char buf[ 20 ];


    if ( cb < 999.5 )
        assert( sprintf( buf, "LB%6.2f", cb ) <= 8 );
    else
        assert( sprintf( buf, "LB%6.1f", cb ) <= 8 );
    bvt3000_send_command( buf , port_choice);
}

/*-------------------------------*
 * Returns the low cutback value 
 *-------------------------------*/

double eurotherm902s_get_cutback_low(  struct sp_port* port_choice )
{
    return atof( bvt3000_query( "LB",port_choice ) );
}


/*-----------------------------*
 * Returns the display maximum
 *-----------------------------*/

double eurotherm902s_get_display_maximum( struct sp_port* port_choice)
{
    return atof( bvt3000_query( "1H" , port_choice) );
}

/*-----------------------------*
 * Returns the display minimum
 *-----------------------------*/

double eurotherm902s_get_display_minimum( struct sp_port* port_choice)
{
    return atof( bvt3000_query( "1L" , port_choice) );
}



/*-------------------------------*
 * Locks or unlocks the keyboard 
 *-------------------------------*/

void eurotherm902s_lock_keyboard( bool lock,struct sp_port* port_choice )
{
    unsigned int sw = eurotherm902s_get_sw(port_choice );


    if ( lock )
        sw |= KEYLOCK_FLAG;
    else
        sw &= ~ KEYLOCK_FLAG;

    eurotherm902s_set_sw( sw , port_choice );
}


/*-----------------------------------*
 * Switches the LN2 heater on or off 
 *-----------------------------------*/
void bvt3000_set_ln2_heater_state( bool state, struct sp_port* port_choice )
{
    char buf[ 4 ] = "NP*";


    buf[ 2 ] = state ? '1' : '0';
    bvt3000_send_command( buf , port_choice);
    usleep( 500000);
}

/*----------------------------------------*
 * Returns if the LN2 heater is on or off 
 *----------------------------------------*/

bool
bvt3000_get_ln2_heater_state( struct sp_port* port_choice)
{
    char *reply = bvt3000_query( "NP" , port_choice);

    if ( reply[ 0 ] != '1' && reply[ 0 ] != '0' )
        bvt3000_comm_fail( );
    return reply[ 0 ] == '1' ? SET : UNSET;
}

/*---------------------------*
 * Sets the LN2 heater power
 *---------------------------*/

void bvt3000_set_ln2_heater_power( double p, struct sp_port* port_choice )
{
    char buf[ 8 ];

    assert( p >= 0.0 && p <= 100.0 );

    sprintf( buf, "NH%5.2f", p );
    bvt3000_send_command( buf, port_choice );
}

/*------------------------------*
 * Returns the LN2 heater power
 *------------------------------*/

double
bvt3000_get_ln2_heater_power( struct sp_port* port_choice )
{
    return (double)atof( bvt3000_query( "NH", port_choice ) );
}

/*-----------------------------------------------------------------*
 * Returns if the LN2 tank needs is empty, needs a refill or is ok
 *-----------------------------------------------------------------*/

int bvt3000_check_ln2_heater( struct sp_port* port_choice)
{
    unsigned int state = bvt3000_get_interface_status( port_choice );


    if ( state & BVT3000_LN2_EMPTY )
        return LN2_TANK_EMPTY;
    else if ( state & BVT3000_LN2_REFILL )
        return LN2_NEEDS_REFILL;

    return LN2_OK;
}

/* ------------------------------------------- *
 * Helper function to print out raw bytes 
 * WARNING: destructive! Do not use the string
 * afterward!
 *------------------------------------------- */

void printArray(char* buf) {
  while (*buf) printf("%02x", *buf++);
}
