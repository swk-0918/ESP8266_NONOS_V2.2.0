#include "user_config.h"		// 用户配置
#include "driver/uart.h"  		// 串口
#include "driver/oled.h"  		// OLED
#include "driver/dht11.h"		// DHT11头文件

//#include "at_custom.h"
#include "c_types.h"			// 变量类型
#include "eagle_soc.h"			// GPIO函数、宏定义
#include "ip_addr.h"			// 被"espconn.h"使用。在"espconn.h"开头#include"ip_addr.h"或#include"ip_addr.h"放在"espconn.h"之前
#include "espconn.h"
//#include "espnow.h"
#include "ets_sys.h"			// 回调函数
//#include "gpio.h"				//
#include "mem.h"				// 内存申请函数
#include "os_type.h"			// os_XXX
#include "osapi.h"  			// os_XXX、软件定时器
#include "gpio.h"				// gpio
//#include "ping.h"
//#include "pwm.h"
//#include "queue.h"
//#include "smartconfig.h"
//#include "sntp.h"
//#include "spi_flash.h"
//#include "upgrade.h"
#include "user_interface.h" 	// 系统接口、system_param_xxx接口、WIFI、RateContro

//全局变量
//===================================================================================
os_timer_t OS_Timer_1;			// 定义软件定时器OS_Timer_1必须定义为全局变量，因为ESP8266的内核还要使用
os_event_t *Pointer_Task_1;		// 第①步：定义任务指针   os_malloc函数的返回值
//===================================================================================

// 宏定义
//==================================================================================
#define		ProjectName			"AP_Mode"		// 工程名宏定义
#define		ESP8266_AP_SSID		"ESP8266"	// 创建的WIFI名
#define 	ESP6266_AP_PASS		"123456789"	// 创建的WIFI密码
#define		LED_ON				GPIO_OUTPUT_SET(GPIO_ID_PIN(4),0)		// LED亮
#define		LED_OFF				GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1)		// LED灭
//==================================================================================

// 毫秒延时函数
void ICACHE_FLASH_ATTR delay_ms(u32 C_time)
{	for(;C_time>0;C_time--)
		os_delay_us(1000);
}

// 软件定时器回调函数
void ICACHE_FLASH_ATTR OS_Timer_1_cb(void)
{
	/* wifi AP 模式 */
	u8 number[128] = {0};			// 连接个数
	struct ip_info ST_ESP8266_IP;	// IP信息结构体
	u8 ESP8266_IP[4] = {0};			// 点分十进制保存信息

	// 查询并打印ESP8266的工作模式
	switch(wifi_get_opmode())
	{
		case 0x01: os_printf("\nESP8266_Mode = Station\n");break;
		case 0x02: os_printf("\nESP8266_Mode = SoftAP\n");break;
		case 0x03: os_printf("\nESP8266_Mode = Station + SoftAP\n");break;
	}

	// 获取ESP8266_AP模式下的IP地址  大端格式
	//【AP模式下，如果开启DHCP(默认)，并且未设置IP相关参数，ESP8266的IP地址=192.168.4.1】
	wifi_get_ip_info(SOFTAP_IF, &ST_ESP8266_IP);	// 获取查Wi-Fi接⼝或者 SoftAP 接⼝的 IP 地址
	ESP8266_IP[0] = ST_ESP8266_IP.ip.addr;			// 点分十进制IP的第一个数 <==> addr低八位
	ESP8266_IP[1] = ST_ESP8266_IP.ip.addr>>8;		// 点分十进制IP的第二个数 <==> addr次低八位
	ESP8266_IP[2] = ST_ESP8266_IP.ip.addr>>16;		// 点分十进制IP的第三个数 <==> addr次高八位
	ESP8266_IP[3] = ST_ESP8266_IP.ip.addr>>24;		// 点分十进制IP的第四个数 <==> addr高八位

	// 打印ESP8266的IP地址
	os_printf("ESP8266_IP = %d.%d.%d.%d\n",ESP8266_IP[0],ESP8266_IP[1],ESP8266_IP[2],ESP8266_IP[3]);
	OLED_ShowIP(24,2,ESP8266_IP);			// 显示ESP8266的IP地址

	// 查询并打印接入此WIFI的设备数量
	os_printf("Number of devices connected to this WIFI = %d\n",wifi_softap_get_station_num());

	os_sprintf(number, "connect number:%d",wifi_softap_get_station_num());
	OLED_ShowString(0,4,number);
}

/* 软件定时器初始化  */
void ICACHE_FLASH_ATTR OS_Timer_1_Init(u32 Timer_ms, u8 timer_repetition)
{
	os_timer_disarm(&OS_Timer_1);	//关闭定时器
	os_timer_setfn(&OS_Timer_1,(os_timer_func_t *)OS_Timer_1_cb, NULL);		//初始化软件定时器
	os_timer_arm(&OS_Timer_1, Timer_ms, timer_repetition);	//使能软件定时器
}

/* ESP8266 AP模式初始化 */
void ESP8266_AP_Init(void)
{
	struct	softap_config	AP_config;		// 设置AP参数结构体

	wifi_set_opmode(0x02);					// 设置WIFI工作模式为AP模式
	//结构体赋值(注意：【服务集标识符/密码】须设为字符串形式)
	os_memset(&AP_config, 0, sizeof(struct softap_config));		// 将AP_config清空
	ets_strcpy(AP_config.ssid, ESP8266_AP_SSID);				// 设置WiFi名称(将字符串复制到ssid数组)
	ets_strcpy(AP_config.password, ESP6266_AP_PASS);			// 设置WiFi密码(将字符串复制到password数组)
	AP_config.ssid_len = os_strlen(ESP8266_AP_SSID);			// wifi密码的长度
	AP_config.ssid_hidden = 0;									// 不隐藏SSID
	AP_config.authmode = AUTH_WPA2_PSK;           				// 设置加密模式
	AP_config.channel = 1;										// 通道号1～13
	AP_config.max_connection = 6;								// 最大连接个数
	AP_config.beacon_interval = 100;							// 信标间隔时槽100～60000 ms

	wifi_softap_set_config(&AP_config);							// 设置soft-AP，并保存到Flash
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

	/* 软件定时器 */
	OS_Timer_1_Init(1000, 1);				// 软件定时器初始化

	/* OLED屏初始化 */
	OLED_Init();							// |
	OLED_ShowString(0,0,"ESP8266 = AP");	// |
	OLED_ShowString(0,2,"IP:");				// |

	/* AP模式 */
	ESP8266_AP_Init();


	os_printf("\r\n-------------------- user_init OVER --------------------\r\n");

	//while(1)system_soft_wdt_feed();	//喂软件看⻔狗  防止复位 调试用
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



