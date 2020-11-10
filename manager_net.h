
#ifndef _MANAGER_NET_H
#define _MANAGER_NET_H

#include "wiced.h"
#include "string.h"
#include "stdbool.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"


static wiced_result_t Set_SSID(uint8_t *data,uint8_t len){
    //dct_read_write_app_dct_t*       app_dct                  = NULL;
    wiced_result_t res;
    platform_dct_wifi_config_t*  wifi_config;
   //Se genera un cadena sin signo para almancenar temporalmente la cadena que llega , quitando los primeros dos datos

   unsigned char str_r[len];
   strcpy(str_r,&data[2]);

   // Get a copy of the WIFT config from the DCT into RAM
   wiced_dct_read_lock((void**) &wifi_config, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t));

   strcpy((char *) wifi_config->stored_ap_list[0].details.SSID.value, str_r);
   wifi_config->stored_ap_list[0].details.SSID.length = strlen(str_r);

   res=wiced_dct_write((const void *) wifi_config, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t));
   char mensage[30];
   sprintf(mensage,"W_SSID: %s",str_r);
   if(res == WICED_SUCCESS){
       wiced_uart_transmit_bytes( WICED_UART_3,mensage, strlen(mensage));
     wiced_uart_transmit_bytes( WICED_UART_3, ("\r"),1);
       return WICED_SUCCESS;
     }

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
       strncpy(str_r,&data[2],len);
       // Get a copy of the WIFT config from the DCT into RAM
       wiced_dct_read_lock((void**) &wifi_config, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t));

       strcpy((char *) wifi_config->stored_ap_list[0].security_key, str_r);
       wifi_config->stored_ap_list[0].security_key_length = strlen(str_r);

      res= wiced_dct_write((const void *) wifi_config, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t));
       char mensage[30];
       sprintf(mensage,"W_KEY: %s",str_r);
       if(res == WICED_SUCCESS){
           wiced_uart_transmit_bytes( WICED_UART_3,mensage, strlen(mensage));
         wiced_uart_transmit_bytes( WICED_UART_3, ("\r"),1);
           return WICED_SUCCESS;
         }
       wiced_dct_read_unlock(wifi_config, WICED_TRUE);


     /* Read & print all DCT sections to check that nothing has changed */
       return WICED_SUCCESS;
}
static wiced_result_t Set_MASK(uint8_t *data,uint8_t len){
        dct_read_write_app_dct_t*       app_dct                  = NULL;
        wiced_result_t res;

        unsigned char str_r[len];
        strcpy(str_r,&data[2]);

        /* get the App config section for modifying, any memory allocation required would be done inside wiced_dct_read_lock() */
        wiced_dct_read_lock( (void**) &app_dct, WICED_TRUE, DCT_APP_SECTION, 0, sizeof( *app_dct ) );
        /* Modify string_var by writing the whole DCT */
        strcpy( app_dct->MASK, str_r );
        res=wiced_dct_write( (const void*) app_dct, DCT_APP_SECTION, 0, sizeof(dct_read_write_app_dct_t) );
        char mensage[30];
        sprintf(mensage,"W_MASK: %s",str_r);
        if(res == WICED_SUCCESS){
            wiced_uart_transmit_bytes( WICED_UART_3,mensage, strlen(mensage));
          wiced_uart_transmit_bytes( WICED_UART_3, ("\r"),1);
            return WICED_SUCCESS;
          }
        wiced_dct_read_unlock( app_dct, WICED_FALSE);

        /* Read & print all DCT sections to check that nothing has changed */

}
static wiced_result_t print_app_dct( void ){
        dct_read_write_app_dct_t* dct_app = NULL;
        wiced_mac_t  mac;
        platform_dct_wifi_config_t*  wifi_config;

        if ( wiced_dct_read_lock( (void**) &dct_app, WICED_FALSE, DCT_APP_SECTION, 0, sizeof( *dct_app ) ) != WICED_SUCCESS )
        {
            return WICED_ERROR;
        }
        // Get a copy of the WIFT config from the DCT into RAM
        wiced_dct_read_lock((void**) &wifi_config, WICED_FALSE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t));



        /* since we passed ptr_is_writable as WICED_FALSE, we are not allowed to write in to memory pointed by dct_security */
        wiced_uart_transmit_bytes(WICED_UART_3,( "\nGATEWAY: " ),10);
        wiced_uart_transmit_bytes(WICED_UART_3,( "%s\n",(char*) ((dct_read_write_app_dct_t*)dct_app)->GATE ),strlen((((dct_read_write_app_dct_t*)dct_app)->GATE )));
        wiced_uart_transmit_bytes(WICED_UART_3,( "\nIP: " ),5);
        wiced_uart_transmit_bytes(WICED_UART_3,( "%s\n",(char*) ((dct_read_write_app_dct_t*)dct_app)->IP ),strlen((((dct_read_write_app_dct_t*)dct_app)->IP )));
        wiced_uart_transmit_bytes(WICED_UART_3,( "\nSERVER: " ),9);
        wiced_uart_transmit_bytes(WICED_UART_3,( "%s\n",(char*) ((dct_read_write_app_dct_t*)dct_app)->SERVER ),strlen((((dct_read_write_app_dct_t*)dct_app)->SERVER )));
        wiced_uart_transmit_bytes(WICED_UART_3,( "\nMASK: " ),7);
        wiced_uart_transmit_bytes(WICED_UART_3,( "%s\n",(char*) ((dct_read_write_app_dct_t*)dct_app)->MASK ),strlen((((dct_read_write_app_dct_t*)dct_app)->MASK )));
        wiced_uart_transmit_bytes(WICED_UART_3,( "\nSSID: " ),7);
        wiced_uart_transmit_bytes(WICED_UART_3,("%s\n\r",wifi_config->stored_ap_list[0].details.SSID.value),wifi_config->stored_ap_list[0].details.SSID.length);
        wiced_uart_transmit_bytes(WICED_UART_3,( "\nKEY: " ),6);
        wiced_uart_transmit_bytes(WICED_UART_3,("%s\n\r",wifi_config->stored_ap_list[0].security_key),wifi_config->stored_ap_list[0].security_key_length);
        //wiced_uart_transmit_bytes(WICED_UART_3,( "\nMAC: " ),6);

        if ( wwd_wifi_get_mac_address( &mac, WWD_STA_INTERFACE ) == WWD_SUCCESS )
        {
         //   sprintf(buffer_t,("%02B:%02B:%02B:%02B:%02B:%02B\r\n",mac.octet[0], mac.octet[1], mac.octet[2],mac.octet[3], mac.octet[4], mac.octet[5]));
           //        wiced_uart_transmit_bytes(WICED_UART_3,("%s \n\r",buffer_t),strlen(buffer_t));
        }

        wiced_uart_transmit_bytes(WICED_UART_3,( "     \n" ),6);



        /* Here ptr_is_writable should be same as what we passed during wiced_dct_read_lock() */
        wiced_dct_read_unlock( dct_app, WICED_FALSE );
        wiced_dct_read_unlock(wifi_config, WICED_FALSE);

        return WICED_SUCCESS;
}
static wiced_result_t Set_IP(uint8_t *data,uint8_t len){
    dct_read_write_app_dct_t*       app_dct                  = NULL;
    wiced_result_t res;
           unsigned char str_r[len];
           strcpy(str_r,&data[2]);
           /* get the App config section for modifying, any memory allocation required would be done inside wiced_dct_read_lock() */
           wiced_dct_read_lock( (void**) &app_dct, WICED_TRUE, DCT_APP_SECTION, 0, sizeof( *app_dct ) );

           /* Modify string_var by writing the whole DCT */
           strcpy( app_dct->IP, str_r );

           res=wiced_dct_write( (const void*) app_dct, DCT_APP_SECTION, 0, sizeof(dct_read_write_app_dct_t) );
           char mensage[30];
           sprintf(mensage,"W_IP: %s",str_r);
           if(res == WICED_SUCCESS){
               wiced_uart_transmit_bytes( WICED_UART_3,mensage, strlen(mensage));
               wiced_uart_transmit_bytes( WICED_UART_3, ("\r"),1);
               return WICED_SUCCESS;
             }

           /* release the read lock */
           wiced_dct_read_unlock( app_dct, WICED_FALSE);

         /* Read & print all DCT sections to check that nothing has changed */
           return WICED_SUCCESS;
}
static wiced_result_t Set_SERVER(uint8_t *data,uint8_t len){

    dct_read_write_app_dct_t*       app_dct                  = NULL;
    wiced_result_t res;
           unsigned char str_r[len];
           strcpy(str_r,&data[2]);

           /* get the App config section for modifying, any memory allocation required would be done inside wiced_dct_read_lock() */
           wiced_dct_read_lock( (void**) &app_dct, WICED_TRUE, DCT_APP_SECTION, 0, sizeof( *app_dct ) );

           /* Modify string_var by writing the whole DCT */
           strcpy( app_dct->SERVER, str_r );

           res=wiced_dct_write( (const void*) app_dct, DCT_APP_SECTION, 0, sizeof(dct_read_write_app_dct_t) );
           char mensage[30];
           sprintf(mensage,"W_SERVER: %s",str_r);
           if(res == WICED_SUCCESS){
               wiced_uart_transmit_bytes( WICED_UART_3,mensage, strlen(mensage));
             wiced_uart_transmit_bytes( WICED_UART_3, ("\r"),1);
               return WICED_SUCCESS;
             }
           /* release the read lock */
           wiced_dct_read_unlock( app_dct, WICED_FALSE);

         /* Read & print all DCT sections to check that nothing has changed */
           return WICED_SUCCESS;
}
static wiced_result_t Set_GATEWAY(uint8_t *data,uint8_t len){
    dct_read_write_app_dct_t*       app_dct                  = NULL;
    wiced_result_t res;
           unsigned char str_r[len];
           strcpy(str_r,&data[2]);
           /* get the App config section for modifying, any memory allocation required would be done inside wiced_dct_read_lock() */
           wiced_dct_read_lock( (void**) &app_dct, WICED_TRUE, DCT_APP_SECTION, 0, sizeof( *app_dct ) );

           /* Modify string_var by writing the whole DCT */
           strcpy( app_dct->GATE, str_r );

           res=wiced_dct_write( (const void*) app_dct, DCT_APP_SECTION, 0, sizeof(dct_read_write_app_dct_t) );
           char mensage[30];
           sprintf(mensage,"W_GATEWAY: %s",str_r);
           if(res == WICED_SUCCESS){
               wiced_uart_transmit_bytes( WICED_UART_3,mensage, strlen(mensage));
             wiced_uart_transmit_bytes( WICED_UART_3, ("\r"),1);
               return WICED_SUCCESS;
             }
           /* release the read lock */
           wiced_dct_read_unlock( app_dct, WICED_FALSE);

         /* Read & print all DCT sections to check that nothing has changed */
           return WICED_SUCCESS;
}


#endif  /* stdbool.h */
