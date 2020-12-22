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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													//																				//
// 工程：	SmartConfig_WeChat						// ①：手机预先接入8266将要配置的WIFI，然后复位ESP8266							//
//													//																				//
// 平台：	物联网开发板 ESP8266 V1.0	// ②：OLED显示【WIFI Connecting................】								//
//													//																				//
// 功能：	①：8266设置为STA模式，接入WIFI热点		// ③：如果【WIFI名/密码】错误，OLED显示【Use WeChat to SmartConfig】			//
//													//																				//
//			②：如果WIFI连接出错，进入【微信配网】	// ④：打开【安信可科技】微信公众号，点击【WIFI配置】							//
//													//																				//
//			③：8266获取手机发送的【WIFI名/密码】	// ⑤：在【WIFI配置】页面，点击【开始配置】										//
//													//																				//
//			④：ESP8266连接WIFI(STA参数 -> Flash)	// ⑥：在【配置设备上网】页面，查看【WIFI名】是否正确，并配置【WIFI密码】		//
//													//																				//
//			⑤：连接成功后，LED快闪3次				// ⑦：点击【连接】，提示"连接中"，OLED显示【Use WeChat to SmartConfig】		//
//													//																				//
//	版本：	V1.0									// ⑧：WIFI连接成功后，提示"配置成功"，OLED显示【Connect to WIFI Successfully】	//
//													//																				//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



// 头文件引用
//==================================================================================
#include "user_config.h"		// 用户配置
#include "driver/uart.h"  		// 串口
#include "driver/oled.h"  		// OLED

//#include "at_custom.h"
#include "c_types.h"			// 变量类型
#include "eagle_soc.h"			// GPIO函数、宏定义
#include "ip_addr.h"			// 被"espconn.h"使用。在"espconn.h"开头#include"ip_addr.h"或#include"ip_addr.h"放在"espconn.h"之前
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
//#include "sntp.h"
//#include "spi_flash.h"
//#include "upgrade.h"
#include "user_interface.h" 	// 系统接口、system_param_xxx接口、WIFI、RateContro
//==================================================================================


// 宏定义
//==================================================================================
#define		ProjectName			"SmartConfig_WeChat"	// 工程名宏定义

#define		Sector_STA_INFO		0x80			// 【STA参数】保存扇区
//==================================================================================

// 全局变量
//==================================================================================
struct station_config STA_INFO;		// 【STA】参数结构体

os_timer_t OS_Timer_IP;				// 软件定时器

struct ip_info ST_ESP8266_IP;		// 8266的IP信息
u8 ESP8266_IP[4];					// 8266的IP地址

u8 C_LED_Flash = 0;					// LED闪烁计次
//==================================================================================


// 毫秒延时函数
//===========================================
void ICACHE_FLASH_ATTR delay_ms(u32 C_time)
{	for(;C_time>0;C_time--)
		os_delay_us(1000);
}
//===========================================


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

        // 正在获取【SSID】【PSWD】（8266正在抓取并解密【SSID+PSWD】）
        //…………………………………………………………………………………………………
        case SC_STATUS_GETTING_SSID_PSWD:
            os_printf("\r\nSC_STATUS_GETTING_SSID_PSWD\r\n");

            // 【SC_STATUS_GETTING_SSID_PSWD】状态下，参数2==SmartConfig类型指针
            //-------------------------------------------------------------------
			sc_type *type = pdata;		// 获取【SmartConfig类型】指针

			// 配网方式 == 【ESPTOUCH】
			//-------------------------------------------------
            if (*type == SC_TYPE_ESPTOUCH)
            { os_printf("\r\nSC_TYPE:SC_TYPE_ESPTOUCH\r\n"); }

            // 配网方式 == 【AIRKISS】||【ESPTOUCH_AIRKISS】
            //-------------------------------------------------
            else
            { os_printf("\r\nSC_TYPE:SC_TYPE_AIRKISS\r\n"); }

	    break;
	    //…………………………………………………………………………………………………

        // 成功获取到【SSID】【PSWD】，保存STA参数，并连接WIFI
	    //…………………………………………………………………………………………………
        case SC_STATUS_LINK:
            os_printf("\r\nSC_STATUS_LINK\r\n");

            // 【SC_STATUS_LINK】状态下，参数2 == STA参数结构体指针
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

            smartconfig_stop();		// 停止SmartConfig，释放内存

            //**************************************************************************************************
            wifi_get_ip_info(STATION_IF,&ST_ESP8266_IP);	// 获取8266_STA的IP地址

			ESP8266_IP[0] = ST_ESP8266_IP.ip.addr;		// IP地址高八位 == addr低八位
			ESP8266_IP[1] = ST_ESP8266_IP.ip.addr>>8;	// IP地址次高八位 == addr次低八位
			ESP8266_IP[2] = ST_ESP8266_IP.ip.addr>>16;	// IP地址次低八位 == addr次高八位
			ESP8266_IP[3] = ST_ESP8266_IP.ip.addr>>24;	// IP地址低八位 == addr高八位

			// 显示ESP8266的IP地址
			//-----------------------------------------------------------------------------------------------
			os_printf("ESP8266_IP = %d.%d.%d.%d\n",ESP8266_IP[0],ESP8266_IP[1],ESP8266_IP[2],ESP8266_IP[3]);
			OLED_ShowIP(24,2,ESP8266_IP);	// OLED显示ESP8266的IP地址
			OLED_ShowString(0,4,"Connect to WIFI ");
			OLED_ShowString(0,6,"Successfully    ");
			//-----------------------------------------------------------------------------------------------

			// 接入WIFI成功后，LED快闪3次
			//----------------------------------------------------
			for(; C_LED_Flash<=5; C_LED_Flash++)
			{
				GPIO_OUTPUT_SET(GPIO_ID_PIN(4),(C_LED_Flash%2));
				delay_ms(100);
			}

			os_printf("\r\n---- ESP8266 Connect to WIFI Successfully ----\r\n");

			//*****************************************************
			// WIFI连接成功，执行后续功能。	如：SNTP/UDP/TCP/DNS等
			//*****************************************************

			//**************************************************************************************************

	    break;
	    //…………………………………………………………………………………………………

    }
}
//=================================================================================================================


// IP定时检查的回调函数
//========================================================================================================
void ICACHE_FLASH_ATTR OS_Timer_IP_cb(void)
{
	u8 S_WIFI_STA_Connect;			// WIFI接入状态标志


	// 查询STA接入WIFI状态
	//-----------------------------------------------------
	S_WIFI_STA_Connect = wifi_station_get_connect_status();
	//---------------------------------------------------
	// Station连接状态表
	// 0 == STATION_IDLE -------------- STATION闲置
	// 1 == STATION_CONNECTING -------- 正在连接WIFI
	// 2 == STATION_WRONG_PASSWORD ---- WIFI密码错误
	// 3 == STATION_NO_AP_FOUND ------- 未发现指定WIFI
	// 4 == STATION_CONNECT_FAIL ------ 连接失败
	// 5 == STATION_GOT_IP ------------ 获得IP，连接成功
	//---------------------------------------------------


	// 成功接入WIFI
	//----------------------------------------------------------------------------------------
	if( S_WIFI_STA_Connect == STATION_GOT_IP )	// 判断是否获取IP
	{
		wifi_get_ip_info(STATION_IF,&ST_ESP8266_IP);	// 获取8266_STA的IP地址

		ESP8266_IP[0] = ST_ESP8266_IP.ip.addr;		// IP地址高八位 == addr低八位
		ESP8266_IP[1] = ST_ESP8266_IP.ip.addr>>8;	// IP地址次高八位 == addr次低八位
		ESP8266_IP[2] = ST_ESP8266_IP.ip.addr>>16;	// IP地址次低八位 == addr次高八位
		ESP8266_IP[3] = ST_ESP8266_IP.ip.addr>>24;	// IP地址低八位 == addr高八位

		// 显示ESP8266的IP地址
		//-----------------------------------------------------------------------------------------------
		os_printf("ESP8266_IP = %d.%d.%d.%d\n",ESP8266_IP[0],ESP8266_IP[1],ESP8266_IP[2],ESP8266_IP[3]);
		OLED_ShowIP(24,2,ESP8266_IP);	// OLED显示ESP8266的IP地址
		OLED_ShowString(0,4,"Connect to WIFI ");
		OLED_ShowString(0,6,"Successfully    ");
		//-----------------------------------------------------------------------------------------------

		// 接入WIFI成功后，LED快闪3次
		//----------------------------------------------------
		for(; C_LED_Flash<=5; C_LED_Flash++)
		{
			GPIO_OUTPUT_SET(GPIO_ID_PIN(4),(C_LED_Flash%2));
			delay_ms(100);
		}

		os_printf("\r\n---- ESP8266 Connect to WIFI Successfully ----\r\n");

		os_timer_disarm(&OS_Timer_IP);	// 关闭定时器

		//*****************************************************
		// WIFI连接成功，执行后续功能。	如：SNTP/UDP/TCP/DNS等
		//*****************************************************
	}

	// ESP8266无法连接WIFI
	//------------------------------------------------------------------------------------------------
	else if(	S_WIFI_STA_Connect==STATION_NO_AP_FOUND 	||		// 未找到指定WIFI
				S_WIFI_STA_Connect==STATION_WRONG_PASSWORD 	||		// WIFI密码错误
				S_WIFI_STA_Connect==STATION_CONNECT_FAIL		)	// 连接WIFI失败
	{
		os_timer_disarm(&OS_Timer_IP);			// 关闭定时器

		os_printf("\r\n---- S_WIFI_STA_Connect=%d-----------\r\n",S_WIFI_STA_Connect);
		os_printf("\r\n---- ESP8266 Can't Connect to WIFI-----------\r\n");

		// 微信智能配网设置
		//…………………………………………………………………………………………………………………………
		//wifi_set_opmode(STATION_MODE);			// 设为STA模式						//【第①步】

		smartconfig_set_type(SC_TYPE_AIRKISS); 	// ESP8266配网方式【AIRKISS】			//【第②步】

		smartconfig_start(smartconfig_done);	// 进入【智能配网模式】,并设置回调函数	//【第③步】
		//…………………………………………………………………………………………………………………………
	}
}
//========================================================================================================

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
	OLED_ShowString(0,2,"IP:");					// |
	OLED_ShowString(0,4,"WIFI Connecting ");	// |
	OLED_ShowString(0,6,"................");	// |
	//－－－－－－－－－－－－－－－－－－－－－－－

	LED_Init();		// LED初始化


// ESP8266读取【外部Flash】中的【STA参数】(SSID/PASS)，作为STA，连接WIFI
//………………………………………………………………………………………………………………………………
	os_memset(&STA_INFO,0,sizeof(struct station_config));			// STA_INFO = 0
	spi_flash_read(Sector_STA_INFO*4096,(uint32 *)&STA_INFO, 96);	// 读出【STA参数】(SSID/PASS)
	STA_INFO.ssid[31] = 0;		// SSID最后添'\0'
	STA_INFO.password[63] = 0;	// APSS最后添'\0'
	os_printf("\r\nSTA_INFO.ssid=%s\r\nSTA_INFO.password=%s\r\n",STA_INFO.ssid,STA_INFO.password);

	wifi_set_opmode(0x01);					// 设置为STA模式，并保存到Flash
	wifi_station_set_config(&STA_INFO);		// 设置STA参数
	wifi_station_connect();					// ESP8266连接WIFI（这里，此句可省）
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
