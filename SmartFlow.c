/** @file
 *
 * TCP Client Powersave Application
 *
 * This application snippet demonstrates how to connect to a Wi-Fi
 * network and communicate with a TCP server. The application attempts
 * to save power between TCP connections to demonstrate low power.
 *
 * Features demonstrated
 *  - Wi-Fi client mode
 *  - DHCP client
 *  - TCP transmit and receive
 *
 * Application Instructions
 *   1. Modify the CLIENT_AP_SSID/CLIENT_AP_PASSPHRASE Wi-Fi credentials
 *      in the wifi_config_dct.h header file to match your Wi-Fi access point
 *   2. Ensure your computer is connected to the same Wi-Fi access point.
 *   3. Determine the computer's IP address for the Wi-Fi interface.
 *   4. Change the #define TCP_SERVER_IP_ADDRESS in the code below to match
 *      the computer's IP address
 *   5. Connect a PC terminal to the serial port of the WICED Eval board,
 *      then build and download the application as described in the WICED
 *      Quick Start Guide
 *   6. Ensure Python 2.7.x (*NOT* 3.x) is installed on your computer
 *   7. Open a command shell
 *   8. Run the python TCP echo server as follows from the tcp_client directory
 *      c:\<WICED-SDK>\Apps\snip\tcp_client_powersave> c:\path\to\Python27\python.exe tcp_echo_server.py
 *        - Ensure your firewall allows TCP for Python on port 50007
 *
 * Every TCP_CLIENT_INTERVAL seconds, the app establishes a connection
 * with the remote TCP server, sends a message and receives a response.
 *
 */

#include "wiced.h"
#include "string.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define TCP_PACKET_MAX_DATA_LENGTH        64
#define TCP_CLIENT_INTERVAL               2
#define TCP_SERVER_PORT                   8010
#define TCP_CLIENT_CONNECT_TIMEOUT        5000
#define TCP_CLIENT_RECEIVE_TIMEOUT        5000
#define TCP_CONNECTION_NUMBER_OF_RETRIES  3
#define RX_BUFFER_SIZE                    5
#define TIMER_TIME (8000)

/* Change the server IP address to match the TCP echo server address */
#define TCP_SERVER_IP_ADDRESS MAKE_IPV4_ADDRESS(172,168,100,49)


/******************************************************
 *                    Constants
 ******************************************************/

//static uint16_t myDeviceId; 					// A checksum of the MAC address
//static uint16_t myBssiId;                       // MAC address BSSI
static int GPIO_Evac = 0;
static int GPIO_Aux  = 0;
static int GPIO_Recib = 0;

/******************************************************
 *               Variable Definitions
 ******************************************************/
static const wiced_ip_setting_t device_init_ip_settings =
{
    INITIALISER_IPV4_ADDRESS( .ip_address, MAKE_IPV4_ADDRESS(10,10,85,100) ),
    INITIALISER_IPV4_ADDRESS( .netmask,    MAKE_IPV4_ADDRESS(255,255,248, 0) ),
    INITIALISER_IPV4_ADDRESS( .gateway,    MAKE_IPV4_ADDRESS(10,10,84,1) ),
};

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Static Function Declarations
 ******************************************************/

//static wiced_result_t tcp_client();
static wiced_result_t tcp_client( void );
static wiced_result_t wifi_init(void);
void button_B(void* arg);
void button_h(void* arg);

/******************************************************
 *               Variable Definitions
 ******************************************************/

static wiced_tcp_socket_t  socket;
uint32_t        start_time, end_time;
static wiced_bool_t button1_pressed = WICED_TRUE;
static wiced_bool_t EnableA1 = WICED_FALSE;

/******************************************************
 *               Function Definitions
 ******************************************************/

void application_start(void)
{
    wiced_result_t result;
    wiced_bool_t Ctr_Connection;
    /* Inicializar salida de leds */
    wiced_gpio_init( WICED_BUTTON1, INPUT_PULL_UP );
    wiced_gpio_init(WICED_LED1, OUTPUT_PUSH_PULL);
    wiced_gpio_output_high( WICED_LED1 );
    wiced_gpio_init(WICED_LED2, OUTPUT_PUSH_PULL);
    wiced_gpio_output_high( WICED_LED2 );
    wiced_gpio_input_irq_enable(WICED_BUTTON1, IRQ_TRIGGER_FALLING_EDGE, button_B, NULL); /* Setup interrupt */
    /* Initialize the device and WICED framework */
    wiced_init();

    while(1)

    {
        Ctr_Connection = WICED_TRUE;
        /* Inicializar red */
        result = wifi_init();
        if (result != WICED_SUCCESS)
        {
            WPRINT_APP_INFO(("Error en conexion AP \r\n"));
            Ctr_Connection = WICED_FALSE;
        }

        WPRINT_APP_INFO(("\n\rPing Period:  %d seconds, sleeping....\n\r", TCP_CLIENT_INTERVAL));

        if(Ctr_Connection == WICED_TRUE)
        {
            wiced_platform_mcu_enable_powersave();
        }
        WPRINT_APP_INFO(("Valor control %d \r\n",Ctr_Connection));
        while(Ctr_Connection == WICED_TRUE)
        {
            printf("Awake, starting ping...\n\r");
            WPRINT_APP_INFO(("Bandera ciclo principal\r\n"));
            result = tcp_client();
            if (result != WICED_SUCCESS)
            {
                Ctr_Connection = WICED_FALSE;
            }
            wiced_wifi_enable_powersave_with_throughput(40);
            /* Suspend networking activity while sleeping */
            wiced_network_suspend();
            wiced_rtos_delay_milliseconds( TCP_CLIENT_INTERVAL * SECONDS );
            wiced_network_resume();
        }
    }
}

static wiced_result_t wifi_init(void)
{
    wiced_result_t result;

    /* Set the beacon listen interval */
    wiced_wifi_set_listen_interval( 10, WICED_LISTEN_INTERVAL_TIME_UNIT_BEACON );

    /* Bring up the network interface */
    wiced_network_up( WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL );
    //result = wiced_network_up(WICED_STA_INTERFACE, WICED_USE_STATIC_IP, &device_init_ip_settings);
    if ( result != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ("Bringing up network interface failed !\n") );
        return WICED_ERROR;
    }
    /* Create a TCP socket */
    if (wiced_tcp_create_socket(&socket, WICED_STA_INTERFACE) != WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("TCP socket creation failed\r\n"));
        return WICED_ERROR;
    }
    //WPRINT_APP_INFO(("Bandera inicio de programa\r\n"));
    /* Bind to the socket */
    wiced_tcp_bind( &socket, TCP_SERVER_PORT );
    //wiced_tcp_connect(&socket,TCP_SERVER_IP_ADDRESS,TCP_SERVER_PORT,2000);
    return WICED_SUCCESS;
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
    const wiced_ip_address_t INITIALISER_IPV4_ADDRESS( server_ip_address, TCP_SERVER_IP_ADDRESS );
    /* Lectura de ADC por un segundo, de lo contrario se toma el valor por defecta*/

    //char receiveChar;
    //uint32_t expected_data_size = 4;

    /*if ( wiced_uart_receive_bytes( STDIO_UART, &receiveChar, &expected_data_size, 1000 ) == WICED_SUCCESS )
    {
        WPRINT_APP_INFO(("New ADC Value = %s\n\r",receiveChar));
    }
    WPRINT_APP_INFO(("Dato no actualizado \n\r"));
    */
    // Open the connection to the remote server via a Socket
    result = wiced_tcp_create_socket(&socket, WICED_STA_INTERFACE); //wiced_tcp_connect( &tcp_client_socket, &server_ip_address, TCP_SERVER_PORT, TCP_CLIENT_CONNECT_TIMEOUT );
    if ( result != WICED_SUCCESS )
    {
        WPRINT_APP_INFO(( "Failed create socket step 01 = [%d]\n", result ));
        return WICED_ERROR;
    }

    result = wiced_tcp_bind( &socket, WICED_ANY_PORT ); /* Poner any port para que actualice el uerto de manera automatica */
    if(result!=WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("Failed to bind socket step 02 %d\n",result));
        wiced_tcp_delete_socket(&socket); /* Delete socket and return*/
        return WICED_ERROR;
    }

    result = wiced_tcp_connect(&socket,&server_ip_address,TCP_SERVER_PORT,2500); // 2 second timeout
    if ( result != WICED_SUCCESS )
    {
        WPRINT_APP_INFO(( "Failed connect to server step 03= [%d]\n", result ));
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
    sprintf(sendMessage,"L;%02X:%02X:%02X:%02X:%02X:%02X,1536,0000000000000%d%d%d0,%02X:%02X:%02X:%02X:%02X:%02X,%u.%u.%u.%u \r\n",
            myMac.octet[0],myMac.octet[1],myMac.octet[2],myMac.octet[3],myMac.octet[4],myMac.octet[5],GPIO_Evac,GPIO_Aux,GPIO_Recib,ap_info_buffer.BSSID.octet[0], ap_info_buffer.BSSID.octet[1],
            ap_info_buffer.BSSID.octet[2],ap_info_buffer.BSSID.octet[3],ap_info_buffer.BSSID.octet[4],ap_info_buffer.BSSID.octet[5],
            (uint8_t)(GET_IPV4_ADDRESS(myIpAddress) >> 24),
            (uint8_t)(GET_IPV4_ADDRESS(myIpAddress) >> 16),
            (uint8_t)(GET_IPV4_ADDRESS(myIpAddress) >> 8),
            (uint8_t)(GET_IPV4_ADDRESS(myIpAddress) >> 0)
            );

    // 5 is the register from the lab manual
    WPRINT_APP_INFO(("%s \r\n",sendMessage)); // echo the message so that the user can see something
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
        wiced_gpio_output_low( WICED_LED1 );
        wiced_rtos_delay_milliseconds( 250 );
        /* LED ON for the shield (LED OFF if using the baseboard by itself) */
        wiced_gpio_output_high( WICED_LED1 );
        wiced_rtos_delay_milliseconds( 250 );
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

    /*Lectura de datos
     * A1 ----> Evacuacion, Despues de recibirlo levantar GPIO
     * N1 ----> Cancelacion, Despues de recibirlo limpiar GPIO
     * P1 ----> Verificar Socket
     * ACK----> Verificar Socket
     * DA ----> Cancela evacuacion
     * Funcionalidad de GPIO <Evacuacion><Auxilio><AcuseRecibeEvac><0>
     * */
    /* Se mejora el performance en la identificacion de palabras*/
    /* Hay que revisar lo GPIO que se utilizan ya que en este momento se esta habilitando el de auxilio*/
    /* A1 Procesp */
    char* resultado;
    if (EnableA1 == WICED_FALSE)
    {
        //char* resultado;
        resultado = strstr(rbuffer,"A1");   // Busca si existe la palabra A1
        if (resultado)
            {
                WPRINT_APP_INFO(("ID A1 \n"));
                GPIO_Evac = 1;                  // Cambio de estado de GPIO
                EnableA1 = WICED_TRUE;
            }
    }

    /* DA proceso */
    if (EnableA1 == WICED_TRUE)
    {
        //char* resultado;
        resultado = strstr(rbuffer,"DA");   // Busca si existe la palabra A1
        if (resultado)
            {
                WPRINT_APP_INFO(("ID DA \n"));
                GPIO_Evac = 0;                  // Cambio de estado de GPIO
                GPIO_Recib = 0;
                EnableA1 = WICED_FALSE;
            }
    }
    resultado = strstr(rbuffer,"N1");   // Busca si existe la palabra N1
    if(resultado)
    {
        WPRINT_APP_INFO(("ID N1 \n"));
        GPIO_Aux = 0;                  // Cambio de estado de GPIO
    }
    /*Otra forma de identificar palabras, es menos eficiente ya que solo identifica la primera*/
    char* P1_rx;
    int val = 0;
    P1_rx = strchr(rbuffer, 'P');
    val = strncmp(P1_rx, "P1", 2);
    if ( val == 0)
    {
        WPRINT_APP_INFO(("ID P1 \n"));
    }
    return WICED_SUCCESS;
}

void button_h(void* arg)
{
    wiced_gpio_input_irq_disable (WICED_BUTTON1);
    //wiced_rtos_delay_milliseconds(20);    /* Revisar si es necesaria esta parte en la practipa para evitar problemas con el boton */
    button1_pressed = wiced_gpio_input_get( WICED_BUTTON1 ) ? WICED_FALSE : WICED_TRUE;
    /* Toggle LED1 */
    if ( button1_pressed == WICED_FALSE )
    {
        wiced_time_get_time( & end_time);
        WPRINT_APP_INFO( ("Boton presionado por: %d ms\r\n", (int)( end_time - start_time ) ) );
        wiced_gpio_output_high( WICED_LED2 );
        if ((end_time-start_time) >= 5000)
        {
            WPRINT_APP_INFO( ("Ayuda \r\n" ));
            GPIO_Aux = 1;
        }
        wiced_gpio_input_irq_enable(WICED_BUTTON1, IRQ_TRIGGER_FALLING_EDGE, button_B, NULL);
    }
    else
    {
        wiced_gpio_input_irq_enable(WICED_BUTTON1, IRQ_TRIGGER_RISING_EDGE, button_h, NULL);
        WPRINT_APP_INFO( ("Falsa Alarma, interrupcion habilitada \r\n"));
    }
    return;
}
void button_B(void* arg)
{
    wiced_gpio_input_irq_disable (WICED_BUTTON1);
    //wiced_rtos_delay_milliseconds(20); /* Revisar si es necesaria esta parte en la practipa para evitar problemas con el boton */
    button1_pressed = wiced_gpio_input_get( WICED_BUTTON1 ) ? WICED_FALSE : WICED_TRUE;
    /* Toggle LED1 */
    if ( button1_pressed == WICED_TRUE )
    {
        if (EnableA1 == WICED_TRUE)
        {
            GPIO_Recib = 1;
        }
        wiced_time_get_time( &start_time );
        wiced_gpio_output_low( WICED_LED2 );
        wiced_gpio_input_irq_enable(WICED_BUTTON1, IRQ_TRIGGER_RISING_EDGE, button_h, NULL);
    }
    else
    {
        wiced_gpio_input_irq_enable(WICED_BUTTON1, IRQ_TRIGGER_FALLING_EDGE, button_B, NULL); /* Setup interrupt */
        WPRINT_APP_INFO( ("Falsa Alarma, interrupcion habilitada \r\n"));
    }
    return;
}

