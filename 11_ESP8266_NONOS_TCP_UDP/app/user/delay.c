#include "os_type.h"			// os_XXX
#include "osapi.h"  			// os_XXX、软件定时器

// 毫秒延时函数
void ICACHE_FLASH_ATTR delay_ms(u32 C_time)
{
	for(;C_time>0;C_time--)
	{
		os_delay_us(1000);
	}
}
