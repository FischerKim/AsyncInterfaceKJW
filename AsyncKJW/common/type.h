#pragma once
#pragma pack(push,1)

#define II_VERSION "2.0.0" 
#define UDP_UNICAST				0
#define UDP_MULTICAST			1

enum E_INTERFACE_TYPE	//통신 모듈 타입
{
	TYPE_UDP_UNICAST = 0,
	TYPE_UDP_MULTICAST,
	TYPE_TCP_SERVER,
	TYPE_TCP_CLIENT,
	TYPE_SERIAL
};
#pragma pack(pop)