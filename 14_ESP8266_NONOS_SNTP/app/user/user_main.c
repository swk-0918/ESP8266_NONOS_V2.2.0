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
#include "sntp.h"
//#include "spi_flash.h"
//#include "upgrade.h"
#include "user_interface.h" 	// 系统接口、system_param_xxx接口、WIFI、RateContro

//全局变量
//===================================================================================
os_timer_t OS_Timer_1;			// 定义软件定时器OS_Timer_1必须定义为全局变量，因为ESP8266的内核还要使用
os_event_t *Pointer_Task_1;		// 第①步：定义任务指针   os_malloc函数的返回值

u8 C_Read_DHT11 = 0;			// 读取DHT11计时

os_timer_t OS_Timer_SNTP;		// 定时器_SNTP
//===================================================================================

// 宏定义
//==================================================================================
#define		ProjectName			"STA_Mode"		// 工程名宏定义
#define		ESP8266_STA_SSID	"Tenda_5E"		// 创建的WIFI名
#define 	ESP6266_STA_PASS	"SQ123456"		// 创建的WIFI密码
#define		LED_ON				GPIO_OUTPUT_SET(GPIO_ID_PIN(4),0)		// LED亮
#define		LED_OFF				GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1)		// LED灭
//==================================================================================

// 毫秒延时函数
void ICACHE_FLASH_ATTR delay_ms(u32 C_time)
{	for(;C_time>0;C_time--)
		os_delay_us(1000);
}

/* SNTP定时回调函数 */
void ICACHE_FLASH_ATTR OS_Timer_SNTP_Fun(void)
{
	// 字符串整理 相关变量
	u8 C_Str = 0;						// 字符串字节计数
	char A_Str_Data[20] = {0};			// 【"日期"】字符串数组
	char *T_A_Str_Data = A_Str_Data;	// 缓存数组指针
	char A_Str_Clock[10] = {0};			// 【"时间"】字符串数组
	char * Str_Head_Week;				// 【"星期"】字符串首地址
	char * Str_Head_Month;				// 【"月份"】字符串首地址
	char * Str_Head_Day;				// 【"日数"】字符串首地址
	char * Str_Head_Clock;				// 【"时钟"】字符串首地址
	char * Str_Head_Year;				// 【"年份"】字符串首地址

	 uint32	TimeStamp;		// 时间戳
	 char * Str_RealTime;	// 实际时间的字符串

	 // 查询当前距离基准时间(1970.01.01 00:00:00 GMT+8)的时间戳(单位:秒)
	 TimeStamp = sntp_get_current_timestamp();

	 if(TimeStamp)		// 判断是否获取到偏移时间
	 {
		 //os_timer_disarm(&OS_Timer_SNTP);	// 关闭SNTP定时器

		 // 查询实际时间(GMT+8):东八区(北京时间)
		 Str_RealTime = sntp_get_real_time(TimeStamp);


		 // 【实际时间】字符串 == "周 月 日 时:分:秒 年"
		 os_printf("\r\n----------------------------------------------------\r\n");
		 os_printf("SNTP_TimeStamp = %d\r\n",TimeStamp);		// 时间戳
		 os_printf("\r\nSNTP_InternetTime = %s",Str_RealTime);	// 实际时间
		 os_printf("--------------------------------------------------------\r\n");


		 // 时间字符串整理，OLED显示【"日期"】、【"时间"】字符串
		 // 【"年份" + ' '】填入日期数组
		 Str_Head_Year = Str_RealTime;	// 设置起始地址

		 while( *Str_Head_Year )		// 找到【"实际时间"】字符串的结束字符'\0'
			 Str_Head_Year ++ ;

		 // 【注：API返回的实际时间字符串，最后还有一个换行符，所以这里 -5】
		 Str_Head_Year -= 5 ;			// 获取【"年份"】字符串的首地址

		 T_A_Str_Data[4] = ' ' ;
		 os_memcpy(T_A_Str_Data, Str_Head_Year, 4);		// 【"年份" + ' '】填入日期数组

		 T_A_Str_Data += 5;				// 指向【"年份" + ' '】字符串的后面的地址

		 // 获取【日期】字符串的首地址
		 Str_Head_Week 	= Str_RealTime;							// "星期" 字符串的首地址
		 Str_Head_Month = os_strstr(Str_Head_Week,	" ") + 1;	// "月份" 字符串的首地址
		 Str_Head_Day 	= os_strstr(Str_Head_Month,	" ") + 1;	// "日数" 字符串的首地址
		 Str_Head_Clock = os_strstr(Str_Head_Day,	" ") + 1;	// "时钟" 字符串的首地址


		 // 【"月份" + ' '】填入日期数组
		 C_Str = Str_Head_Day - Str_Head_Month;				// 【"月份" + ' '】的字节数

		 os_memcpy(T_A_Str_Data, Str_Head_Month, C_Str);	// 【"月份" + ' '】填入日期数组

		 T_A_Str_Data += C_Str;		// 指向【"月份" + ' '】字符串的后面的地址


		 // 【"日数" + ' '】填入日期数组
		 C_Str = Str_Head_Clock - Str_Head_Day;				// 【"日数" + ' '】的字节数

		 os_memcpy(T_A_Str_Data, Str_Head_Day, C_Str);		// 【"日数" + ' '】填入日期数组

		 T_A_Str_Data += C_Str;		// 指向【"日数" + ' '】字符串的后面的地址


		 // 【"星期" + ' '】填入日期数组
		 C_Str = Str_Head_Month - Str_Head_Week - 1;		// 【"星期"】的字节数

		 os_memcpy(T_A_Str_Data, Str_Head_Week, C_Str);		// 【"星期"】填入日期数组

		 T_A_Str_Data += C_Str;		// 指向【"星期"】字符串的后面的地址


		 // OLED显示【"日期"】、【"时钟"】字符串
		 *T_A_Str_Data = '\0';		// 【"日期"】字符串后面添加'\0'

		 OLED_ShowString(0,0,A_Str_Data);		// OLED显示日期


		 os_memcpy(A_Str_Clock, Str_Head_Clock, 8);		// 【"时钟"】字符串填入时钟数组
		 A_Str_Clock[8] = '\0';

		 OLED_ShowString(64,2,A_Str_Clock);		// OLED显示时间
	 }

	// 每2秒，读取/显示温湿度数据
	C_Read_DHT11 ++ ;		// 读取DHT11计时

	if(C_Read_DHT11>=2)		// 2秒计时
	{
		C_Read_DHT11 = 0;	// 计时=0

		if(DHT11_Read_Data_Complete() == 0)		// 读取DHT11温湿度
		{
			DHT11_NUM_Char();	// DHT11数据值转成字符串

			OLED_ShowString(64,4,DHT11_Data_Char[1]);	// DHT11_Data_Char[0] == 【温度字符串】
			OLED_ShowString(64,6,DHT11_Data_Char[0]);	// DHT11_Data_Char[1] == 【湿度字符串】
		}

		else
		{
			OLED_ShowString(64,4,"----");	// Temperature：温度
			OLED_ShowString(64,6,"----");	// Humidity：湿度
		}
	}
}

/* SNTP定时初始化 */
void ICACHE_FLASH_ATTR OS_Timer_SNTP_Init(u32 time_ms, u8 time_repetitive)
{
	os_timer_disarm(&OS_Timer_SNTP);							// 取消定时器定时
	os_timer_setfn(&OS_Timer_SNTP,(os_timer_func_t *)OS_Timer_SNTP_Fun,NULL);	//设置定时器回调函数
	os_timer_arm(&OS_Timer_SNTP, time_ms, time_repetitive);		// 使能软件定时器
}

/* 初始化SNTP */
void ICACHE_FLASH_ATTR ESP8266_SNTP_Init(void)
{
	ip_addr_t * addr = (ip_addr_t *)os_zalloc(sizeof(ip_addr_t));

	sntp_setservername(0, "us.pool.ntp.org");	// 服务器_0【域名】
	sntp_setservername(1, "ntp.sjtu.edu.cn");	// 服务器_1【域名】

	ipaddr_aton("210.72.145.44", addr);			// 点分十进制 => 32位二进制
	sntp_setserver(2, addr);					// 服务器_2【IP地址】
	os_free(addr);								// 释放addr

	sntp_init();	// SNTP初始化API

	OS_Timer_SNTP_Init(1000,1);					// 1秒重复定时(SNTP)
}

// 软件定时器回调函数
void ICACHE_FLASH_ATTR OS_Timer_1_cb(void)
{
	/* wifi STA 模式 */
	u8 LED_Count;					// LED闪烁
	u8 ESP8266_Connect_state;		// 8266连接WiFi状态
	struct ip_info ST_ESP8266_IP;	// IP信息结构体
	u8 ESP8266_IP[4] = {0};			// 点分十进制保存信息

	// 查询STA接入WIFI状态
	ESP8266_Connect_state = wifi_station_get_connect_status();
	//---------------------------------------------------
	// Station连接状态表
	// 0 == STATION_IDLE -------------- STATION闲置
	// 1 == STATION_CONNECTING -------- 正在连接WIFI
	// 2 == STATION_WRONG_PASSWORD ---- WIFI密码错误
	// 3 == STATION_NO_AP_FOUND ------- 未发现指定WIFI
	// 4 == STATION_CONNECT_FAIL ------ 连接失败
	// 5 == STATION_GOT_IP ------------ 获得IP，连接成功
	//---------------------------------------------------
	switch(ESP8266_Connect_state)
	{
		case 0 : 	os_printf("\nSTATION_IDLE \n");				break;
		case 1 : 	os_printf("\nSTATION_CONNECTING \n");		break;
		case 2 : 	os_printf("\nSTATION_WRONG_PASSWORD \n");	break;
		case 3 : 	os_printf("\nSTATION_NO_AP_FOUND \n");		break;
		case 4 : 	os_printf("\nSTATION_CONNECT_FAIL \n");		break;
		case 5 : 	os_printf("\nSTATION_GOT_IP \n");			break;
	}

	// 成功接入WIFI【STA模式下，如果开启DHCP(默认)，则ESO8266的IP地址由WIFI路由器自动分配】
	if(ESP8266_Connect_state == STATION_GOT_IP)
	{
		// 获取ESP8266_AP模式下的IP地址  大端格式
		//DHCP-Client默认开启，ESP8266接入WIFI后，由路由器分配IP地址，IP地址不确定
		wifi_get_ip_info(STATION_IF, &ST_ESP8266_IP);	// 获取查Wi-Fi接⼝或者 SoftAP 接⼝的 IP 地址

		// ESP8266_AP_IP.ip.addr是32位二进制代码，转换为点分十进制形式
		ESP8266_IP[0] = ST_ESP8266_IP.ip.addr;		// IP地址高八位 == addr低八位
		ESP8266_IP[1] = ST_ESP8266_IP.ip.addr>>8;	// IP地址次高八位 == addr次低八位
		ESP8266_IP[2] = ST_ESP8266_IP.ip.addr>>16;	// IP地址次低八位 == addr次高八位
		ESP8266_IP[3] = ST_ESP8266_IP.ip.addr>>24;	// IP地址低八位 == addr高八位

		// 打印ESP8266的IP地址
		//os_printf("ESP8266_IP = %d.%d.%d.%d\n",ESP8266_IP[0],ESP8266_IP[1],ESP8266_IP[2],ESP8266_IP[3]);
		//OLED_ShowIP(24,2,ESP8266_IP);			// 显示ESP8266的IP地址

		for(LED_Count=0; LED_Count<=5; LED_Count++)
		{
			GPIO_OUTPUT_SET(GPIO_ID_PIN(4),(LED_Count%2));
			delay_ms(200);
		}
		os_timer_disarm(&OS_Timer_1);	// 关闭定时器

		ESP8266_SNTP_Init();			// 初始化SNTP
	}
}

/* 软件定时器初始化  */
void ICACHE_FLASH_ATTR OS_Timer_1_Init(u32 Timer_ms, u8 timer_repetition)
{
	os_timer_disarm(&OS_Timer_1);	//关闭定时器
	os_timer_setfn(&OS_Timer_1,(os_timer_func_t *)OS_Timer_1_cb, NULL);		//初始化软件定时器
	os_timer_arm(&OS_Timer_1, Timer_ms, timer_repetition);	//使能软件定时器
}

/* ESP8266 STA模式初始化 */
void ESP8266_STA_Init(void)
{
	struct	station_config	STA_config;		// 设置STA参数结构体
	struct  ip_info         ST_ESP8266_IP;	// STA信息结构体

	// 设置ESP8266的工作模式
	wifi_set_opmode(0x01);					// 设置WIFI工作模式为STA模式

	// 设置STA模式下的IP地址【ESP8266默认开启DHCP Client，接入WIFI时会自动分配IP地址】
	//wifi_station_dhcpc_stop();							// 关闭 ESP8266 Station DHCP client
	//IP4_ADDR(&ST_ESP8266_IP.ip, 192,168,0,10);			// 配置IP地址
	//IP4_ADDR(&ST_ESP8266_IP.netmask, 255,255,255,0);	// 配置子网掩码
	//IP4_ADDR(&ST_ESP8266_IP.gw, 192,168,0,1);			// 配置网关
	//wifi_set_ip_info(STATION_IF, &ST_ESP8266_IP);		// 设置 Wi-Fi Station 或者 SoftAP 的 IP 地址

	//结构体赋值(注意：【服务集标识符/密码】须设为字符串形式)
	os_memset(&STA_config, 0, sizeof(struct station_config));	// 将 STA_config 清空
	ets_strcpy(STA_config.ssid, ESP8266_STA_SSID);				// 设置WiFi名称(将字符串复制到ssid数组)
	ets_strcpy(STA_config.password, ESP6266_STA_PASS);			// 设置WiFi密码(将字符串复制到password数组)

	wifi_station_set_config(&STA_config);						// 设置STA参数 并保存到Flash

	// 此API不能在user_init初始化中调用
	// 如果user_init中调用wifi_station_set_config(...)的话，内核会自动将ESP8266接入WIFI
	// wifi_station_connect();		// ESP8266连接WIFI
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
	OLED_Init();								// |
	OLED_Init();								// OLED初始化
	OLED_ShowString(0,0,"                ");	// Internet Time
	OLED_ShowString(0,2,"Clock =         ");	// Clock：时钟
	OLED_ShowString(0,4,"Temp  =         ");	// Temperature：温度
	OLED_ShowString(0,6,"Humid =         ");	// Humidity：湿度

	/* STA模式 */
	ESP8266_STA_Init();

	/* 软件定时器 */
	OS_Timer_1_Init(1000, 1);				// 软件定时器初始化
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



