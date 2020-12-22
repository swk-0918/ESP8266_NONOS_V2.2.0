#include "user_config.h"		// 用户配置
#include "driver/uart.h"  		// 串口
#include "driver/oled.h"  		// OLED

#include "c_types.h"			// 变量类型
#include "eagle_soc.h"			// GPIO函数、宏定义
#include "ip_addr.h"			// 被"espconn.h"使用。在"espconn.h"开头#include"ip_addr.h"或#include"ip_addr.h"放在"espconn.h"之前
#include "espconn.h"
#include "ets_sys.h"			// 回调函数
#include "mem.h"				// 内存申请函数
#include "os_type.h"			// os_XXX
#include "osapi.h"  			// os_XXX、软件定时器
#include "user_interface.h" 	// 系统接口、system_param_xxx接口、WIFI、RateContro


//=====================================全局变量========================================
bool F_LED = 0;					// LED翻转标志位

os_timer_t OS_Timer_1;			// 定义软件定时器OS_Timer_1必须定义为全局变量，因为ESP8266的内核还要使用
os_event_t *Pointer_Task_1;		// 第①步：定义任务指针   os_malloc函数的返回值

struct espconn ST_NetCon;		// 定义espconn型结构体(网络连接结构体) 注：必须定义为全局变量，内核将会使用此变量(内存)
esp_udp ST_UDP;					// UDP通信结构体
//===================================================================================

//====================================== 宏定义=======================================
#define		ProjectName			"AP_UDP_Server"		// 工程名宏定义
#define		ESP8266_AP_SSID		"ESP8266"	// 创建的WIFI名
#define 	ESP6266_AP_PASS		"123456789"	// 创建的WIFI密码
#define		LED_ON				GPIO_OUTPUT_SET(GPIO_ID_PIN(4),0)		// LED亮
#define		LED_OFF				GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1)		// LED灭
//==================================================================================

/* 成功发送网络数据的回调函数    (相对于网络调试助手)*/
void ICACHE_FLASH_ATTR ESP8266_WIFI_Send(void *arg)
{
	os_printf("ESP8266_WIFI_Send OK\r\n");
}

/* 成功接收网络数据的回调函数    (相对于ESP8266)【参数1：网络传输结构体espconn指针、参数2：网络传输数据指针、参数3：数据长度】 */
void ICACHE_FLASH_ATTR ESP8266_WIFI_Read(void * arg, char * pdata, unsigned short len)
{
	struct espconn * T_arg = arg;		// 缓存网络连接结构体指针

	remot_info *P_port_info = NULL;	// 远端连接信息结构体指针

	// 根据数据设置LED的亮/灭
	if((pdata[0] == 'k') || (pdata[0] == 'K')){
		LED_ON;
	}
	if((pdata[0] == 'g') || (pdata[0] == 'G')){
		LED_OFF;
	}
	os_printf("\nESP8266_Receive_Data = %s\n",pdata);		// 串口打印接收到的数据

	// 获取远端信息【UDP通信是无连接的，向远端主机回应时需获取对方的IP/端口信息】
	if(espconn_get_connection_info(T_arg, &P_port_info, 0)==ESPCONN_OK)	// 获取远端信息
	{
		T_arg->proto.udp->remote_port  = P_port_info->remote_port;		// 获取对方端口号
		T_arg->proto.udp->remote_ip[0] = P_port_info->remote_ip[0];		// 获取对方IP地址
		T_arg->proto.udp->remote_ip[1] = P_port_info->remote_ip[1];
		T_arg->proto.udp->remote_ip[2] = P_port_info->remote_ip[2];
		T_arg->proto.udp->remote_ip[3] = P_port_info->remote_ip[3];
		//os_memcpy(T_arg->proto.udp->remote_ip,P_port_info->remote_ip,4);	// 内存拷贝
	}

	OLED_ShowIP(24,6,T_arg->proto.udp->remote_ip);	// 显示远端主机IP地址

	espconn_sent(T_arg,	"ESP8266_WIFI_Recv_OK", os_strlen("ESP8266_WIFI_Recv_OK"));		// 8266向网络调试助手应答
}

/* 初始化网络连接(UDP通信) */
void ESP8266_NetCon_Init(void)
{
	// 通信协议：UDP
	ST_NetCon.type = ESPCONN_UDP;		// 结构体赋值

	// ST_NetCon.proto.udp只是一个指针，不是真正的esp_udp型结构体变量
	//ST_NetCon.proto.udp = &ST_UDP;				//
	ST_NetCon.proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));	// 申请内存

	// 此处无需目标设置Client IP/端口(ESP8266作为Server，不需要预先知道Client(手机、电脑网络调试助手等设备)的IP/端口)
	ST_NetCon.proto.udp->local_port = 8266;			// 设置本地 Server(8266)端口
	//ST_NetCon.proto.tcp->remote_port = 8888;		// 设置目标 Client(手机、电脑网络调试助手等设备)端口  远端主机
	//ST_NetCon.proto.tcp->remote_ip[0] = 192;		// 设置目标IP地址
	//ST_NetCon.proto.tcp->remote_ip[1] = 168;
	//ST_NetCon.proto.tcp->remote_ip[2] = 4;
	//ST_NetCon.proto.tcp->remote_ip[3] = 2;
	//u8 remote_ip[4] = {192.168.4.2};				//设置 Client(手机、电脑网络调试助手等设备)IP地址
	//os_memcpy(ST_NetCon.proto.tcp->remote_ip, remote_ip, 4);

	// 注册/定义回调函数
	espconn_regist_sentcb(&ST_NetCon, ESP8266_WIFI_Send);		// 注册网络数据发送成功的回调函数
	espconn_regist_recvcb(&ST_NetCon, ESP8266_WIFI_Read);		// 注册网络数据接收成功的回调函数

	// 调用UDP初始化API
	espconn_create(&ST_NetCon);			// 初始化UDP通信
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

	//(number, "connect number:%d",wifi_softap_get_station_num());
	//OLED_ShowString(0,4,number);

	os_timer_disarm(&OS_Timer_1);	// 关闭定时器

	ESP8266_NetCon_Init();			// 初始网络连接 （UDP/TCP通信）
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

/* 软件定时器初始化  */
void ICACHE_FLASH_ATTR OS_Timer_1_Init(u32 Timer_ms, u8 timer_repetition)
{
    os_timer_disarm(&OS_Timer_1);   //关闭定时器
    os_timer_setfn(&OS_Timer_1,(os_timer_func_t *)OS_Timer_1_cb, NULL);     //初始化软件定时器
    os_timer_arm(&OS_Timer_1, Timer_ms, timer_repetition);  //使能软件定时器
}

void AP_UDP_Servic_Init(void)		//初始化
{
	os_printf("\r\n=============================================================\r\n");
	os_printf("\t\tSDK	version:%s\r\n",system_get_sdk_version());	//串口打印SDK版本
	os_printf("\t\tProjectName :%s\r\n",ProjectName);
	os_printf("\t\thello world\r\n");
	os_printf("=============================================================\r\n");
	/* ESP8266设置为AP模式 */
	ESP8266_AP_Init();

	/* OLED屏初始化 */
	OLED_Init();								// OLED屏初始化
	OLED_ShowString(0,0,"ESP8266 = AP");		// ESP8266模式
	OLED_ShowString(0,2,"IP:");					// ESP8266_IP地址
	OLED_ShowString(0,4,"Remote  = STA");		// 远端主机模式
	OLED_ShowString(0,6,"IP:");					// 远端主机IP地址

	/* 软件定时器 */
	OS_Timer_1_Init(1000, 1);				// 软件定时器初始化
}
