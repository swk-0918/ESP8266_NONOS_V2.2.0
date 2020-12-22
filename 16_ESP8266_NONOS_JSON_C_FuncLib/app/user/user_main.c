#include "user_config.h"		// 用户配置
#include "driver/uart.h"  		// 串口

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
//#include "sntp.h"
//#include "spi_flash.h"
//#include "upgrade.h"
#include "user_interface.h" 	// 系统接口、system_param_xxx接口、WIFI、RateContro

//==================================================================================


// 宏定义
//==================================================================================
#define		ProjectName			"JSON_C_FuncLib"			// 工程名宏定义

/* 【JSON_Tree】
//**************************************************************************
	{
    	"Shanghai":
    	{
        	"temp": "30℃",
        	"humid": "30%RH"
    	},

    	"Shenzhen":
    	{
        	"temp": "35℃",
        	"humid": 50
		},

    	"result": "Shenzhen is too hot!"
	}

//***************************************
*/

#define		JSON_Tree_Format	" { \n "								\
    								" \"Shanghai\": \n "				\
									" { \n "							\
        								" \"temp\": \"%s\", \n "		\
										" \"humid\": \"%s\" \n "		\
    								" }, \n "							\
																		\
									" \"Shenzhen\": \n "				\
									" { \n "							\
    									" \"temp\": \"%s\", \n "		\
										" \"humid\": %s \n "			\
    								" }, \n "							\
																		\
									" \"result\": \"%s\" \n "			\
								" } \n "

//**************************************************************************

//==================================================================================


// 全局变量
//==================================================================================
char A_JSON_Tree[256] = {0};	// 存放JSON树
//==================================================================================


// 毫秒延时函数
//===========================================
void ICACHE_FLASH_ATTR delay_ms(u32 C_time)
{	for(;C_time>0;C_time--)
		os_delay_us(1000);
}
//===========================================


// 创建JSON树
//===================================================================================================
void ICACHE_FLASH_ATTR Setup_JSON_Tree(void)
{

	// 赋值JSON树【赋值JSON_Tree_Format字符串中的格式字符】
	//--------------------------------------------------------------------------------------------
	os_sprintf(A_JSON_Tree, JSON_Tree_Format, "30℃","30%RH","35℃","50","Shenzhen is too hot!");

	os_printf("\r\n-------------------- 创建JSON树 -------------------\r\n");

	os_printf("%s",A_JSON_Tree);	// 串口打印JSON树

	os_printf("\r\n-------------------- 创建JSON树 -------------------\r\n");
}
//===================================================================================================


// 解析JSON树
//===================================================================================================
int ICACHE_FLASH_ATTR Parse_JSON_Tree(void)
{
	os_printf("\r\n-------------------- 解析JSON树 -------------------\r\n");

	char A_JSONTree_Value[64] = {0};	// JSON数据缓存数组

	char * T_Pointer_Head = NULL;		// 临时指针
	char * T_Pointer_end = NULL;		// 临时指针

	u8 T_Value_Len = 0;					// 【"值"】的长度


	// 【"Shanghai"】
	//………………………………………………………………………………………………………………
	T_Pointer_Head = os_strstr(A_JSON_Tree, "\"Shanghai\"");		// 【"Shanghai"】
	os_printf("Shanghai:\n");

//	T_Pointer_Head = os_strstr(T_Pointer_Head, ":");				// 【:】
//	T_Pointer_Head = os_strstr(T_Pointer_Head, "{");				// 【{】
	T_Pointer_Head = os_strstr(T_Pointer_Head, "\"temp\"");			// 【"temp"】
	T_Pointer_Head = os_strstr(T_Pointer_Head, ":");				// 【:】
	T_Pointer_Head = os_strstr(T_Pointer_Head, "\"") + 1;			// 【值的首指针】
	T_Pointer_end = os_strstr(T_Pointer_Head, "\"");				// 【"值"尾的"】
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// 计算【值】的长度
	os_memcpy(A_JSONTree_Value, T_Pointer_Head, T_Value_Len);		// 获取【值】
	A_JSONTree_Value[T_Value_Len] = '\0';							// 【值后添'\0'】
	os_printf("\t temp:%s\n",A_JSONTree_Value);


//	T_Pointer_Head = os_strstr(T_Pointer_Head, ",");				// 【,】
//	T_Pointer_Head = os_strstr(T_Pointer_Head, "\"");				// 【\"】
	T_Pointer_Head = os_strstr(T_Pointer_Head, "\"humid\"");		// 【"humid"】
	T_Pointer_Head = os_strstr(T_Pointer_Head, ":");				// 【:】
	T_Pointer_Head = os_strstr(T_Pointer_Head, "\"") + 1;			// 【值的首指针】
	T_Pointer_end = os_strstr(T_Pointer_Head, "\"");				// 【"值"尾的"】
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// 计算【值】的长度
	os_memcpy(A_JSONTree_Value, T_Pointer_Head, T_Value_Len);		// 获取【值】
	A_JSONTree_Value[T_Value_Len] = '\0';							// 【值后添'\0'】
	os_printf("\t humid:%s\n",A_JSONTree_Value);

	//	T_Pointer_Head = os_strstr(T_Pointer_Head, "}");			// 【}】
	//	T_Pointer_Head = os_strstr(T_Pointer_Head, ",");			// 【,】
	//………………………………………………………………………………………………………………


	// 【"Shenzhen"】
	//………………………………………………………………………………………………………………
	T_Pointer_Head = os_strstr(T_Pointer_Head, "\"Shenzhen\"");		// 【"Shenzhen"】
	os_printf("Shenzhen:\n");

//	T_Pointer_Head = os_strstr(T_Pointer_Head, ":");				// 【:】
//	T_Pointer_Head = os_strstr(T_Pointer_Head, "{");				// 【{】
	T_Pointer_Head = os_strstr(T_Pointer_Head, "\"temp\"");			// 【"temp"】
	T_Pointer_Head = os_strstr(T_Pointer_Head, ":");				// 【:】
	T_Pointer_Head = os_strstr(T_Pointer_Head, "\"") + 1;			// 【值的首指针】
	T_Pointer_end = os_strstr(T_Pointer_Head, "\"");				// 【"值"尾的"】
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// 计算【值】的长度
	os_memcpy(A_JSONTree_Value, T_Pointer_Head, T_Value_Len);		// 获取【值】
	A_JSONTree_Value[T_Value_Len] = '\0';							// 【值后添'\0'】
	os_printf("\t temp:%s\n",A_JSONTree_Value);


	//【注："humid"键所对应的值是数字。数字同样是由ASSIC码表示，但是没有""】
	//……………………………………………………………………………………………………
//	T_Pointer_Head = os_strstr(T_Pointer_Head, ",");				// 【,】
//	T_Pointer_Head = os_strstr(T_Pointer_Head, "\"");				// 【\"】
	T_Pointer_Head = os_strstr(T_Pointer_Head, "\"humid\"");		// 【"humid"】
	T_Pointer_Head = os_strstr(T_Pointer_Head, ":");				// 【:】

	// 获取数字的首指针【数字为十进制形式，并且没有""】
	//-----------------------------------------------------
	while(*T_Pointer_Head < '0' || *T_Pointer_Head > '9')	// 排除不在【0～9】范围内的字符
		T_Pointer_Head ++ ;

	T_Pointer_end = T_Pointer_Head;	// 设置数字尾指针初值

	// 获取数字的尾指针+1【数字为十进制形式，并且没有""】
	//-----------------------------------------------------
	while(*T_Pointer_end >= '0' && *T_Pointer_end <= '9')	// 计算在【0～9】范围内的字符
		T_Pointer_end ++ ;

	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// 计算【值(数字)】的长度
	os_memcpy(A_JSONTree_Value, T_Pointer_Head, T_Value_Len);		// 获取【值(数字)】
	A_JSONTree_Value[T_Value_Len] = '\0';							// 【值后添'\0'】
	os_printf("\t humid:%s\n",A_JSONTree_Value);

//	T_Pointer_Head = os_strstr(T_Pointer_Head, "}");				// 【}】
//	T_Pointer_Head = os_strstr(T_Pointer_Head, ",");				// 【,】
	//………………………………………………………………………………………………………………

	// 【"result"】
	//………………………………………………………………………………………………………………
	T_Pointer_Head = os_strstr(T_Pointer_Head, "\"result\"");		// 【"result"】
	T_Pointer_Head = os_strstr(T_Pointer_Head, ":");				// 【:】
	T_Pointer_Head = os_strstr(T_Pointer_Head, "\"") + 1;			// 【值的首指针】
	T_Pointer_end = os_strstr(T_Pointer_Head, "\"");				// 【"值"尾的"】
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// 计算【值】的长度
	os_memcpy(A_JSONTree_Value, T_Pointer_Head, T_Value_Len);		// 获取【值】
	A_JSONTree_Value[T_Value_Len] = '\0';							// 【值后添'\0'】
	os_printf("result:%s\n",A_JSONTree_Value);

	T_Pointer_Head = os_strstr(T_Pointer_Head, "}");			// 【}】
	//………………………………………………………………………………………………………………

    os_printf("\r\n-------------------- 解析JSON树 -------------------\r\n");

    return 0 ;
}
//===================================================================================================



// user_init：entry of user application, init user function here
//==============================================================================
void ICACHE_FLASH_ATTR user_init(void)
{
	uart_init(74800,74800);	// 初始化串口波特率
	os_delay_us(10000);			// 等待串口稳定
	os_printf("\r\n=================================================\r\n");
	os_printf("\t Project:\t%s\r\n", ProjectName);
	os_printf("\t SDK version:\t%s", system_get_sdk_version());
	os_printf("\r\n=================================================\r\n");

	os_printf("JSON_Tree_Format:\n%s", JSON_Tree_Format);	// 打印JSON树格式

	Setup_JSON_Tree();		// 创建JSON树

	Parse_JSON_Tree();		// 解析JSON树
}
//==============================================================================


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
