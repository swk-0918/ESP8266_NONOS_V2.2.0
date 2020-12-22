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

struct espconn ST_NetCon;		// 定义espconn型结构体(网络连接结构体) 注：必须定义为全局变量，内核将会使用此变量(内存)
esp_tcp ST_TCP;					// TCP通信结构体
//===================================================================================

// 宏定义
//==================================================================================
#define		ProjectName			"TCP_Server"		// 工程名宏定义
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

/* 成功发送网络数据的回调函数    (相对于网络调试助手)*/
void ICACHE_FLASH_ATTR ESP8266_WIFI_Send(void *arg)
{
	os_printf("ESP8266_WIFI_Send OK\r\n");
}

/* 成功接收网络数据的回调函数    (相对于ESP8266)【参数1：网络传输结构体espconn指针、参数2：网络传输数据指针、参数3：数据长度】 */
void ICACHE_FLASH_ATTR ESP8266_WIFI_Recv(void * arg, char * pdata, unsigned short len)
{
	struct espconn * T_arg = arg;		// 缓存网络连接结构体指针

	// 根据数据设置LED的亮/灭
	if((pdata[0] == 'k') || (pdata[0] == 'K')){
		LED_ON;
	}
	if((pdata[0] == 'g') || (pdata[0] == 'G')){
		LED_OFF;
	}
	os_printf("\nESP8266_Receive_Data = %s\n",pdata);		// 串口打印接收到的数据

	OLED_ShowIP(24,6,T_arg->proto.udp->remote_ip);	// 显示远端主机IP地址

	//【TCP通信是面向连接的，向远端主机回应时可直接使用T_arg结构体指针指向的IP信息】
	espconn_sent(T_arg,	"ESP8266_WIFI_Recv_OK", os_strlen("ESP8266_WIFI_Recv_OK"));		// 8266向网络调试助手应答
}

/* TCP连接断开成功的回调函数  */
void ICACHE_FLASH_ATTR ESP8266_TCP_Disconnect(void *arg)
{
	os_printf("\nESP8266_TCP_Disconnect_OK\n");
}

/* TCP连接建立成功的回调函数  */
void ESP8266_TCP_Connect(void *arg)
{
	espconn_regist_sentcb((struct espconn *)arg, ESP8266_WIFI_Send);			// 注册网络数据发送成功的回调函数
	espconn_regist_recvcb((struct espconn *)arg, ESP8266_WIFI_Recv);			// 注册网络数据接收成功的回调函数
	espconn_regist_disconcb((struct espconn *)arg,ESP8266_TCP_Disconnect);		// 注册成功断开TCP连接的回调函数

	os_printf("\n--------------- ESP8266_TCP_Connect_OK ---------------\n");
}

/* TCP连接异常断开时的回调函数   */
void ICACHE_FLASH_ATTR ESP8266_TCP_Break(void *arg, sint8 err)
{
	os_printf("\nESP8266_TCP_Break\n");
}

/* 初始化网络连接(TCP通信) */
void ESP8266_NetCon_Init(void)
{
	// 通信协议：TCP
	ST_NetCon.type = ESPCONN_TCP;		// 结构体赋值

	// ST_NetCon.proto.tcp只是一个指针，不是真正的esp_udp型结构体变量
	//ST_NetCon.proto.tcp = &ST_TCP;				//
	ST_NetCon.proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));	// 申请内存

	// 此处无需设置Client IP/端口(ESP8266作为Server，不需要预先知道Client(手机、电脑网络调试助手等设备)的IP/端口)
	ST_NetCon.proto.udp->local_port = 8266;		// 设置本地 Server(8266)端口
	//ST_NetCon.proto.tcp->remote_port = 8888;	// 设置目标 Client(手机、电脑网络调试助手等设备)端口  远端主机
	//ST_NetCon.proto.tcp->remote_ip[0] = 192;	// 设置目标 IP地址
	//ST_NetCon.proto.tcp->remote_ip[1] = 168;
	//ST_NetCon.proto.tcp->remote_ip[2] = 4;
	//ST_NetCon.proto.tcp->remote_ip[3] = 2;
	//u8 remote_ip[4] = {192.168.4.2};			//设置 Client(手机、电脑网络调试助手等设备)IP地址
	//os_memcpy(ST_NetCon.proto.tcp->remote_ip, remote_ip, 4);

	// 注册连接成功回调函数、异常断开回调函数
	espconn_regist_connectcb(&ST_NetCon, ESP8266_TCP_Connect);	// 注册TCP连接成功建立的回调函数
	espconn_regist_reconcb(&ST_NetCon, ESP8266_TCP_Break);		// 注册TCP连接异常断开的回调函数

	// 创建TCP_server，建立侦听
	espconn_accept(&ST_NetCon);		// 创建TCP_server，建立侦听

	// 在 espconn_accept 之后  连接未建立之前  注册 ESP8266 TCP server 超时时间
	// 如果超时时间设置为 0，ESP8266_TCP_server将始终不会断开已经不与它通信的TCP_client，不建议这样使用。
	espconn_regist_time(&ST_NetCon, 300, 0); 	//设置超时断开时间   单位=秒，最大值=7200
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
	wifi_get_ip_info(SOFTAP_IF, &ST_ESP8266_IP);	// 查询AP模式下ESP8266的IP地址

	if(ST_ESP8266_IP.ip.addr != 0)
	{
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
}

/* LED初始化  */
void ICACHE_FLASH_ATTR LED_Init(void)
{
	// 设置GPIO4 - LED
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);	//设置GPIO4为IO口功能
	GPIO_OUTPUT_SET(GPIO_ID_PIN(4), 1);						//将相应管脚设为输出模式 并输出高电平

	//设置GPIO0 - BOOT
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);	//设置GPIO0为IO口功能
	GPIO_DIS_OUTPUT(GPIO_ID_PIN(0));						//失能GPIO0输出功能
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_GPIO0_U);					//失能GPIO0内部上拉

	// 注意：【PIN_NAME】、【FUNC】、【gpio_no】不要混淆
	//…………………………………………………………………………………………………
	//·【PIN_NAME】		管脚名称		"PERIPHS_IO_MUX_" + "管脚名"
	//·【FUNC】			管脚功能		功能序号 - 1
	//·【gpio_no】		IO端口序号		GPIO_ID_PIN(IO端口序号)
	//…………………………………………………………………………………………………
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

	/* 2、GPIO */
	LED_Init();							// LED初始化

	/* 4、软件定时器 */
	OS_Timer_1_Init(1000, 1);				// 软件定时器初始化

	/* OLED屏初始化 */
	OLED_Init();								// OLED屏初始化
	OLED_ShowString(0,0,"ESP8266 = AP");		// ESP8266模式
	OLED_ShowString(0,2,"IP:");					// ESP8266_IP地址
	OLED_ShowString(0,4,"Remote  = STA");		// 远端主机模式
	OLED_ShowString(0,6,"IP:");					// 远端主机IP地址

	/* 8、AP模式 */
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



