#include "user_main.h"
#include "user_config.h"		// 用户配置
#include "driver/uart.h"  		// 串口
#include "driver/oled.h"  		// OLED
#include "driver/dht11.h"		// DHT11头文件


#include "c_types.h"			// 变量类型
#include "eagle_soc.h"			// GPIO函数、宏定义
#include "ets_sys.h"			// 回调函数
#include "os_type.h"			// os_XXX
#include "osapi.h"  			// os_XXX、软件定时器
#include "gpio.h"				// gpio
#include "user_interface.h" 	// 系统接口、system_param_xxx接口、WIFI、RateContro

//全局变量
//===================================================================================
os_timer_t checkTimer_wifistate;						// 定义软件定时器OS_Timer_1必须定义为全局变量，因为ESP8266的内核还要使用
os_event_t *Pointer_Task_1;								// 第①步：定义任务指针   os_malloc函数的返回值
//===================================================================================

// 宏定义
//==================================================================================
#define		ProjectName			"PostAndGet"			// 工程名宏定义
#define		ESP8266_STA_SSID	"Tenda_5E"				// 创建的WIFI名
#define 	ESP6266_STA_PASS	"SQ123456"				// 创建的WIFI密码
//==================================================================================

// 毫秒延时函数
void ICACHE_FLASH_ATTR delay_ms(u32 C_time)
{	for(;C_time>0;C_time--)
		os_delay_us(1000);
}


// 软件定时器回调函数
void ICACHE_FLASH_ATTR OS_Timer_1_cb(void)
{
	u8 ESP8266_Connect_state;		// 8266连接WiFi状态
	struct ip_info ST_ESP8266_IP;	// IP信息结构体
	u8 ESP8266_IP[4] = {0};			// 点分十进制保存信息

	// 查询STA接入WIFI状态
	ESP8266_Connect_state = wifi_station_get_connect_status();

	//如果状态正确，证明已经成功连接到路由器
	if (ESP8266_Connect_state == STATION_GOT_IP)
	{
		os_printf("\r\n WIFI Connection successful! \r\n");
		os_timer_disarm(&checkTimer_wifistate);
		os_timer_disarm(&connect_timer);

		uint8 status = wifi_station_get_connect_status();
		if (status == STATION_GOT_IP)
		{
			os_printf("\r\n WIFI Connection successful! \r\n");
			startHttpQuestByGET(
					"https://api.seniverse.com/v3/weather/daily.json?key=rrpd2zmqkpwlsckt&location=guangzhou&language=en&unit=c&start=0&days=3");
			return;
		}
	}
}


void ICACHE_FLASH_ATTR user_init(void)
{
	/* 串口 */
	uart_init(74800,74800);				//设置串口1和串口2的波特率
	os_delay_us(10000);					// 等待串口稳定
	os_printf("\r\n=============================================================\r\n");
	os_printf("\t\tSDK	version:%s\r\n",system_get_sdk_version());	//串口打印SDK版本
	os_printf("\t\tProjectName :%s\r\n",ProjectName);
	os_printf("\t\thello world\r\n");
	os_printf("=============================================================\r\n");

	/* OLED屏初始化 */
	OLED_Init();								// OLED屏初始化

	wifi_set_opmode(0x01); //设置为STATION模式
	struct station_config stationConf;					// 设置STA参数结构体
	os_strcpy(stationConf.ssid, ESP8266_STA_SSID);	  	// 设置WiFi名称(将字符串复制到ssid数组)
	os_strcpy(stationConf.password, ESP6266_STA_PASS); 	// 设置WiFi密码(将字符串复制到password数组)
	wifi_station_set_config(&stationConf); 				// 设置WiFi station接口配置，并保存到 flash
	wifi_station_connect(); 							// 连接路由器

	/* 软件定时器 */
	os_timer_disarm(&checkTimer_wifistate);				//关闭定时器
	os_timer_setfn(&checkTimer_wifistate,(os_timer_func_t *)OS_Timer_1_cb, NULL);		//初始化软件定时器
	os_timer_arm(&checkTimer_wifistate, 1000, 1);		//使能软件定时器


	os_printf("\r\n-------------------- user_init OVER --------------------\r\n");
}


/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
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



