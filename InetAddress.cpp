#include "InetAddress.h"
#include <string.h>

InetAddress::InetAddress(uint16_t port, std::string ip)
{
	bzero(&addr_, sizeof addr_);	// 清0
	addr_.sin_family = AF_INET;		// 地址家族，IPv4网络协议的套接字类型，用于互联网通信的Socket域
	addr_.sin_port = htons(port);	// host2net，将信息转成网络字节区，把证数据可识别
	addr_.sin_addr.s_addr = inet_addr(ip.c_str());	// inet_addr，也会自动转换网络字节区，并完成ip的整数化
}

std::string InetAddress::toIp() const
{
	char buf[64] = {0};
	::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);	// 将ip从网络字节区转换到本地
	return buf;
}

std::string InetAddress::toIpPort() const
{
    char buf[64] = " ";
	::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);	// 将ip从网络字节区转换到本地字节区
	size_t end = strlen(buf);
	uint16_t port = ntohs(addr_.sin_port);
	sprintf(buf+end, ":%u", port);	// 将ip和port捆绑一起返回了
	return buf;
}

uint16_t InetAddress::toPort() const
{	
	return ntohs(addr_.sin_port);
}

int main(){
	InetAddress addr(8000);	// 这个8000是port还是addr_ ???
	std::cout << addr.toIpPort() << std:;endl;
	
	return 0;
}