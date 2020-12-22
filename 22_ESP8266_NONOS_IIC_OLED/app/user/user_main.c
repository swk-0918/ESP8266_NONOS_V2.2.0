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
// 工程：	IIC_OLED								//	注意：																		//
//													//																				//
// 平台：	物联网开发板 ESP8266 V1.0	//	0.96'OLED分辨率 == 128*64		即：每一行有128个点，每一列有64个点			//
//													//																				//
// 功能：	①：初始化OLED							//	字符的大小为【8*16】			即：一个字符占【横向8个点、纵向16个点】		//
//													//																				//
//			②：显示【Project=IIC_OLED】			//	只支持ASCII码表中，从【' '】~【'~'】的字符			注：可自行修改			//
//													//																				//
//			③：显示【IP:192.168.4.1】				//	【OLED_ShowString(x, y, "...")】显示字符/字符串		注：【x、y从0开始】		//
//													//																				//
//			④：显示【0123456789ABCDEFGHIJKLMN】	//	【OLED_ShowIP(x, y, A)】显示“点分十进制IP地址”	注：【x、y从0开始】		//
//													//																				//
//	版本：	V1.0									//	开发板OLED【SDA=IO2】，ESP-12F模组LED连IO2。8266与OLED进行IIC通信时LED会亮	//
//													//																				//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// 头文件引用
//==================================================================================
#include "user_config.h"		// 用户配置
#include "driver/uart.h"  		// 串口
#include "driver/oled.h"  		// OLED头文件

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
//#include "smartconfig.h"
#include "sntp.h"				// SNTP
//#include "spi_flash.h"
//#include "upgrade.h"
#include "user_interface.h" 	// 系统接口、system_param_xxx接口、WIFI、RateContro
//==================================================================================


// 宏定义
//==================================================================================
#define		ProjectName			"IIC_OLED"				// 工程名宏定义
//==================================================================================

// 全局变量
//==================================================================================

//==================================================================================


// 毫秒延时函数
//===========================================
void ICACHE_FLASH_ATTR delay_ms(u32 C_time)
{	for(;C_time>0;C_time--)
		os_delay_us(1000);
}
//===========================================


// user_init：entry of user application, init user function here
//=====================================================================================================================================
void ICACHE_FLASH_ATTR user_init(void)
{
	uart_init(74800,74800);	// 初始化串口波特率
	os_delay_us(10000);			// 等待串口稳定
	os_printf("\r\n=================================================\r\n");
	os_printf("\t Project:\t%s\r\n", ProjectName);
	os_printf("\t SDK version:\t%s", system_get_sdk_version());
	os_printf("\r\n=================================================\r\n");



//--------------------------------
	OLED_Init();	// OLED初始化



// OLED显示字符串
//-------------------------------------------------------------------------------------------------------------------------
	OLED_ShowString(0,0,"Project=");		// 从(0,0)开始显示

	OLED_ShowString(64,0,"IIC_OLED");		// 从(64,0)开始显示		// 因为，【Project=】一共8个字符，横向上占用【64】个点
//-------------------------------------------------------------------------------------------------------------------------



// OLED显示【点分十进制_IP地址】
//-------------------------------------------------------------------------------------------------------------------------
	u8 IP_Address[4];
	   IP_Address[0] = 192;
	   IP_Address[1] = 168;
	   IP_Address[2] = 4;
	   IP_Address[3] = 1;
//	u8 IP_Address[4] = { 192,168,4,1 };		// 数组形式表示【点分十进制_IP地址】


	OLED_ShowString(0,2,"IP:");				// 从(0,2)开始显示		// 因为【Project=IIC_OLED】在纵向上占用了【2】页(2*8个点)

	OLED_ShowIP(24, 2, IP_Address);			// 从(24,2)开始显示		// 因为，【IP:】一共3个字符，横向上占用【24】个点
//-------------------------------------------------------------------------------------------------------------------------


	OLED_ShowString(0,4,"0123456789ABCDEFGHIJKLMN");	// 从(0,4)开始显示
														// 因为【Project=IIC_OLED】【IP:192.168.4.1】在纵向上占用了【4】页(4*8个点)
}
//=====================================================================================================================================


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
