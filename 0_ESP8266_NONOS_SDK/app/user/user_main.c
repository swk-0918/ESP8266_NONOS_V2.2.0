#include "user_config.h"		// 用户配置
#include "driver/uart.h"  		// 串口
#include "driver/oled.h"  		// OLED
#include "driver/dht11.h"		// DHT11头文件

//#include "at_custom.h"
#include "c_types.h"			// 变量类型
#include "eagle_soc.h"			// GPIO函数、宏定义
//#include "ip_addr.h"			// 被"espconn.h"使用。在"espconn.h"开头#include"ip_addr.h"或#include"ip_addr.h"放在"espconn.h"之前
//#include "espconn.h"
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
bool F_LED = 0;					// LED翻转标志位

os_timer_t OS_Timer_1;			// 定义软件定时器OS_Timer_1必须定义为全局变量，因为ESP8266的内核还要使用
os_event_t *Pointer_Task_1;		// 第①步：定义任务指针   os_malloc函数的返回值

u16 N_Data_FLASH = 0x77;		// 存储数据的扇区编号
u32 Write_Flash[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};	// 写入Flash的数据
u32 Read_Flash[16] = {0};
//===================================================================================

// 宏定义
//==================================================================================
#define		ProjectName			"Task"		// 工程名宏定义
#define		MESSAGE_QUEUE_LEN	4			// 消息队列深度(对于同一个任务，系统最多接受的叠加任务数)
#define 	SPI_FLASH_SIZE		4096		// FLASH扇区大小
//==================================================================================

// 毫秒延时函数
void ICACHE_FLASH_ATTR delay_ms(u32 C_time)
{	for(;C_time>0;C_time--)
		os_delay_us(1000);
}

// 中断服务函数
void GPIO_INTERRUPT(void)
{
	u32 GPIO_STATUS;		//所有IO口中断状态
	u32 GPIO_0_STATUS;		//GPIO0中断状态

	GPIO_STATUS = GPIO_REG_READ(GPIO_STATUS_ADDRESS);		//获取GPIO中断状态
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, GPIO_STATUS);	//清除中断状态 防止持续进入中断

	GPIO_0_STATUS = GPIO_STATUS & (0x01<<0);	//获取GPIO0中断状态

	//判断GPIO0是否发生中断
	if(GPIO_0_STATUS)
	{
		delay_ms(10);
		F_LED = !F_LED;
		GPIO_OUTPUT_SET(GPIO_ID_PIN(4), F_LED);
	}

}

// 软件定时器回调函数
void ICACHE_FLASH_ATTR OS_Timer_1_cb(void)
{
	/* 读取完整的DHT11数据 */
	if(DHT11_Read_Data_Complete() == 0)
	{
		//-------------------------------------------------
		// DHT11_Data_Array[0] == 湿度_整数_部分
		// DHT11_Data_Array[1] == 湿度_小数_部分
		// DHT11_Data_Array[2] == 温度_整数_部分
		// DHT11_Data_Array[3] == 温度_小数_部分
		// DHT11_Data_Array[4] == 校验字节
		// DHT11_Data_Array[5] == 【1:温度>=0】【0:温度<0】
		//-------------------------------------------------

		// 温度超过30℃，LED亮
		//----------------------------------------------------
		if((DHT11_Data_Array[5] == 1) && (DHT11_Data_Array[2] >= 30))
			GPIO_OUTPUT_SET(GPIO_ID_PIN(4),0);		// LED亮
		else
			GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1);		// LED灭

		// 串口输出温湿度
		//---------------------------------------------------------------------------------
		if(DHT11_Data_Array[5] == 1)			// 温度 >= 0℃
		{
			os_printf("\r\n Hum == %d.%d %RH\r\n",DHT11_Data_Array[0],DHT11_Data_Array[1]);
			os_printf("\r\n Tem == %d.%d C\r\n", DHT11_Data_Array[2],DHT11_Data_Array[3]);
		}
		else // if(DHT11_Data_Array[5] == 0)	// 温度 < 0℃
		{
			os_printf("\r\n Hum == %d.%d %RH\r\n",DHT11_Data_Array[0],DHT11_Data_Array[1]);
			os_printf("\r\n Tem == -%d.%d C\r\n",DHT11_Data_Array[2],DHT11_Data_Array[3]);
		}

		// OLED显示温湿度
		//---------------------------------------------------------------------------------
		DHT11_NUM_Char();	// DHT11数据值转成字符串

		OLED_ShowString(0,2,DHT11_Data_Char[0]);	// DHT11_Data_Char[0] == 【湿度字符串】
		OLED_ShowString(0,6,DHT11_Data_Char[1]);	// DHT11_Data_Char[1] == 【温度字符串】
	}
}

// 硬件定时器回调函数
void hw_test_timer_cb(void)
{
}

//任务执行函数(形参：类型必须为【os_event_t *】)		// 第③步：创建任务函数
void Func_Task_1(os_event_t * Task_message)		// Task_message = Pointer_Task_1
{
	// Task_message->sig=消息类型、Task_message->par=消息参数	// 第⑥步：编写任务函数(根据消息类型/消息参数实现相应功能)
	os_printf("Task_message->sig = %d \t Task_message->par = %c \r\n",Task_message->sig, Task_message->par);
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


void ICACHE_FLASH_ATTR user_init(void)
{
	u8 Flash_i = 0;
	u8 C_Task = 0 ;			// 调用任务计数
	u8 Message_Type = 1;	// 消息类型
	u8 Message_Para = 'A';	// 消息参数
	u32 Flash_ID = 0;		// flash的ID

	/* 1、串口 */
	uart_init(74800,74800);				//设置串口1和串口2的波特率
	os_delay_us(10000);					// 等待串口稳定
	os_printf("\r\n=============================================================\r\n");
	os_printf("\t\tSDK	version:%s\r\n",system_get_sdk_version());	//串口打印SDK版本
	os_printf("\t\tProjectName :%s\r\n",ProjectName);
	os_printf("\t\thello world\r\n");
	os_printf("=============================================================\r\n");

	/* 2、GPIO */
	LED_Init();							// LED初始化

	/* 3、中断 */
	//ETS_GPIO_INTR_DISABLE();										// 关闭GPIO中断功能
	//ETS_GPIO_INTR_ATTACH((ets_isr_t)GPIO_INTERRUPT,NULL);			// 注册中断回调函数
	//gpio_pin_intr_state_set(GPIO_ID_PIN(0),GPIO_PIN_INTR_NEGEDGE);// 配置GPIO0下降沿触发中断
	//ETS_GPIO_INTR_ENABLE();										// 使能GPIO中断功能

	/* 4、软件定时器 */
	OS_Timer_1_Init(2000, 1);				// 软件定时器初始化

	/* 5、硬件定时器 */
	//hw_timer_init(0,1);					// 初始化硬件 ISR 定时器 	使⽤ FRC1 中断源  重复 【FRC1_SOURCE==0、NMI_SOURCE=1】
	//hw_timer_set_func(hw_test_timer_cb);	// 设置定时器回调函数
	//hw_timer_arm(500000);					// (单位us，参数必须<=1,677,721)

	/* OLED屏初始化 */
	OLED_Init();								// OLED屏初始化
	OLED_ShowString(0,0,"Humidity:");			// 湿度
	OLED_ShowString(0,4,"Temperature:");		// 温度

	/* 6、任务 */
	/*Pointer_Task_1 = (os_event_t	*)os_malloc(sizeof(os_event_t)*MESSAGE_QUEUE_LEN);	// 给任务1分配空间(任务1空间 = 1个队列空间 * 队列数)	// 第②步：为任务分配内存
	system_os_task(Func_Task_1, USER_TASK_PRIO_0, Pointer_Task_1, MESSAGE_QUEUE_LEN);	// 创建任务(参数1=任务执行函数 / 参数2=任务等级 / 参数3=任务空间指针 / 参数4=消息队列深度)	// 第④步：创建任务

	for(C_Task=0;C_Task<MESSAGE_QUEUE_LEN;C_Task++)	//调用4次任务
	{
		system_soft_wdt_feed();	//喂软件看⻔狗  防止复位

		delay_ms(1000);			// 延时1秒
		os_printf("\r\n assign the task:Task == %d\r\n",C_Task);

		// 调用任务 给任务发信息(参数1=任务等级 / 参数2=消息类型 / 参数3=消息参数)
		// 注意：参数3必须为无符号整数，否则需要强制类型转换
		system_os_post(USER_TASK_PRIO_0, Message_Type++, Message_Para++);	// 第⑤步：给系统安排任务
	}*/

	/* 7、外部Flash */
	/*Flash_ID = spi_flash_get_id();					// 查询 SPI Flash 的 ID
	spi_flash_erase_sector(N_Data_FLASH);			// 擦除 Flash 扇区 0x77		参数==【扇区编号】
	spi_flash_write(N_Data_FLASH * SPI_FLASH_SIZE, (uint32 *)Write_Flash, sizeof(Write_Flash));	// 往Flash写数据 (参数1=【字节地址】、参数2=写入数据的指针、参数3=数据长度)
	spi_flash_read(N_Data_FLASH * SPI_FLASH_SIZE, (uint32 *)Read_Flash, sizeof(Write_Flash));	// 读取Flash里的数据

	os_printf("Flash_ID = %d\r\n",Flash_ID);
	for(Flash_i=0; Flash_i<16; Flash_i++)
	{
		os_printf("Read_Flash_Data_%d = %d\r\n",Flash_i,Read_Flash[Flash_i]);
		delay_ms(10);
	}*/


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



