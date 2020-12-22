#ifndef APP_INCLUDE_CLIENT_H_
#define APP_INCLUDE_CLIENT_H_

#include "user_main.h"
#include "espconn.h"
#include "mem.h"

char buffer[1024];
#define GET "GET /%s HTTP/1.1\r\nContent-Type: text/html;charset=utf-8\r\nAccept: */*\r\nHost: %s\r\nConnection: Keep-Alive\r\n\r\n"
#define POST "POST /%s HTTP/1.1\r\nAccept: */*\r\nContent-Length: %d\r\nContent-Type: application/json\r\nHost: %s\r\nConnection: Keep-Alive\r\n\r\n%s"
struct espconn user_tcp_conn;		// 定义espconn型结构体(网络连接结构体) 注：必须定义为全局变量，内核将会使用此变量(内存)


void my_station_init(struct ip_addr *remote_ip,struct ip_addr *local_ip,int remote_port);

#endif /* APP_INCLUDE_CLIENT_H_ */
