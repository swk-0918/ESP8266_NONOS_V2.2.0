#include "c_types.h"			// ��������
#include "eagle_soc.h"			// GPIO�������궨��

#define		LED_ON				GPIO_OUTPUT_SET(GPIO_ID_PIN(4),0)		// LED��
#define		LED_OFF				GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1)		// LED��

/* LED��ʼ��  */
void ICACHE_FLASH_ATTR LED_Init(void)
{
	// ע�⣺��PIN_NAME������FUNC������gpio_no����Ҫ����
	//��������������������������������������������������������������������������
	//����PIN_NAME��		�ܽ�����		"PERIPHS_IO_MUX_" + "�ܽ���"
	//����FUNC��			�ܽŹ���		������� - 1
	//����gpio_no��		IO�˿����		GPIO_ID_PIN(IO�˿����)
	//��������������������������������������������������������������������������

	// ����GPIO4 - LED
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);	//����GPIO4ΪIO�ڹ���
	GPIO_OUTPUT_SET(GPIO_ID_PIN(4), 1);						//����Ӧ�ܽ���Ϊ���ģʽ ������ߵ�ƽ

	//����GPIO0 - BOOT
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);	//����GPIO0ΪIO�ڹ���
	GPIO_DIS_OUTPUT(GPIO_ID_PIN(0));						//ʧ��GPIO0�������
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_GPIO0_U);					//ʧ��GPIO0�ڲ�����
}

