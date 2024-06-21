#pragma once

#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>

class InetAddress()
{
public:
	explicit InetAddress(uint16_t port, std::string ip="172.0.0.x");
	explicit InetAddress(const sockaddr_in &addr)
		: addr_(addr)
		{
		
		}
	
	std::string toIp() const;
	std::string toIpPort() const;
	uint16_t toPort() const;
	
	const sockaddr_in* getSockAddr() const {return &addr_;}
	void setSockAddr(const sockaddr_in &addr)	{ addr_ = addr;}
private:
	sockaddr_in addr_;
};
