#include "c_types.h"			// 变量类型
#include "eagle_soc.h"			// GPIO函数、宏定义

#define		LED_ON				GPIO_OUTPUT_SET(GPIO_ID_PIN(4),0)		// LED亮
#define		LED_OFF				GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1)		// LED灭

/* LED初始化  */
void ICACHE_FLASH_ATTR LED_Init(void)
{
	// 注意：【PIN_NAME】、【FUNC】、【gpio_no】不要混淆
	//…………………………………………………………………………………………………
	//·【PIN_NAME】		管脚名称		"PERIPHS_IO_MUX_" + "管脚名"
	//·【FUNC】			管脚功能		功能序号 - 1
	//·【gpio_no】		IO端口序号		GPIO_ID_PIN(IO端口序号)
	//…………………………………………………………………………………………………

	// 设置GPIO4 - LED
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);	//设置GPIO4为IO口功能
	GPIO_OUTPUT_SET(GPIO_ID_PIN(4), 1);						//将相应管脚设为输出模式 并输出高电平

	//设置GPIO0 - BOOT
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);	//设置GPIO0为IO口功能
	GPIO_DIS_OUTPUT(GPIO_ID_PIN(0));						//失能GPIO0输出功能
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_GPIO0_U);					//失能GPIO0内部上拉
}

