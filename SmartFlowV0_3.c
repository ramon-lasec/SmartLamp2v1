
#include "wiced.h"
#include "dct_data.h"
#include "string.h"
#include "stdbool.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"

/******************************************************
 *                      Macros
 ******************************************************/
#define VERSION "Version 1.0.0\r\n"
/******************************************************
 *                    Constants
 ******************************************************/

#define TCP_PACKET_MAX_DATA_LENGTH        64
#define TCP_CLIENT_INTERVAL               2
#define TCP_SERVER_PORT                   8010
#define TCP_CLIENT_CONNECT_TIMEOUT        5000
#define TCP_CLIENT_RECEIVE_TIMEOUT        5000
#define TCP_CONNECTION_NUMBER_OF_RETRIES  3
#define TIMER_TIME (8000)

/* Change the server IP address to match the TCP echo server address */
#define TCP_SERVER_IP_ADDRESS MAKE_IPV4_ADDRESS(172,168,100,49)

#define CHANGE_SSID                      0
#define CHANGE_KEY                       1
#define CHANGE_SERVER                    2

#define BUFFER_RX_LEN       120
#define BUFFER_RX_WIDTH     120

#define BUFFER_SIZE    120
#define TEST_STR          "\r\nType something! Keystrokes are echoed to the terminal ...>\r\n"

#define        ERROR "No Value\r\n"
#define        TIMEOUT "TIMEOUT\r\n"

/******************************************************
 *                    Constants
 ******************************************************/
static int GPIO_Evac = 0;
static int GPIO_Aux  = 0;
static int GPIO_Recib = 0;

/****************************************************
 *              Uart definitions
 ****************************************************/
typedef struct RxDatas
{
    unsigned char bufferRx [BUFFER_RX_LEN];
}Receiver_structure;

struct RxDatas rxstruct [BUFFER_RX_WIDTH];

volatile uint8_t write_buffer_cnt_width;  //-Incoming data- raw's counter
volatile uint8_t write_buffer_cnt_len;   //-Incoming data- column's counter
volatile uint8_t read_buffer_cnt_width; //-data to read- Read column's counter


wiced_uart_config_t uart_config =
{
    .baud_rate    = 115200,
    .data_width   = DATA_WIDTH_8BIT,
    .parity       = NO_PARITY,
    .stop_bits    = STOP_BITS_1,
    .flow_control = FLOW_CONTROL_DISABLED,
};


wiced_ring_buffer_t rx_buffer1;
wiced_ring_buffer_t tx_buffer1;

wiced_ring_buffer_t rx_buffer2;
wiced_ring_buffer_t tx_buffer2;

uint8_t             rx_data1[BUFFER_RX_WIDTH];
uint8_t             tx_data1[BUFFER_RX_WIDTH];

uint8_t             rx_data2[BUFFER_RX_WIDTH];
uint8_t             tx_data2[BUFFER_RX_WIDTH];

uint8_t rx_data1_cont;


uint32_t    expected_data_size = 1;
    char        TX;
    char       RX;
    char        RX2[4];
    int con=0;
    unsigned char cadena[BUFFER_SIZE];
    char buffer_t[40];
    char    mcu[4];



/****************************************************
*              Uart definitions
****************************************************/
/****************************************************
 *         Static Function Declarations
 ****************************************************/
static wiced_result_t mcu_config(uint8_t *data,uint8_t len);
static wiced_result_t Un_Set_config();
static wiced_result_t Set_config();
static wiced_result_t  is_config();
static wiced_result_t print_app_dct( void );
static wiced_result_t init_tcp_w();
static wiced_result_t Set_SSID(uint8_t *data,uint8_t len);
static wiced_result_t Set_KEY(uint8_t *data,uint8_t len);
static wiced_result_t Set_MASK(uint8_t *data,uint8_t len);
static wiced_result_t Set_SERVER(uint8_t *data,uint8_t len);
static wiced_result_t Set_IP(uint8_t *data,uint8_t len);
static wiced_result_t Set_GATEWAY(uint8_t *data,uint8_t len);
static wiced_result_t uart_init(void);
static wiced_result_t tcp_client();
static wiced_result_t start_tcp();
static wiced_tcp_socket_t  tcp_client_socket;
static wiced_timed_event_t tcp_client_event;
static wiced_tcp_socket_t  socket;

/****************************************************
 *         Static Function Declarations
 ****************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/
/******************************************************
 *                    Variable
 ******************************************************/
uint32_t rs[4];
uint32_t rn[4];
uint32_t ri[4];
uint32_t rg[4];

uint32_t d1;
uint32_t d2;
uint32_t d3;
uint32_t d4;

int flag_tcp = 0;

unsigned long s1;
/******************************************************
 *                    Variable
 ******************************************************/



void application_start( )
{
        wiced_interface_t interface;
        wiced_result_t result;
        /* Initialize the device */
        wiced_init();
        uart_init();

        init_tcp_w();
        is_config();

        if(flag_tcp==1){


        wiced_interface_t interface;
        wiced_result_t result;

        d1= ((rg[0]<< 24) | (rg[1] << 16) | ( rg[2] << 8) | (rg[3]));
        d2= ((rn[0]<< 24) | (rn[1] << 16) | ( rn[2] << 8) | (rn[3]));
        d3= ((ri[0]<< 24) | (ri[1] << 16) | ( ri[2] << 8) | (ri[3]));

        s1 = MAKE_IPV4_ADDRESS(rs[0],rs[1],rs[2],rs[3]);

        wiced_ip_setting_t device_init_ip_settings2 ={
        .gateway={WICED_IPV4,{.v4=(uint32_t)d1}},
        .netmask={WICED_IPV4,{.v4=(uint32_t)d2}},
        .ip_address={WICED_IPV4,{.v4=(uint32_t)d3}},

        };

        result = wiced_network_up(WICED_STA_INTERFACE, WICED_USE_STATIC_IP, &device_init_ip_settings2);

        if ( result != WICED_SUCCESS )
        {

        wiced_uart_transmit_bytes(WICED_UART_3,("Bringing up network interface failed !\n"),40);
        }

        /* Create a TCP socket */
        if ( wiced_tcp_create_socket( &tcp_client_socket, interface ) != WICED_SUCCESS )
        {
        wiced_uart_transmit_bytes(WICED_UART_3,("TCP socket creation failed\n"),20);

        }
        /* Bind to the socket */
        wiced_tcp_bind( &tcp_client_socket, TCP_SERVER_PORT );

        /* Register a function to send TCP packets */
        wiced_rtos_register_timed_event( &tcp_client_event, WICED_NETWORKING_WORKER_THREAD, &tcp_client, TCP_CLIENT_INTERVAL * SECONDS, 0 );
            }
    while ( 1 )
    {
        //levantar el servicio tcp

         if( wiced_uart_receive_bytes( WICED_UART_3, &RX, &expected_data_size, WICED_NEVER_TIMEOUT ) == WICED_SUCCESS ){
            char com[2];

            if(RX=='\n'){
                memcpy(com,cadena, 2);

                if(strcmp(com,"-S") == 0){
                    Set_SSID(cadena,con);
                }
                else if(strcmp(com,"-A") == 0){
                    print_app_dct();
                }
                else if(strcmp(com,"-K") == 0){
                    Set_KEY(cadena,con);
                }
                else if(strcmp(com,"-I") == 0){
                    Set_SERVER(cadena,con);
                }
                else if(strcmp(com,"-G") == 0){

                    Set_GATEWAY(cadena,con);
                }
                else if(strcmp(com,"-M") == 0){
                    Set_MASK(cadena,con);
                }
                else if(strcmp(com,"-C") == 0){
                    Set_IP(cadena,con);
                }
                else if(strcmp(com,"-U") == 0){
                    start_tcp();
                    Set_config();
                }
                else if(strcmp(com,"-Y") == 0){
                    Un_Set_config();
                }
                else if(strcmp(com,"-X") == 0){
                    mcu_config(cadena,con);
                }
                else{

                }
            expected_data_size=1;
            con=0;
            memset(cadena,'\0',BUFFER_SIZE-1);
            memset(com,'\0',BUFFER_SIZE-1);
            memset(&RX,'\0',BUFFER_SIZE);
            }
            else{
                cadena[con++]=RX;
            }

        }


    }
}

static wiced_result_t uart_init(void)
{
        /* Initialise ring buffer */
        //ring_buffer_init(&rx_buffer, rx_data, RX_BUFFER_SIZE );
        ring_buffer_init(&rx_buffer1, rx_data1, BUFFER_SIZE ); /* Initialize ring buffer to hold receive data */
        ring_buffer_init(&rx_buffer2, rx_data2, BUFFER_SIZE); /* Initialize ring buffer to hold receive data */

        /* Initialise UART. A ring buffer is used to hold received characters */
       // wiced_uart_init( STDIO_UART, &uart_config, &rx_buffer );

        wiced_uart_init( WICED_UART_3, &uart_config, &rx_buffer1); /* Setup UART */
        wiced_uart_init( WICED_UART_1, &uart_config, &rx_buffer2); /* Setup UART */

        /* Send a test string to the terminal */
        //wiced_uart_transmit_bytes( STDIO_UART, TEST_STR, sizeof( TEST_STR ) - 1 );
       wiced_uart_transmit_bytes( WICED_UART_3, TEST_STR, sizeof( TEST_STR ) - 1 );
       wiced_uart_transmit_bytes( WICED_UART_1, TEST_STR, sizeof( TEST_STR ) - 1 );
        return WICED_SUCCESS;
}
static wiced_result_t print_app_dct( void ){
        dct_read_write_app_dct_t* dct_app = NULL;
        wiced_mac_t mac;
        platform_dct_wifi_config_t*  wifi_config;

        if ( wiced_dct_read_lock( (void**) &dct_app, WICED_FALSE, DCT_APP_SECTION, 0, sizeof( *dct_app ) ) != WICED_SUCCESS )
        {
            return WICED_ERROR;
        }
        // Get a copy of the WIFT config from the DCT into RAM
        wiced_dct_read_lock((void**) &wifi_config, WICED_FALSE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t));


        wiced_wifi_get_mac_address(&mac);
        /* since we passed ptr_is_writable as WICED_FALSE, we are not allowed to write in to memory pointed by dct_security */
        wiced_uart_transmit_bytes(WICED_UART_3,( "\nGATEWAY:\n" ),10);
        wiced_uart_transmit_bytes(WICED_UART_3,( "%s\n",(char*) ((dct_read_write_app_dct_t*)dct_app)->GATE ),strlen((((dct_read_write_app_dct_t*)dct_app)->GATE )));
        wiced_uart_transmit_bytes(WICED_UART_3,( "\nIP:\n" ),5);
        wiced_uart_transmit_bytes(WICED_UART_3,( "%s\n",(char*) ((dct_read_write_app_dct_t*)dct_app)->IP ),strlen((((dct_read_write_app_dct_t*)dct_app)->IP )));
        wiced_uart_transmit_bytes(WICED_UART_3,( "\nSERVER:\n" ),9);
        wiced_uart_transmit_bytes(WICED_UART_3,( "%s\n",(char*) ((dct_read_write_app_dct_t*)dct_app)->SERVER ),strlen((((dct_read_write_app_dct_t*)dct_app)->SERVER )));
        wiced_uart_transmit_bytes(WICED_UART_3,( "\nMASK:\n" ),7);
        wiced_uart_transmit_bytes(WICED_UART_3,( "%s\n",(char*) ((dct_read_write_app_dct_t*)dct_app)->MASK ),strlen((((dct_read_write_app_dct_t*)dct_app)->MASK )));
        wiced_uart_transmit_bytes(WICED_UART_3,( "\nSSID:\n" ),7);
        wiced_uart_transmit_bytes(WICED_UART_3,("%s\n\r",wifi_config->stored_ap_list[0].details.SSID.value),wifi_config->stored_ap_list[0].details.SSID.length);
        wiced_uart_transmit_bytes(WICED_UART_3,( "\nKEY:\n" ),6);
        wiced_uart_transmit_bytes(WICED_UART_3,("%s\n\r",wifi_config->stored_ap_list[0].security_key),wifi_config->stored_ap_list[0].security_key_length);

        wiced_uart_transmit_bytes(WICED_UART_3,( "MAC:\n" ),5);
        wiced_uart_transmit_bytes(WICED_UART_1,(("%X:%X:%X:%X:%X:%X\r\n",mac.octet[0], mac.octet[1], mac.octet[2],mac.octet[3], mac.octet[4], mac.octet[5])),30);

        sprintf(buffer_t,("%02X:%02X:%02X:%02X:%02X:%02X\r\n",mac.octet[0], mac.octet[1], mac.octet[2],mac.octet[3], mac.octet[4], mac.octet[5]));
       // wiced_uart_transmit_bytes(WICED_UART_1,buffer_t,strlen(buffer_t));
        /* Here ptr_is_writable should be same as what we passed during wiced_dct_read_lock() */
        wiced_dct_read_unlock( dct_app, WICED_FALSE );
        wiced_dct_read_unlock(wifi_config, WICED_FALSE);

        return WICED_SUCCESS;
}

static wiced_result_t Set_SSID(uint8_t *data,uint8_t len){
    //dct_read_write_app_dct_t*       app_dct                  = NULL;
    wiced_result_t res;
    platform_dct_wifi_config_t*  wifi_config;
   //Se genera un cadena sin signo para almancenar temporalmente la cadena que llega , quitando los primeros dos datos

    unsigned char str_r[len];
   strcpy(str_r,&data[2]);

   /* get the App config section for modifying, any memory allocation required would be done inside wiced_dct_read_lock() */
 //  wiced_dct_read_lock( (void**) &app_dct, WICED_TRUE, DCT_APP_SECTION, 0, sizeof( *app_dct ) );

   /* Modify string_var by writing the whole DCT */
   //strcpy( app_dct->SSID, str_r );
   // Get a copy of the WIFT config from the DCT into RAM
   wiced_dct_read_lock((void**) &wifi_config, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t));

   strcpy((char *) wifi_config->stored_ap_list[0].details.SSID.value, str_r);
   wifi_config->stored_ap_list[0].details.SSID.length = strlen(str_r);

   wiced_dct_write((const void *) wifi_config, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t));
   //wiced_dct_write( (const void*) app_dct, DCT_APP_SECTION, 0, sizeof(dct_read_write_app_dct_t) );
   /* release the read lock */
   //wiced_dct_read_unlock( app_dct, WICED_FALSE);

   wiced_dct_read_unlock(wifi_config, WICED_TRUE);


 /* Read & print all DCT sections to check that nothing has changed */
   return WICED_SUCCESS;
}
static wiced_result_t Set_KEY(uint8_t *data,uint8_t len){
    //dct_read_write_app_dct_t*       app_dct                  = NULL;
        wiced_result_t res;
        platform_dct_wifi_config_t*  wifi_config;
       //Se genera un cadena sin signo para almancenar temporalmente la cadena que llega , quitando los primeros dos datos

        unsigned char str_r[len];
       strcpy(str_r,&data[2]);

       /* get the App config section for modifying, any memory allocation required would be done inside wiced_dct_read_lock() */
     //  wiced_dct_read_lock( (void**) &app_dct, WICED_TRUE, DCT_APP_SECTION, 0, sizeof( *app_dct ) );

       /* Modify string_var by writing the whole DCT */
       //strcpy( app_dct->SSID, str_r );
       // Get a copy of the WIFT config from the DCT into RAM
       wiced_dct_read_lock((void**) &wifi_config, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t));

       strcpy((char *) wifi_config->stored_ap_list[0].security_key, str_r);
       wifi_config->stored_ap_list[0].security_key_length = strlen(str_r);

       wiced_dct_write((const void *) wifi_config, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t));
       //wiced_dct_write( (const void*) app_dct, DCT_APP_SECTION, 0, sizeof(dct_read_write_app_dct_t) );
       /* release the read lock */
       //wiced_dct_read_unlock( app_dct, WICED_FALSE);

       wiced_dct_read_unlock(wifi_config, WICED_TRUE);


     /* Read & print all DCT sections to check that nothing has changed */
       return WICED_SUCCESS;
}
static wiced_result_t Set_MASK(uint8_t *data,uint8_t len){
    dct_read_write_app_dct_t*       app_dct                  = NULL;

           unsigned char str_r[len];
           strcpy(str_r,&data[2]);

           /* get the App config section for modifying, any memory allocation required would be done inside wiced_dct_read_lock() */
           wiced_dct_read_lock( (void**) &app_dct, WICED_TRUE, DCT_APP_SECTION, 0, sizeof( *app_dct ) );

           /* Modify string_var by writing the whole DCT */
           strcpy( app_dct->MASK, str_r );

           wiced_dct_write( (const void*) app_dct, DCT_APP_SECTION, 0, sizeof(dct_read_write_app_dct_t) );
           /* release the read lock */
           wiced_dct_read_unlock( app_dct, WICED_FALSE);

         /* Read & print all DCT sections to check that nothing has changed */
           return WICED_SUCCESS;
}
static wiced_result_t Set_IP(uint8_t *data,uint8_t len){
    dct_read_write_app_dct_t*       app_dct                  = NULL;

           unsigned char str_r[len];
           strcpy(str_r,&data[2]);

           /* get the App config section for modifying, any memory allocation required would be done inside wiced_dct_read_lock() */
           wiced_dct_read_lock( (void**) &app_dct, WICED_TRUE, DCT_APP_SECTION, 0, sizeof( *app_dct ) );

           /* Modify string_var by writing the whole DCT */
           strcpy( app_dct->IP, str_r );

           wiced_dct_write( (const void*) app_dct, DCT_APP_SECTION, 0, sizeof(dct_read_write_app_dct_t) );
           /* release the read lock */
           wiced_dct_read_unlock( app_dct, WICED_FALSE);

         /* Read & print all DCT sections to check that nothing has changed */
           return WICED_SUCCESS;
}
static wiced_result_t Set_SERVER(uint8_t *data,uint8_t len){

    dct_read_write_app_dct_t*       app_dct                  = NULL;

           unsigned char str_r[len];
           strcpy(str_r,&data[2]);

           /* get the App config section for modifying, any memory allocation required would be done inside wiced_dct_read_lock() */
           wiced_dct_read_lock( (void**) &app_dct, WICED_TRUE, DCT_APP_SECTION, 0, sizeof( *app_dct ) );

           /* Modify string_var by writing the whole DCT */
           strcpy( app_dct->SERVER, str_r );

           wiced_dct_write( (const void*) app_dct, DCT_APP_SECTION, 0, sizeof(dct_read_write_app_dct_t) );
           /* release the read lock */
           wiced_dct_read_unlock( app_dct, WICED_FALSE);

         /* Read & print all DCT sections to check that nothing has changed */
           return WICED_SUCCESS;
}
static wiced_result_t Set_GATEWAY(uint8_t *data,uint8_t len){
    dct_read_write_app_dct_t*       app_dct                  = NULL;

           unsigned char str_r[len];
           strcpy(str_r,&data[2]);

           /* get the App config section for modifying, any memory allocation required would be done inside wiced_dct_read_lock() */
           wiced_dct_read_lock( (void**) &app_dct, WICED_TRUE, DCT_APP_SECTION, 0, sizeof( *app_dct ) );

           /* Modify string_var by writing the whole DCT */
           strcpy( app_dct->GATE, str_r );

           wiced_dct_write( (const void*) app_dct, DCT_APP_SECTION, 0, sizeof(dct_read_write_app_dct_t) );
           /* release the read lock */
           wiced_dct_read_unlock( app_dct, WICED_FALSE);

         /* Read & print all DCT sections to check that nothing has changed */
           return WICED_SUCCESS;
}
static wiced_result_t init_tcp_w(){
    dct_read_write_app_dct_t* dct_app = NULL;
    wiced_result_t result;
    char ip_a[4];
    char *p1;
    char *p2;
    char *p3;
    int x=0;
    char bb[15];


    if ( wiced_dct_read_lock( (void**) &dct_app, WICED_FALSE, DCT_APP_SECTION, 0, sizeof( *dct_app ) ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    char cad_aux[sizeof(dct_app->IP)];
    /* Modify string_var by writing the whole DCT */
    strcpy(cad_aux,(char*) (dct_read_write_app_dct_t*)dct_app->IP);

    //establece como  realizara el split
    char delim[] = ".";
    char *ptr1 = strtok(cad_aux, delim);

    while(ptr1 != NULL)
    {

        ri[x]= strtol(ptr1, &p1, 10);
        x++;
        ptr1=strtok(NULL, delim);
    }
    x=0;

    /* Modify string_var by writing the whole DCT */
    strcpy(cad_aux,(char*) (dct_read_write_app_dct_t*)dct_app->MASK);

    //establece como  realizara el split
    char *ptr2 = strtok(cad_aux, delim);

    while(ptr2 != NULL)
    {

        rn[x]= strtol(ptr2, &p2, 10);
        x++;
        ptr2=strtok(NULL, delim);
    }
    x=0;

    /* Modify string_var by writing the whole DCT */
    strcpy(cad_aux,(char*) (dct_read_write_app_dct_t*)dct_app->SERVER);

    //establece como  realizara el split
    char *ptr3 = strtok(cad_aux, delim);

    while(ptr3 != NULL)
    {

        rs[x]= strtol(ptr3, &p3, 10);
        x++;
        ptr3=strtok(NULL, delim);
    }
    x=0;



    d1= ((rg[0]<< 24) | (rg[1] << 16) | ( rg[2] << 8) | (rg[3]));
    d2= ((rn[0]<< 24) | (rn[1] << 16) | ( rn[2] << 8) | (rn[3]));
    d3= ((ri[0]<< 24) | (ri[1] << 16) | ( ri[2] << 8) | (ri[3]));
    d4= ((rs[0]<< 24) | (rs[1] << 16) | ( rs[2] << 8) | (rs[3]));

    wiced_dct_read_unlock( dct_app, WICED_FALSE);

    /* Read & print all DCT sections to check that nothing has changed */
    return WICED_SUCCESS;

}
static wiced_result_t start_tcp(){

}
static wiced_result_t tcp_client( void )
{
    wiced_tcp_socket_t socket;                      // The TCP socket
    wiced_tcp_stream_t stream;                      // The TCP stream
    char sendMessage[80];
    wiced_result_t result;
    wiced_mac_t myMac;
    wiced_ip_address_t myIpAddress;
    wl_bss_info_t ap_info_buffer;
    wiced_security_t ap_security;
     wiced_ip_address_t INITIALISER_IPV4_ADDRESS( server_ip_address, s1 );
    /* Lectura de ADC por un segundo, de lo contrario se toma el valor por defecta*/


    // Open the connection to the remote server via a Socket
    result = wiced_tcp_create_socket(&socket, WICED_STA_INTERFACE); //wiced_tcp_connect( &tcp_client_socket, &server_ip_address, TCP_SERVER_PORT, TCP_CLIENT_CONNECT_TIMEOUT );
    if ( result != WICED_SUCCESS )
    {
       wiced_uart_transmit_bytes(WICED_UART_3,(( "Failed create socket step 01 = [ %d ]\n",(int) result )),40);
        return WICED_ERROR;

    }

    result = wiced_tcp_bind( &socket, WICED_ANY_PORT ); /* Poner any port para que actualice el uerto de manera automatica */
    if(result!=WICED_SUCCESS)
    {
        wiced_uart_transmit_bytes(WICED_UART_3,( "Failed to bind socket step 02 = [%d]\n",(int) result ),40);
        wiced_tcp_delete_socket(&socket); /* Delete socket and return*/
        return WICED_ERROR;
    }

    result = wiced_tcp_connect(&socket,&server_ip_address,TCP_SERVER_PORT,2500); // 2 second timeout
    if ( result != WICED_SUCCESS )
    {
        wiced_uart_transmit_bytes(WICED_UART_3,( "Failed conect to server socket step 03 = [%d]\n", (int)result ),40);

        wiced_tcp_delete_socket(&socket);
        return WICED_ERROR;

    }



    // Format the data per the specification in section 6
    wiced_wifi_get_mac_address(&myMac);                             // Se obtiene la MAC del dispositivo
    wiced_ip_get_ipv4_address( WICED_STA_INTERFACE, &myIpAddress);  // Se obtiene la IP del dispositivo
    wwd_wifi_get_ap_info(&ap_info_buffer, &ap_security);            // Se obtiene la MAC de la red a la que estamos conectados
    //myDeviceId = myMac.octet[0] + myMac.octet[1] + myMac.octet[2] + myMac.octet[3] + myMac.octet[4] + myMac.octet[5];
    //myBssiId = ap_info_buffer.BSSID.octet[0] + ap_info_buffer.BSSID.octet[1] + ap_info_buffer.BSSID.octet[2] + ap_info_buffer.BSSID.octet[3] + ap_info_buffer.BSSID.octet[4] + ap_info_buffer.BSSID.octet[5];

    /*Generacion de cadena a enviar*/
    sprintf(sendMessage,"L;%02X:%02X:%02X:%02X:%02X:%02X,]%s,0000000000000%d%d%d0,%02X:%02X:%02X:%02X:%02X:%02X,%u.%u.%u.%u \r\n",
            myMac.octet[0],myMac.octet[1],myMac.octet[2],myMac.octet[3],myMac.octet[4],myMac.octet[5],mcu,GPIO_Evac,GPIO_Aux,GPIO_Recib,ap_info_buffer.BSSID.octet[0], ap_info_buffer.BSSID.octet[1],
            ap_info_buffer.BSSID.octet[2],ap_info_buffer.BSSID.octet[3],ap_info_buffer.BSSID.octet[4],ap_info_buffer.BSSID.octet[5],
            (uint8_t)(GET_IPV4_ADDRESS(myIpAddress) >> 24),
            (uint8_t)(GET_IPV4_ADDRESS(myIpAddress) >> 16),
            (uint8_t)(GET_IPV4_ADDRESS(myIpAddress) >> 8),
            (uint8_t)(GET_IPV4_ADDRESS(myIpAddress) >> 0)
            );

    // 5 is the register from the lab manual
    wiced_uart_transmit_bytes(WICED_UART_3,(("%s \r\n",sendMessage)),strlen(sendMessage)); // echo the message so that the user can see something
    // Initialize the TCP stream
    wiced_tcp_stream_init(&stream, &socket);

    // Send the data via the stream
    wiced_tcp_stream_write(&stream, sendMessage, strlen(sendMessage));
    // Force the data to be sent right away even if the packet isn't full yet
    wiced_tcp_stream_flush(&stream);

    // Get the response back from the WWEP server
    char rbuffer[30] = {0}; // The first 11 bytes of the buffer will be sent by the server. Byte 12 will stay 0 to null terminate the string
    result = wiced_tcp_stream_read(&stream, rbuffer, 29, TCP_CLIENT_RECEIVE_TIMEOUT); // Read 11 bytes from the buffer - wait up to 500ms for a response
    if(result == WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("Server Response=%s\n",rbuffer));
        //rxMessage = rbuffer;

    }
    else
    {
        WPRINT_APP_INFO(("Malformed response\n"));
        //return WICED_ERROR;
    }
    //wwd_wifi_get_ap_info/* Obtener informacion del AP*/

    // Delete the stream and socket
    wiced_tcp_stream_deinit(&stream);
    wiced_tcp_delete_socket(&socket);

    char* resultado;


    wiced_ip_address_t ipAddress;

    return WICED_SUCCESS;
}

static wiced_result_t  is_config(){
    dct_read_write_app_dct_t* dct_app = NULL;
    wiced_mac_t mac;

    if ( wiced_dct_read_lock( (void**) &dct_app, WICED_FALSE, DCT_APP_SECTION, 0, sizeof( *dct_app ) ) != WICED_SUCCESS )
    {
    return WICED_ERROR;
    }
    // Get a copy of the WIFT config from the DCT into RAM

    flag_tcp=(int)dct_app->F_config;

    wiced_dct_read_unlock( dct_app, WICED_FALSE );


return WICED_SUCCESS;
}

static wiced_result_t Set_config(){
    dct_read_write_app_dct_t*       app_dct                  = NULL;

    /* get the App config section for modifying, any memory allocation required would be done inside wiced_dct_read_lock() */
    wiced_dct_read_lock( (void**) &app_dct, WICED_TRUE, DCT_APP_SECTION, 0, sizeof( *app_dct ) );

    /* Modify string_var by writing the whole DCT */
        app_dct->F_config=1;
    wiced_dct_write( (const void*) app_dct, DCT_APP_SECTION, 0, sizeof(dct_read_write_app_dct_t) );
    /* release the read lock */
    wiced_dct_read_unlock( app_dct, WICED_FALSE);

    /* Read & print all DCT sections to check that nothing has changed */
    wiced_network_resume();
    return WICED_SUCCESS;
}

static wiced_result_t Un_Set_config(){

    dct_read_write_app_dct_t*       app_dct                  = NULL;

    /* get the App config section for modifying, any memory allocation required would be done inside wiced_dct_read_lock() */
    wiced_dct_read_lock( (void**) &app_dct, WICED_TRUE, DCT_APP_SECTION, 0, sizeof( *app_dct ) );

    /* Modify string_var by writing the whole DCT */
        app_dct->F_config=0;
    wiced_dct_write( (const void*) app_dct, DCT_APP_SECTION, 0, sizeof(dct_read_write_app_dct_t) );
    /* release the read lock */
    wiced_dct_read_unlock( app_dct, WICED_FALSE);

    /* Read & print all DCT sections to check that nothing has changed */
    return WICED_SUCCESS;


}

static wiced_result_t mcu_config (uint8_t *data,uint8_t len){
   unsigned char str_r[len];
   strcpy(str_r,&data[2]);
   sprintf(mcu,"%s",str_r);
}


