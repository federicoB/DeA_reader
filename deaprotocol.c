/*---------------------------------------------------------------------------
 
  FILENAME:
        deaprotocol.c
 
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
#include <sys/time.h>
#include <string.h>
#include <time.h>
#include <usb.h>


/*  ... Library include files
*/

#include <radtimeUtils.h>

/*  ... Local include files
*/
#include <deaprotocol.h>

/* Funzioni USB */
static struct usb_device *find_device(int vendor, int product) {
    struct usb_bus *bus;

    for (bus = usb_get_busses(); bus; bus = bus->next) {
        struct usb_device *dev;

        for (dev = bus->devices; dev; dev = dev->next) {
            if (dev->descriptor.idVendor == vendor
                && dev->descriptor.idProduct == product)
                return dev;
        }
    }
    return NULL;
}

// conversione temperatura
double Temperature_cal(int y) {
    double num;
    if ((y & 0x800) != 0) {
        // XOR per le T negative
        y ^= 0xfff;
        num = (y + 1) * -0.1;
    } else {
        num = y * 0.1;
    }
    if ((num <= 70.0) && (num >= -50.0)) {
        return num;
    }
    return 9999;
}

// conversione HR
double Humidity_cal(double y) {
    y = ((((((int) y) & 0xf00) >> 8) * 100) + (((((int) y) & 240) >> 4) * 10)) + (((int) y) & 15);
    if (y > 100.0) {
        y = 9999;
    }
    return y;
}

void get_time_string(char *datetime) {
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(datetime, 20, "%d/%m/%Y %H:%M:%S", timeinfo);

}

/* Funzioni di decodifica del pacchetto dati */

static void decodeRain(unsigned char *ptr) {
    // la stazione fornisce (pare) solo la pioggia cumulata in mm
    float rainmm;
    rainmm = (float) (0.25 * (((((ptr[15] & 240) >> 4) | (ptr[16] << 4)) | (ptr[17] << 12)) | ((ptr[18] & 15) << 20)));
    //deaWork.dataRXMask |= DEA_SENSOR_RAIN;
    char *datetime = (char *) malloc(20 * sizeof(char));
    get_time_string(datetime);
    FILE *rain = fopen("rain.csv", "a");
    fprintf(rain, "%s, %1.2f,\n", datetime, rainmm);
    fclose(rain);
    printf("dea: rain  accum %1.2f\n", rainmm);
}

static void decodeWind(unsigned char *ptr) {
    float gust, avg;
    gust = (float) (((ptr[32] & 240) >> 4) + ((ptr[33] & 15) << 4)) * 0.2;
    avg = (float) ptr[12] * 0.2;
    float windGustSpeed = gust * 2.23694;
    float windAvgSpeed = avg * 2.23694;
    float windDir = 22.5 * ((ptr[13] & 240) >> 4);
    char *datetime = (char *) malloc(20 * sizeof(char));
    get_time_string(datetime);
    FILE *wind = fopen("wind.csv", "a");
    fprintf(wind, "%s, %1.1f, %1.1f, %3.1f,\n",
            datetime,
            windAvgSpeed,
            windGustSpeed,
            windDir);
    fclose(wind);
    printf(
            "dea: wind speed %1.1f  gust %1.1f  direction %3.1f\n",
            windAvgSpeed,
            windGustSpeed,
            windDir);

}

static void decodeTemp(unsigned char *ptr) {
    // la temperatura fornita è in gradi celsius
    // e viene convertita in farheneit
    // valorizzo i 4 sensori
    // 0 interno
    // 1,2,3 esterni
    int y, sensor;
    float humidity[4];
    float temp[4];
    // TermoIgro Interno
    y = (((int) ptr[1] & 15) << 8) | ptr[0];
    humidity[0] = (float) Humidity_cal((double) y);
    y = (ptr[2] << 4) | ((ptr[1] & 240) >> 4);
    temp[0] = (float) Temperature_cal(y);
    // TermoIgro Ext Ch1
    y = (((int) ptr[4] & 15) << 8) | ptr[3];
    humidity[1] = (float) Humidity_cal((double) y);
    y = (ptr[5] << 4) | ((ptr[4] & 240) >> 4);
    temp[1] = (float) Temperature_cal(y);
    // TermoIgro Ext Ch2
    y = (((int) ptr[7] & 15) << 8) | ptr[6];
    humidity[2] = (float) Humidity_cal((double) y);
    y = (ptr[8] << 4) | ((ptr[7] & 240) >> 4);
    temp[2] = (float) Temperature_cal(y);
    // TermoIgro Ext Ch3
    y = (((int) ptr[10] & 15) << 8) | ptr[9];
    humidity[2] = (float) Humidity_cal((double) y);
    y = (ptr[11] << 4) | ((ptr[10] & 240) >> 4);
    temp[2] = (float) Temperature_cal(y);
    // external temp can be channel 1, 2 or 3. find the most reasonable one
    int i;
    float real_external_temp = 150;
    float real_external_humidity = 150;
    for (i = 1; i < 4; i = i + 1) {
        if (temp[i] < real_external_temp)
            real_external_temp = temp[i];
        if ((humidity[i] < real_external_humidity) && humidity[i] > 0)
            real_external_humidity = humidity[i];
    }
    char *datetime = (char *) malloc(20 * sizeof(char));
    get_time_string(datetime);
    FILE *internal_temp = fopen("internal_temp.csv", "a");
    fprintf(internal_temp, "%s, %1.1f, %2.0f,\n", datetime, temp[0], humidity[0]);
    fclose(internal_temp);
    FILE *external_temp = fopen("external_temp.csv", "a");
    //deaWork.dataRXMask |= 1 << (8+sensor);
    fprintf(external_temp, "%s, %1.1f, %2.0f,\n", datetime, real_external_temp, real_external_humidity);
    fclose(external_temp);
    for (sensor = 0; sensor < 4; sensor++) {
        printf(
                "dea: temp sensor %02d  temperature %1.1f  humidity %2.0f \n",
                sensor,
                temp[sensor],
                humidity[sensor]);
    }

}


static void decodePres(unsigned char *ptr) {
    // la pressione è fornita in hPa (equiv mb)
    // e viene convertita in [inches of Hg]
    float pressure =
            0.02953 * ((int) ((ptr[18] & 240) >> 4) + (ptr[19] << 4));
    pressure = wvutilsConvertINHGToHPA(pressure);
    //deaWork.dataRXMask |= WMR200_SENSOR_PRESSURE;
    char *datetime = (char *) malloc(20 * sizeof(char));
    get_time_string(datetime);
    FILE *pressure_file = fopen("pressure.csv", "a");
    fprintf(pressure_file, "%s, %2.2f,\n", datetime, pressure);
    fclose(pressure_file);
    printf("dea: barometric pressure %2.2f inHg\n", pressure);
}


static int readStationData(struct usb_dev_handle *devh) {
    int i, j, size, blen, ret, usb_error;
    float tempFloat;
    unsigned char *ptr;
    unsigned char packet[8];
    unsigned char recbuf1[88];
    unsigned char recbuf2[88];
    unsigned char recbuf[88];
    // il comando vero e proprio è costituito da 0xff, 0xaf, il resto
    // dovrebbe essere solo padding
    // altri comandi sniffati sono:
    // 0xfa, 0xaf, = richiesta scarico datalogger
    // 0xfd, 0xaf, = stop USB
    char write_packet[8] = {0x02, 0xff, 0xaf, 0xb8, 0x8c, 0x0a, 0xf6, 0xbc};
    /* azzero i buffer */
    for (i = 0; i < 88; i++) {
        recbuf1[i] = 0x00;
        recbuf2[i] = 0x00;
    }
    /*
       Il ciclo di lettura della stazione consiste nell'invio
       di un comando e nella lettura del buffer in seguito a
       messaggi "BULK_INTERRUPT_TRANSFER"
       Dal log con SnoopyPro si rileva che il comando di lettura
       viene inviato due volte di seguito.
    */
    /*
      Invio del comando per ottenere i dati RealTime
    */
    usb_control_msg(devh,
                    USB_TYPE_CLASS + USB_RECIP_INTERFACE,
                    0x09, 0x0200, 0x00, write_packet, 0x08, DEA_USB_TIMEOUT);
    // if (ifWorkData->stationExtLog == 2) printf("dea: Command Response: %d\n", ret);
    usleep(500000);

    /*
      A questo punto vengono generati degli interrupt
      a ciascuno è associato un payload di 8 bytes
      per un totale di 11 pacchetti (interrupts)
    */
    j = 0;
    blen = 0; // serve per il controllo della lunghezza dati ricevuta
    while (1) {
        // loop infinito di lettura interrupt
        ret = usb_interrupt_read(devh, 0x81, packet, 0x08, DEA_USB_TIMEOUT);
        // if (ifWorkData->stationExtLog == 2) printf( "dea: Interrupt Response: %d\n", ret);
        usleep(DEA_PKT_READ_DELAY);
        if (ret == 8) {
            blen += 8;
            size = (int) packet[0]; // il primo byte fornisce il numero di bytes significativi nell'ottetto
            //    printf( "dea: Reading interrupt packet %02X %02X %02X %02X %02X %02X %02X %02X\n",
            //              packet[0], packet[1], packet[2], packet[3], packet[4], packet[5], packet[6], packet[7]);
            if (blen <= 88) { // check di buffer overflow, in caso di num.pkt superiore a 11 semplicemente li ignoro
                for (i = 0; i < size; i++) {
                    recbuf1[j + i] = packet[i + 1];
                }
            }
        } else {
            break;
        }
        j += size;
    }

    /* ripeto il ciclo di lettura dopo circa un secondo */
    usleep(1000000);
    memcpy(write_packet, "\x02\xff\xaf\xb8\x8c\x0a\xf6\xbc", 8);
    //write_packet[8] = { 0x02, 0xff, 0xaf, 0xb8, 0x8c, 0x0a, 0xf6, 0xbc};
    //  if (ifWorkData->stationExtLog == 2) printf("dea: Sending reading command.... \n");
    ret = usb_control_msg(devh,
                          USB_TYPE_CLASS + USB_RECIP_INTERFACE,
                          0x09, 0x0200, 0x00, write_packet, 0x08, DEA_USB_TIMEOUT);
//    if (ifWorkData->stationExtLog == 2) printf("dea: Command Response: %d\n", ret);
    usleep(500000);
    j = 0;
    blen = 0;
    while (1) {
        // loop infinito di lettura interrupt
        ret = usb_interrupt_read(devh, 0x81, packet, 0x08, DEA_USB_TIMEOUT);
        // if (ifWorkData->stationExtLog == 2) printf( "dea: Interrupt Response: %d\n", ret);
        usleep(DEA_PKT_READ_DELAY);
        if (ret == 8) {
            blen += 8;
            size = (int) packet[0]; // il primo byte fornisce il numero di bytes significativi nell'ottetto
            // printf( "dea: Reading interrupt packet %02X %02X %02X %02X %02X %02X %02X %02X\n",
            //          packet[0], packet[1], packet[2], packet[3], packet[4], packet[5], packet[6], packet[7]);
            if (blen <= 88) {
                for (i = 0; i < size; i++) {
                    recbuf2[j + i] = packet[i + 1];
                }
            }
        } else {
            break;
        }
        j += size;
    }

    /* adesso confronto i due pacchetti è, se uguali, confermo il campione
       ma prima rendo uguali i bytes che contengono il timestamp */
    usb_error = 0;
    for (i = 0; i < 10; i++) {
        recbuf1[21 + i] = recbuf2[21 + i];
    }
    for (i = 0; i < 33; i++) {
        if (recbuf1[i] != recbuf2[i]) usb_error = 1; // campioni con dati diversi ---> da scartare
    }

    if (blen < DEA_MIN_BUF_LENGHT) usb_error = 1;

    if (usb_error != 1) {
        printf(
                "dea: sample acquired\n");
        ptr = recbuf;
        memcpy(ptr, recbuf2, 88);
        // infine richiamo le funzioni di conversione
        decodeRain(ptr);
        decodeTemp(ptr);
        decodeWind(ptr);
        decodePres(ptr);
        printf("dea: data decoded\n");

        return OK;
    } else {
//      printf("dea: ERROR on USB transfer\n");
        return ERROR;
    }
}

struct usb_dev_handle *deaInit() {
    struct usb_device *dev;
    struct usb_dev_handle *devh;
    fd_set rfds;
    struct timeval tv;
    time_t nowTime = time(NULL) - (WV_SECONDS_IN_HOUR / (60 / DEA_RAIN_RATE_PERIOD));
    unsigned char buf[255];
    int ret;


    /* standard libusb init functions */
    usb_init();
    usb_find_busses();
    usb_find_devices();
    dev = find_device(dea_vendorid, dea_productid);
    if (dev) {
        printf("dea: USB device found\n");
    } else {
        printf("dea: could not find USB device\n");
    }
    devh = usb_open(dev);
    if (devh) {
        printf(
                "dea: USB device opened\n");
    } else {
        printf("dea: could not open USB device\n");
    }
    // detach del dispositivo dal kernel
    ret = usb_get_driver_np(devh, 0, buf, sizeof(buf));
    if (ret == 0) {
        ret = usb_detach_kernel_driver_np(devh, 0);
    }
    /* claim the device */
    ret = usb_claim_interface(devh, 0);
    if (ret != 0) {
        printf("dea: could not claim USB device\n");
    } else {
        printf("DEA station init completed ...\n");
    }
    ret = usb_set_altinterface(devh, 0);
    usleep(100000);

    return devh;
}

void deaExit(struct usb_dev_handle *devh) {
    int ret;
    ret = usb_release_interface(devh, 0);
    if (!ret)
        printf("dea: usb interface release failed with code %d\n",
               ret);
    usb_close(devh);
}

int deaReadData(struct usb_dev_handle *devh) {
    int ret;
    ret = ERROR;
    ULONG poll_int = 50000; // in microsecondi
    ULONG poll_timeout = poll_int * 1 / 2000; // tengo circa il 50% di margine
    //printf("DEA Poll interval %d\n", poll_int);
    //printf("DEA Poll timeout %d\n", poll_timeout);
    time_t timeout = poll_timeout + time(0); // circa 50% del tempo di polling
    // riprovo a leggere i dati realtime fino
    // a quando la lettura è positiva o il timeout
    // è superato
    while (ret == ERROR) {
        // attendo un secondo
        usleep(1000000);
        // e provo a leggere
        ret = readStationData(devh);
        if (time(0) > timeout) {
            printf("dea: timeout!");
            return ERROR;
        }
    }
    return OK;
}
 
