/* * ESPRESSIF MIT License
 *
 * Copyright (c) 2016 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//															//														//
// 工程：	SNTP_SmartConfig								//		①：复位8266，8266开始连接WIFI。				//
//															//														//
// 平台：	物联网开发板 ESP8266 V1.0			//		②：如果WIFI连接不上，则进行微信配网			//
//															//														//
// 功能：	①：8266设置为STA模式，接入WIFI热点				//		③：OLED显示【日期】【时钟】【温度】【湿度】	//
//															//														//
//			②：如果WIFI连接出错，进入【微信配网】			//		④：OLED_1行：【年、月、日、星期X】				//
//															//														//
//			②：设置SNTP服务器，初始化SNTP					//		⑤：OLED_2行：【Clock = 时、分、秒】			//
//															//														//
//			③：每秒查询Internet时间						//		⑥：OLED_3行：【Hunid = 湿度 %RH】				//
//															//														//
//			④：每5秒读取DHT11检测的温湿度					//		⑦：OLED_4行：【Temp  = 温度 ℃】				//
//															//														//
//			⑤：OLED显示【Internet时间】、【环境温湿度】	//		⑧：如果DHT11读取失败，温湿度显示【----】		//
//															//														//
//	版本：	V1.0											//		⑨：8266和OLED通信时，蓝色OLED闪烁				//
//															//														//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// 头文件引用
//==================================================================================
#include "user_config.h"		// 用户配置
#include "driver/uart.h"  		// 串口
#include "driver/oled.h"  		// OLED
#include "driver/dht11.h"		// DHT11

//#include "at_custom.h"
#include "c_types.h"			// 变量类型
#include "eagle_soc.h"			// GPIO函数、宏定义
#include "ip_addr.h"			// 被"espconn.h"使用。
#include "espconn.h"			// TCP/UDP接口
//#include "espnow.h"
#include "ets_sys.h"			// 回调函数
//#include "gpio.h"
#include "mem.h"				// 内存申请等函数
#include "os_type.h"			// os_XXX
#include "osapi.h"  			// os_XXX、软件定时器
//#include "ping.h"
//#include "pwm.h"
//#include "queue.h"
#include "smartconfig.h"		// 智能配网
#include "sntp.h"				// SNTP
//#include "spi_flash.h"
//#include "upgrade.h"
#include "user_interface.h" 	// 系统接口、system_param_xxx接口、WIFI、RateContro
//==================================================================================


// 宏定义
//==================================================================================
#define		ProjectName			"SNTP_SmartConfig"		// 工程名宏定义

#define		Sector_STA_INFO		0x90			// 【STA参数】保存扇区
//==================================================================================

// 全局变量
//==================================================================================
struct station_config STA_INFO;		// 【STA】参数结构体

u8 C_LED_Flash = 0;					// LED闪烁计次

u8 C_Read_DHT11 = 0;				// 读取DHT11计时

os_timer_t OS_Timer_IP;				// 定时器_查询IP地址

os_timer_t OS_Timer_SNTP;			// 定时器_SNTP
//==================================================================================


// 毫秒延时函数
//===========================================
void ICACHE_FLASH_ATTR delay_ms(u32 C_time)
{	for(;C_time>0;C_time--)
		os_delay_us(1000);
}
//===========================================



//	 -----------------------------------------------			 -----------------------------------------------
//	|	  月份		|	英文简写	|	英文全称	|			|	  星期X		|	英文简写	|	英文全称	|
//	 -----------------------------------------------			 -----------------------------------------------
//	| 	  一月		|	  Jan		|	January		|			|	  周一		|	  Mon		|	Monday		|
//	 -----------------------------------------------			 -----------------------------------------------
//	|	  二月		|	  Feb		|	February	|			|	  周二		|	  Tue		|	Tuesday		|
//	 -----------------------------------------------			 -----------------------------------------------
//	| 	  三月		|	  Mar		|	March		|			|	  周三		|	  Wed		|	Wednesday	|
//	 -----------------------------------------------			 -----------------------------------------------
//	| 	  四月		|	  Apr		|	April		|			|	  周四		|	  Thu		|	Thursday	|
//	 -----------------------------------------------			 -----------------------------------------------
//	|  	  五月		|	  May		|	May			|			|	  周五		|	  Fri		|	Friday		|
//	 -----------------------------------------------			 -----------------------------------------------
//	|  	  六月		|	  June		|	June		|			|	  周六		|	  Sat		|	Saturday	|
//	 -----------------------------------------------			 -----------------------------------------------
//	|  	  七月		|	  July		|	July		|			|	  周日		|	  Sun		|	Sunday		|
//	 -----------------------------------------------			 -----------------------------------------------
//	|  	  八月		|	  Aug		|	Aguest		|
//	 -----------------------------------------------
//	|  	  九月		|	  Sept		|	September	|
//	 -----------------------------------------------
//	|  	  十月		|	  Oct		|	October		|
//	 -----------------------------------------------
//	|  	 十一月		|	  Nov		|	November	|
//	 -----------------------------------------------
//	|  	 十二月		|	  Dec		|	December	|
//	 -----------------------------------------------


// SNTP定时回调函数
//===================================================================================================
void ICACHE_FLASH_ATTR OS_Timer_SNTP_cb(void	 * arg)
{
	// 字符串整理 相关变量
	//------------------------------------------------------

	u8 C_Str = 0;				// 字符串字节计数

	char A_Str_Data[20] = {0};	// 【"日期"】字符串数组

	char *T_A_Str_Data = A_Str_Data;	// 缓存数组指针

	char A_Str_Clock[10] = {0};	// 【"时间"】字符串数组


	char * Str_Head_Week;		// 【"星期"】字符串首地址

	char * Str_Head_Month;		// 【"月份"】字符串首地址

	char * Str_Head_Day;		// 【"日数"】字符串首地址

	char * Str_Head_Clock;		// 【"时钟"】字符串首地址

	char * Str_Head_Year;		// 【"年份"】字符串首地址

	//------------------------------------------------------


	 uint32	TimeStamp;		// 时间戳

	 char * Str_RealTime;	// 实际时间的字符串


	 // 查询当前距离基准时间(1970.01.01 00:00:00 GMT+8)的时间戳(单位:秒)
	 //-----------------------------------------------------------------
	 TimeStamp = sntp_get_current_timestamp();

	 if(TimeStamp)		// 判断是否获取到偏移时间
	 {
		 //os_timer_disarm(&OS_Timer_SNTP);	// 关闭SNTP定时器

		 // 查询实际时间(GMT+8):东八区(北京时间)
		 //--------------------------------------------
		 Str_RealTime = sntp_get_real_time(TimeStamp);


		 // 【实际时间】字符串 == "周 月 日 时:分:秒 年"
		 //------------------------------------------------------------------------
		 os_printf("\r\n----------------------------------------------------\r\n");
		 os_printf("SNTP_TimeStamp = %d\r\n",TimeStamp);		// 时间戳
		 os_printf("\r\nSNTP_InternetTime = %s",Str_RealTime);	// 实际时间
		 os_printf("--------------------------------------------------------\r\n");


		 // 时间字符串整理，OLED显示【"日期"】、【"时间"】字符串
		 //…………………………………………………………………………………………………………………

		 // 【"年份" + ' '】填入日期数组
		 //---------------------------------------------------------------------------------
		 Str_Head_Year = Str_RealTime;	// 设置起始地址

		 while( *Str_Head_Year )		// 找到【"实际时间"】字符串的结束字符'\0'
			 Str_Head_Year ++ ;

		 // 【注：API返回的实际时间字符串，最后还有一个换行符，所以这里 -5】
		 //-----------------------------------------------------------------
		 Str_Head_Year -= 5 ;			// 获取【"年份"】字符串的首地址

		 T_A_Str_Data[4] = ' ' ;
		 os_memcpy(T_A_Str_Data, Str_Head_Year, 4);		// 【"年份" + ' '】填入日期数组

		 T_A_Str_Data += 5;				// 指向【"年份" + ' '】字符串的后面的地址
		 //---------------------------------------------------------------------------------

		 // 获取【日期】字符串的首地址
		 //---------------------------------------------------------------------------------
		 Str_Head_Week 	= Str_RealTime;							// "星期" 字符串的首地址
		 Str_Head_Month = os_strstr(Str_Head_Week,	" ") + 1;	// "月份" 字符串的首地址
		 Str_Head_Day 	= os_strstr(Str_Head_Month,	" ") + 1;	// "日数" 字符串的首地址
		 Str_Head_Clock = os_strstr(Str_Head_Day,	" ") + 1;	// "时钟" 字符串的首地址


		 // 【"月份" + ' '】填入日期数组
		 //---------------------------------------------------------------------------------
		 C_Str = Str_Head_Day - Str_Head_Month;				// 【"月份" + ' '】的字节数

		 os_memcpy(T_A_Str_Data, Str_Head_Month, C_Str);	// 【"月份" + ' '】填入日期数组

		 T_A_Str_Data += C_Str;		// 指向【"月份" + ' '】字符串的后面的地址


		 // 【"日数" + ' '】填入日期数组
		 //---------------------------------------------------------------------------------
		 C_Str = Str_Head_Clock - Str_Head_Day;				// 【"日数" + ' '】的字节数

		 os_memcpy(T_A_Str_Data, Str_Head_Day, C_Str);		// 【"日数" + ' '】填入日期数组

		 T_A_Str_Data += C_Str;		// 指向【"日数" + ' '】字符串的后面的地址


		 // 【"星期" + ' '】填入日期数组
		 //---------------------------------------------------------------------------------
		 C_Str = Str_Head_Month - Str_Head_Week - 1;		// 【"星期"】的字节数

		 os_memcpy(T_A_Str_Data, Str_Head_Week, C_Str);		// 【"星期"】填入日期数组

		 T_A_Str_Data += C_Str;		// 指向【"星期"】字符串的后面的地址


		 // OLED显示【"日期"】、【"时钟"】字符串
		 //---------------------------------------------------------------------------------
		 *T_A_Str_Data = '\0';		// 【"日期"】字符串后面添加'\0'

		 OLED_ShowString(0,0,A_Str_Data);		// OLED显示日期


		 os_memcpy(A_Str_Clock, Str_Head_Clock, 8);		// 【"时钟"】字符串填入时钟数组
		 A_Str_Clock[8] = '\0';

		 OLED_ShowString(64,2,A_Str_Clock);		// OLED显示时间

		 //…………………………………………………………………………………………………………………
	 }


	// 每5秒，读取/显示温湿度数据
	//-----------------------------------------------------------------------------------------
	C_Read_DHT11 ++ ;		// 读取DHT11计时

	if(C_Read_DHT11>=5)		// 5秒计时
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
	//-----------------------------------------------------------------------------------------
}
//===================================================================================================


// SNTP定时初始化
//=============================================================================
void ICACHE_FLASH_ATTR OS_Timer_SNTP_Init(u32 time_ms, u8 time_repetitive)
{
	os_timer_disarm(&OS_Timer_SNTP);
	os_timer_setfn(&OS_Timer_SNTP,(os_timer_func_t *)OS_Timer_SNTP_cb,NULL);
	os_timer_arm(&OS_Timer_SNTP, time_ms, time_repetitive);
}
//=============================================================================


// 初始化SNTP
//=============================================================================
void ICACHE_FLASH_ATTR ESP8266_SNTP_Init(void)
{
	ip_addr_t * addr = (ip_addr_t *)os_zalloc(sizeof(ip_addr_t));

	sntp_setservername(0, "us.pool.ntp.org");	// 服务器_0【域名】
	sntp_setservername(1, "ntp.sjtu.edu.cn");	// 服务器_1【域名】

	ipaddr_aton("210.72.145.44", addr);			// 点分十进制 => 32位二进制
	sntp_setserver(2, addr);					// 服务器_2【IP地址】
	os_free(addr);								// 释放addr

	sntp_init();	// SNTP初始化API

	OS_Timer_SNTP_Init(1000,1);				// 1秒重复定时(SNTP)
}
//=============================================================================


// SmartConfig状态发生改变时，进入此回调函数
//--------------------------------------------
// 参数1：sc_status status / 参数2：无类型指针【在不同状态下，[void *pdata]的传入参数是不同的】
//=================================================================================================================
void ICACHE_FLASH_ATTR smartconfig_done(sc_status status, void *pdata)
{
	os_printf("\r\n------ smartconfig_done ------\r\n");	// ESP8266网络状态改变

    switch(status)
    {
    	// CmartConfig等待
		//……………………………………………………
		case SC_STATUS_WAIT:		// 初始值
			os_printf("\r\nSC_STATUS_WAIT\r\n");
		break;
		//……………………………………………………

		// 发现【WIFI信号】（8266在这种状态下等待配网）
		//…………………………………………………………………………………………………
		case SC_STATUS_FIND_CHANNEL:
			os_printf("\r\nSC_STATUS_FIND_CHANNEL\r\n");

			os_printf("\r\n---- Please Use WeChat to SmartConfig ------\r\n\r\n");

			OLED_ShowString(0,4,"Use WeChat to   ");
			OLED_ShowString(0,6,"SmartConfig     ");
		break;
		//…………………………………………………………………………………………………

        // 正在获取【SSID】【PSWD】
        //…………………………………………………………………………………………………
        case SC_STATUS_GETTING_SSID_PSWD:
            os_printf("\r\nSC_STATUS_GETTING_SSID_PSWD\r\n");

            // 【SC_STATUS_GETTING_SSID_PSWD】状态下，参数2==SmartConfig类型指针
            //-------------------------------------------------------------------
			sc_type *type = pdata;		// 获取【SmartConfig类型】指针

			// 配网方式 == 【ESPTOUCH】
			//-------------------------------------------------
            if (*type == SC_TYPE_ESPTOUCH)
            {
            	os_printf("\r\nSC_TYPE:SC_TYPE_ESPTOUCH\r\n");
            }

            // 配网方式 == 【AIRKISS】||【ESPTOUCH_AIRKISS】
            //-------------------------------------------------
            else
            {
            	os_printf("\r\nSC_TYPE:SC_TYPE_AIRKISS\r\n");
            }

	    break;
	    //…………………………………………………………………………………………………

	    // 成功获取到【SSID】【PSWD】，保存STA参数，并连接WIFI
	    //…………………………………………………………………………………………………
        case SC_STATUS_LINK:
            os_printf("\r\nSC_STATUS_LINK\r\n");

            // 【SC_STATUS_LINK】状态下，参数2 == STA参数指针
            //------------------------------------------------------------------
            struct station_config *sta_conf = pdata;	// 获取【STA参数】指针

            // 将【SSID】【PASS】保存到【外部Flash】中
            //--------------------------------------------------------------------------
			spi_flash_erase_sector(Sector_STA_INFO);						// 擦除扇区
			spi_flash_write(Sector_STA_INFO*4096, (uint32 *)sta_conf, 96);	// 写入扇区
			//--------------------------------------------------------------------------

	        wifi_station_set_config(sta_conf);			// 设置STA参数【Flash】
	        wifi_station_disconnect();					// 断开STA连接
	        wifi_station_connect();						// ESP8266连接WIFI

	    	OLED_ShowString(0,4,"WIFI Connecting ");	// OLED显示：
	    	OLED_ShowString(0,6,"................");	// 正在连接WIFI

	    break;
	    //…………………………………………………………………………………………………


        // ESP8266作为STA，成功连接到WIFI
	    //…………………………………………………………………………………………………
        case SC_STATUS_LINK_OVER:
            os_printf("\r\nSC_STATUS_LINK_OVER\r\n");

            smartconfig_stop();		// 停止SmartConfig

            //**************************************************************************************************
//			wifi_get_ip_info(STATION_IF,&ST_ESP8266_IP);	// 获取8266_STA的IP地址

//			ESP8266_IP[0] = ST_ESP8266_IP.ip.addr;		// IP地址高八位 == addr低八位
//			ESP8266_IP[1] = ST_ESP8266_IP.ip.addr>>8;	// IP地址次高八位 == addr次低八位
//			ESP8266_IP[2] = ST_ESP8266_IP.ip.addr>>16;	// IP地址次低八位 == addr次高八位
//			ESP8266_IP[3] = ST_ESP8266_IP.ip.addr>>24;	// IP地址低八位 == addr高八位

			// 显示ESP8266的IP地址
			//-----------------------------------------------------------------------------------------------
//			os_printf("ESP8266_IP = %d.%d.%d.%d\n",ESP8266_IP[0],ESP8266_IP[1],ESP8266_IP[2],ESP8266_IP[3]);
//			OLED_ShowIP(24,2,ESP8266_IP);	// OLED显示ESP8266的IP地址
//			OLED_ShowString(0,4,"Connect to WIFI ");
//			OLED_ShowString(0,6,"Successfully    ");
			//-----------------------------------------------------------------------------------------------

    		//----------------------------------------------------------------
    		OLED_ShowString(0,0,"                ");	// Internet Time
    		OLED_ShowString(0,2,"Clock =         ");	// Clock：时钟
    		OLED_ShowString(0,4,"Temp  =         ");	// Temperature：温度
    		OLED_ShowString(0,6,"Humid =         ");	// Humidity：湿度
    		//----------------------------------------------------------------

			// 接入WIFI成功后，LED快闪3次
			//----------------------------------------------------
			for(; C_LED_Flash<=5; C_LED_Flash++)
			{
				GPIO_OUTPUT_SET(GPIO_ID_PIN(4),(C_LED_Flash%2));
				delay_ms(100);
			}

			ESP8266_SNTP_Init();		// 初始化SNTP

			os_printf("\r\n---- ESP8266 Connect to WIFI Successfully ----\r\n");
			//**************************************************************************************************

	    break;
	    //…………………………………………………………………………………………………

    }
}
//=================================================================================================================



// IP定时检查的回调函数
//=========================================================================================================
void ICACHE_FLASH_ATTR OS_Timer_IP_cb(void)
{
	struct ip_info ST_ESP8266_IP;	// ESP8266的IP信息
	u8 ESP8266_IP[4];				// ESP8266的IP地址

	u8 S_WIFI_STA_Connect = wifi_station_get_connect_status();	// 查询STA接入WIFI状态


	// 成功接入WIFI【STA模式下，如果开启DHCP(默认)，则ESO8266的IP地址由WIFI路由器自动分配】
	//-------------------------------------------------------------------------------------
	if( S_WIFI_STA_Connect == STATION_GOT_IP )	// 判断是否获取IP
	{
		wifi_get_ip_info(STATION_IF,&ST_ESP8266_IP);	// 获取STA的IP信息
		ESP8266_IP[0] = ST_ESP8266_IP.ip.addr;			// IP地址高八位 == addr低八位
		ESP8266_IP[1] = ST_ESP8266_IP.ip.addr>>8;		// IP地址次高八位 == addr次低八位
		ESP8266_IP[2] = ST_ESP8266_IP.ip.addr>>16;		// IP地址次低八位 == addr次高八位
		ESP8266_IP[3] = ST_ESP8266_IP.ip.addr>>24;		// IP地址低八位 == addr高八位


		//----------------------------------------------------------------
		OLED_ShowString(0,0,"                ");	// Internet Time
		OLED_ShowString(0,2,"Clock =         ");	// Clock：时钟
		OLED_ShowString(0,4,"Temp  =         ");	// Temperature：温度
		OLED_ShowString(0,6,"Humid =         ");	// Humidity：湿度
		//----------------------------------------------------------------

		// 接入WIFI成功后，LED快闪3次
		//----------------------------------------------------
		for(; C_LED_Flash<=5; C_LED_Flash++)
		{
			GPIO_OUTPUT_SET(GPIO_ID_PIN(4),(C_LED_Flash%2));
			delay_ms(100);
		}

		os_timer_disarm(&OS_Timer_IP);	// 关闭定时器

		ESP8266_SNTP_Init();			// 初始化SNTP
	}


	// ESP8266无法连接WIFI
	//------------------------------------------------------------------------------------------------
	else if(	S_WIFI_STA_Connect==STATION_NO_AP_FOUND 	||		// 未找到指定WIFI
				S_WIFI_STA_Connect==STATION_WRONG_PASSWORD 	||		// WIFI密码错误
				S_WIFI_STA_Connect==STATION_CONNECT_FAIL		)	// 连接WIFI失败
	{
		os_timer_disarm(&OS_Timer_IP);			// 关闭定时器

		os_printf("\r\n---- ESP8266 Can't Connect to WIFI-----------\r\n");


		// 微信智能配网设置
		//…………………………………………………………………………………………………………………………
		//wifi_set_opmode(STATION_MODE);		// 设为STA模式							//【第①步】

		smartconfig_set_type(SC_TYPE_AIRKISS); 	// ESP8266配网方式【AIRKISS】			//【第②步】

		smartconfig_start(smartconfig_done);	// 进入【智能配网模式】,并设置回调函数	//【第③步】
		//…………………………………………………………………………………………………………………………
	}
}
//=========================================================================================================

// 软件定时器初始化(ms毫秒)
//========================================================================================
void ICACHE_FLASH_ATTR OS_Timer_IP_Init(u32 time_ms, u8 time_repetitive)
{
	os_timer_disarm(&OS_Timer_IP);	// 关闭定时器
	os_timer_setfn(&OS_Timer_IP,(os_timer_func_t *)OS_Timer_IP_cb, NULL);	// 设置定时器
	os_timer_arm(&OS_Timer_IP, time_ms, time_repetitive);  // 使能定时器
}
//========================================================================================

// LED初始化
//=============================================================================
void ICACHE_FLASH_ATTR LED_Init(void)
{
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U,	FUNC_GPIO4);	// GPIO4设为IO口

	GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1);						// IO4 = 1
}
//=============================================================================


// user_init：entry of user application, init user function here
//=================================================================================================
void ICACHE_FLASH_ATTR user_init(void)
{
	uart_init(74800,74800);	// 初始化串口波特率
	os_delay_us(10000);			// 等待串口稳定
	os_printf("\r\n=================================================\r\n");
	os_printf("\t Project:\t%s\r\n", ProjectName);
	os_printf("\t SDK version:\t%s", system_get_sdk_version());
	os_printf("\r\n=================================================\r\n");


	// OLED初始化
	//－－－－－－－－－－－－－－－－－－－－－－－
	OLED_Init();								// |
	OLED_ShowString(0,0,"ESP8266 = STA");		// |
	OLED_ShowString(0,4,"WIFI Connecting ");	// |
	OLED_ShowString(0,6,"................");	// |
	//－－－－－－－－－－－－－－－－－－－－－－－

	LED_Init();		// LED初始化


// ESP8266读取【外部Flash】中的【STA参数】(SSID/PASS)，作为STA，连接WIFI
	os_memset(&STA_INFO, 0, sizeof(struct station_config));			// STA_INFO = 0
	spi_flash_read(Sector_STA_INFO*4096, (uint32 *)&STA_INFO, 96);	// 读出【STA参数】(SSID/PASS)
	STA_INFO.ssid[31] = 0;		// SSID最后添'\0'
	STA_INFO.password[63] = 0;	// APSS最后添'\0'
	os_printf("\r\nSTA_INFO.ssid=%s\r\nSTA_INFO.password=%s\r\n",STA_INFO.ssid,STA_INFO.password);


	wifi_set_opmode(0x01);					// 设置为STA模式，并保存到Flash
	wifi_station_set_config(&STA_INFO);		// 设置STA参数
//	wifi_station_connect();					// ESP8266连接WIFI（这里，此句可省）
//………………………………………………………………………………………………………………………………

	OS_Timer_IP_Init(1000,1);	// 定时查询8266连接WIFI情况
}
//=================================================================================================


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
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
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

void ICACHE_FLASH_ATTR user_rf_pre_init(void){}
