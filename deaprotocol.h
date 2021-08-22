#ifndef INC_deaprotocolh
#define INC_deaprotocolh
/*---------------------------------------------------------------------------
 
  FILENAME:
        deaprotocol.h
 
  PURPOSE:
        Fornisce il protocollo di comunicazione con la stazione meteo DeA
 
  NOTES:
        
 
  LICENSE:
  
        This source code is released for free distribution under the terms 
        of the GNU General Public License.
  
----------------------------------------------------------------------------*/

/*  ... System include files
*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <usb.h>


/*  ... Library include files
*/
#include <sysdefs.h>
#include <radtimeUtils.h>
#include <radsocket.h>


#define DEA_TEMP_SENSOR_COUNT       4

// some general time definitions
#define WV_SECONDS_IN_HOUR              3600


/* DeA  USB <vendorid, productid> */
#define dea_vendorid   0x1130
#define dea_productid  0x6880


#define DEA_MAX_RETRIES             5
#define DEA_BUFFER_LENGTH           88
#define DEA_READ_TIMEOUT            15 //secondi
#define DEA_USB_TIMEOUT            10 //millisec. timeout comandi USB
#define DEA_PKT_READ_DELAY        20000 //microsec. = 20ms ritardo nel loop di lettura buffer

#define DEA_MIN_BUF_LENGHT        72 // 9 pacchetti da 8 byte MINIMO

// Define the rain rate accumulator period (minutes):
// equivale al tempo di campionamento
#define DEA_RAIN_RATE_PERIOD         5


#define DEA_SENSOR_WIND       1
#define DEA_SENSOR_RAIN       2
#define DEA_SENSOR_PRESSURE   4
#define DEA_SENSOR_TEMP_MASK  0xFF00


/* Sensor ID for indoor and outdoor temp sensors */
/* The WMR200 supports 12 temp sensors. The default indoor
   sensor is always #0.  Sensor #1 is hardcoded as the
   outdoor sensor here.  Should perhaps be configurable */
#define DEA_TEMP_SENSOR_IN  0
#define DEA_TEMP_SENSOR_OUT 1



// parsing helper macros:
#define LO(byte)                (byte & 0x0f)
#define HI(byte)                ((byte & 0xf0) >> 4)
#define MHI(byte)            ((byte & 0x0f) << 4)
#define NUM(byte)            (10 * HI(byte) + LO(byte))
#define BIT(byte, bit)            ((byte & (1 << bit)) >> bit)



// Chiamata una sola volta all'init
extern struct usb_dev_handle*  deaInit();

// cleanup all'uscita
extern void deaExit(struct usb_dev_handle* dev);

// lettura dati dalla stazione:
extern int deaReadData(struct usb_dev_handle *devh);



#endif
 
