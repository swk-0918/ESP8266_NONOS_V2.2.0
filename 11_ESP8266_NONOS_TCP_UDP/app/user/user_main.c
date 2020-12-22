#include "user_config.h"		// 用户配置
#include "driver/uart.h"  		// 串口
#include "driver/dht11.h"		// DHT11头文件

void ICACHE_FLASH_ATTR user_init(void)
{
	/* 串口 */
	uart_init(74800,74800);				//设置串口1和串口2的波特率
	os_delay_us(10000);					// 等待串口稳定

	/* 以下8个函数同时只能运行一个 */
	//AP_UDP_Servic_Init();		// ESP8266设置为AP模式并作为Server端用UDP与Client通信
	//AP_UDP_Client_Init();		// ESP8266设置为AP模式并作为Client端用UDP与Server通信
	//AP_TCP_Servic_Init();		// ESP8266设置为AP模式并作为Server端用TCP与Client通信
	//AP_TCP_Client_Init();		// ESP8266设置为AP模式并作为Client端用TCP与Server通信

	//STA_UDP_Servic_Init();	// ESP8266设置为STA模式并作为Server端用UDP与Client通信
	//STA_UDP_Client_Init();	// ESP8266设置为STA模式并作为Client端用UDP与Server通信
	//STA_TCP_Servic_Init();	// ESP8266设置为STA模式并作为Server端用TCP与Client通信
	STA_TCP_Client_Init();	// ESP8266设置为STA模式并作为Client端用TCP与Server通信


	os_printf("\r\n-------------------- user_init OVER --------------------\r\n");
}


uint32 ICACHE_FLASH_ATTR user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
            rf_cal_sec = 512 - 5;
            break;
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
            rf_cal_sec = 1024 - 5;
            break;
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

void ICACHE_FLASH_ATTR
user_rf_pre_init(void){}



