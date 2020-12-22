#include "user_config.h"		// 用户配置
#include "driver/uart.h"  		// 串口
#include "driver/oled.h"  		// OLED
#include "driver/dht11.h"		// DHT11头文件

#include "ets_sys.h"			// 回调函数
#include "osapi.h"				// os_XXX、软件定时器
#include "ip_addr.h"
#include "espconn.h"
#include "mem.h"				// 内存申请等函数
#include "user_interface.h"		// 系统接口、system_param_xxx接口、WIFI、RateContro
#include "smartconfig.h"		// 智能配网
#include "airkiss.h"

#define		ProjectName			"SmartConfig"		// 工程名宏定义
#define		Sector_STA_INFO		0x90				// 【STA参数】保存扇区
struct 		station_config 		STA_INFO;			// 【STA】参数结构体


os_timer_t OS_Timer_IP;					// 软件定时器
struct ip_info ST_ESP8266_IP;			// 8266的IP信息
u8 ESP8266_IP[4];						// 8266的IP地址
u8 C_LED_Flash = 0;						// LED闪烁计次

//按键定义
#define GPIO_KEY_NUM                            1
#define KEY_0_IO_MUX                            PERIPHS_IO_MUX_GPIO0_U
#define KEY_0_IO_NUM                            0
#define KEY_0_IO_FUNC                           FUNC_GPIO0


uint8_t lan_buf[200];
uint16_t lan_buf_len;
uint8 udp_sent_cnt = 0;

// 毫秒延时函数
void ICACHE_FLASH_ATTR delay_ms(u32 C_time)
{	for(;C_time>0;C_time--)
		os_delay_us(1000);
}

void ICACHE_FLASH_ATTR smartconfig_done(sc_status status, void *pdata) {

	switch (status) {

	//连接未开始，请勿在此阶段开始连接
	case SC_STATUS_WAIT:
		os_printf("SC_STATUS_WAIT\n");
		break;

	//发现信道
	case SC_STATUS_FIND_CHANNEL:
		os_printf("SC_STATUS_FIND_CHANNEL\n");
		break;


	//得到wifi名字和密码
	case SC_STATUS_GETTING_SSID_PSWD:
		os_printf("SC_STATUS_GETTING_SSID_PSWD\n");
		sc_type *type = pdata;
		if (*type == SC_TYPE_ESPTOUCH)
		{
			os_printf("SC_TYPE:SC_TYPE_ESPTOUCH\n");
		}
		else
		{
			os_printf("SC_TYPE:SC_TYPE_AIRKISS\n");
		}
		break;

	case SC_STATUS_LINK:
		os_printf("SC_STATUS_LINK\n");
		struct station_config *sta_conf = pdata;

        // 将【SSID】【PASS】保存到【外部Flash】中/
		spi_flash_erase_sector(Sector_STA_INFO);						// 擦除扇区
		spi_flash_write(Sector_STA_INFO*4096, (uint32 *)sta_conf, 96);	// 写入扇区/

		wifi_station_set_config(sta_conf);		// 设置STA参数【Flash】
		wifi_station_disconnect();				// 断开STA连接
		wifi_station_connect();					// ESP8266连接WIFI
		break;

	//成功获取到IP，连接路由完成。
	case SC_STATUS_LINK_OVER:
		os_printf("SC_STATUS_LINK_OVER \n\n");
		if (pdata != NULL)
		{
			wifi_get_ip_info(STATION_IF,&ST_ESP8266_IP);	// 获取8266_STA的IP地址

			ESP8266_IP[0] = ST_ESP8266_IP.ip.addr;		// IP地址高八位 == addr低八位
			ESP8266_IP[1] = ST_ESP8266_IP.ip.addr>>8;	// IP地址次高八位 == addr次低八位
			ESP8266_IP[2] = ST_ESP8266_IP.ip.addr>>16;	// IP地址次低八位 == addr次高八位
			ESP8266_IP[3] = ST_ESP8266_IP.ip.addr>>24;	// IP地址低八位 == addr高八位

			os_memcpy(ESP8266_IP, (uint8*) pdata, 4);
			os_printf("Phone ip: %d.%d.%d.%d\n", ESP8266_IP[0], ESP8266_IP[1],	ESP8266_IP[2], ESP8266_IP[3]);
			OLED_ShowIP(24,2,ESP8266_IP);	// OLED显示ESP8266的IP地址
			OLED_ShowString(0,4,"Connect to WIFI ");
			OLED_ShowString(0,6,"Successfully    ");
		}

		// 接入WIFI成功后，LED快闪3次
		//----------------------------------------------------
		for(; C_LED_Flash<=5; C_LED_Flash++)
		{
			GPIO_OUTPUT_SET(GPIO_ID_PIN(4),(C_LED_Flash%2));
			delay_ms(100);
		}

		os_timer_disarm(&OS_Timer_IP);	// 关闭定时器
		smartconfig_stop();				//停止配置

		os_printf("\r\n---- ESP8266 Connect to WIFI Successfully ----\r\n");
		break;
	}

}

// IP定时检查的回调函数
void ICACHE_FLASH_ATTR OS_Timer_IP_cb(void)
{
	u8 S_WIFI_STA_Connect;			// WIFI接入状态标志

	// 查询STA接入WIFI状态
	S_WIFI_STA_Connect = wifi_station_get_connect_status();

	// 成功接入WIFI
	if( S_WIFI_STA_Connect == STATION_GOT_IP )	// 判断是否获取IP
	{
		wifi_get_ip_info(STATION_IF,&ST_ESP8266_IP);	// 获取8266_STA的IP地址

		ESP8266_IP[0] = ST_ESP8266_IP.ip.addr;		// IP地址高八位 == addr低八位
		ESP8266_IP[1] = ST_ESP8266_IP.ip.addr>>8;	// IP地址次高八位 == addr次低八位
		ESP8266_IP[2] = ST_ESP8266_IP.ip.addr>>16;	// IP地址次低八位 == addr次高八位
		ESP8266_IP[3] = ST_ESP8266_IP.ip.addr>>24;	// IP地址低八位 == addr高八位

		// 显示ESP8266的IP地址
		os_printf("ESP8266_IP = %d.%d.%d.%d\n",ESP8266_IP[0],ESP8266_IP[1],ESP8266_IP[2],ESP8266_IP[3]);
		OLED_ShowIP(24,2,ESP8266_IP);	// OLED显示ESP8266的IP地址
		OLED_ShowString(0,4,"Connect to WIFI ");
		OLED_ShowString(0,6,"Successfully    ");

		// 接入WIFI成功后，LED快闪3次
		for(; C_LED_Flash<=5; C_LED_Flash++)
		{
			GPIO_OUTPUT_SET(GPIO_ID_PIN(4),(C_LED_Flash%2));
			delay_ms(100);
		}

		os_printf("\r\n---- ESP8266 Connect to WIFI Successfully ----\r\n");

		os_timer_disarm(&OS_Timer_IP);	// 关闭定时器

		// WIFI连接成功，执行后续功能。	如：SNTP/UDP/TCP/DNS等
	}

	// ESP8266无法连接WIFI
	else if(	S_WIFI_STA_Connect==STATION_NO_AP_FOUND 	||		// 未找到指定WIFI
				S_WIFI_STA_Connect==STATION_WRONG_PASSWORD 	||		// WIFI密码错误
				S_WIFI_STA_Connect==STATION_CONNECT_FAIL		)	// 连接WIFI失败
	{
		os_timer_disarm(&OS_Timer_IP);			// 关闭定时器

		os_printf("\r\n---- S_WIFI_STA_Connect=%d-----------\r\n",S_WIFI_STA_Connect);
		os_printf("\r\n---- ESP8266 Can't Connect to WIFI-----------\r\n");

		// 微信智能配网设置
		//wifi_set_opmode(STATION_MODE);			// 设为STA模式						//【第①步】
		smartconfig_set_type(SC_TYPE_AIRKISS); 	// ESP8266配网方式【AIRKISS】			//【第②步】
		smartconfig_start(smartconfig_done);	// 进入【智能配网模式】,并设置回调函数	//【第③步】
	}
}

// 软件定时器初始化(ms毫秒)
void ICACHE_FLASH_ATTR OS_Timer_IP_Init(u32 time_ms, u8 time_repetitive)
{

	os_timer_disarm(&OS_Timer_IP);	// 关闭定时器
	os_timer_setfn(&OS_Timer_IP,(os_timer_func_t *)OS_Timer_IP_cb, NULL);	// 设置定时器
	os_timer_arm(&OS_Timer_IP, time_ms, time_repetitive);  // 使能定时器
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
	OLED_ShowString(0,0,"ESP8266 = STA");		// |
	OLED_ShowString(0,2,"IP:");					// |
	OLED_ShowString(0,4,"WIFI Connecting ");	// |
	OLED_ShowString(0,6,"................");	// |

	// ESP8266读取【外部Flash】中的【STA参数】(SSID/PASS)，作为STA，连接WIFI
	os_memset(&STA_INFO, 0, sizeof(struct station_config));			// STA_INFO = 0
	spi_flash_read(Sector_STA_INFO*4096, (uint32 *)&STA_INFO, 96);	// 读出【STA参数】(SSID/PASS)
	STA_INFO.ssid[31] = 0;		// SSID最后添'\0'
	STA_INFO.password[63] = 0;	// APSS最后添'\0'
	os_printf("\r\nSTA_INFO.ssid=%s\r\nSTA_INFO.password=%s\r\n",STA_INFO.ssid,STA_INFO.password);

	wifi_set_opmode(0x01);					// 设置为STA模式，并保存到Flash
	wifi_station_set_config(&STA_INFO);		// 设置STA参数
	wifi_station_connect();					// ESP8266连接WIFI（这里，此句可省）

	OS_Timer_IP_Init(1000,1);				// 定时查询8266连接WIFI情况

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



