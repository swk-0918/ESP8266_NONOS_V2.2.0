#include "client.h"
#include "user_config.h"		// 用户配置
#include "ets_sys.h"			// 回调函数


//成功接收到服务器返回数据函数
void ICACHE_FLASH_ATTR user_tcp_recv_cb(void *arg, char *pdata, unsigned short len)
{
	os_printf("\r\n ----- Start accepting data----- \r\n ");
	uart0_tx_buffer(pdata, strlen(pdata));
	os_printf("\r\n -----End of receiving data-----  \r\n ");

}

//发送数据到服务器成功的回调函数
void ICACHE_FLASH_ATTR user_tcp_sent_cb(void *arg)
{
	os_printf("\r\n Data sent successfully \r\n ");
}

//断开服务器成功的回调函数
void ICACHE_FLASH_ATTR user_tcp_discon_cb(void *arg)
{
	os_printf("\r\n Disconnected successfully \r\n ");
}

//连接失败的回调函数，err为错误代码
void ICACHE_FLASH_ATTR user_tcp_recon_cb(void *arg, sint8 err)
{
	os_printf("\r\n Connection error, error code is %d\r\n", err);
	espconn_connect((struct espconn *) arg);
}

//成功连接到服务器的回调函数
void ICACHE_FLASH_ATTR user_tcp_connect_cb(void *arg)
{
	struct espconn *pespconn = arg;
	espconn_regist_recvcb(pespconn, user_tcp_recv_cb);		// 注册网络数据发送成功的回调函数
	espconn_regist_sentcb(pespconn, user_tcp_sent_cb);		// 注册网络数据接收成功的回调函数
	espconn_regist_disconcb(pespconn, user_tcp_discon_cb);	// 注册成功断开TCP连接的回调函数

	os_printf("\r\n ----- Request data start----- \r\n");
	uart0_tx_buffer(buffer, strlen(buffer));
	os_printf("\r\n -----End of request data-----  \r\n");

	espconn_sent(pespconn, buffer, strlen(buffer));

}
void ICACHE_FLASH_ATTR my_station_init(struct ip_addr *remote_ip, struct ip_addr *local_ip, int remote_port)
{
	//配置
	user_tcp_conn.type = ESPCONN_TCP;		// 结构体赋值
	user_tcp_conn.state = ESPCONN_NONE;		// espconn的当前状态

	user_tcp_conn.proto.tcp = (esp_tcp *) os_zalloc(sizeof(esp_tcp));	// 申请内存

	os_memcpy(user_tcp_conn.proto.tcp->local_ip, local_ip, 4);			// 本地IP
	os_memcpy(user_tcp_conn.proto.tcp->remote_ip, remote_ip, 4);		// 远端IP

	user_tcp_conn.proto.tcp->local_port = espconn_port();				// 获取 ESP8266 可⽤的端⼝
	user_tcp_conn.proto.tcp->remote_port = remote_port;					// 目标端口【HTTP端口号80】
	//注册
	espconn_regist_connectcb(&user_tcp_conn, user_tcp_connect_cb);		// 注册TCP连接成功建立的回调函数
	espconn_regist_reconcb(&user_tcp_conn, user_tcp_recon_cb);			// 注册TCP连接异常断开的回调函数
	//连接服务器
	espconn_connect(&user_tcp_conn);									// 创建TCP_server，建立侦听
}
