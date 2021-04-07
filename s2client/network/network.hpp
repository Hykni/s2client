#pragma once

#include <core/prerequisites.hpp>

namespace network {
	static const int MAX_PACKET_SIZE = 8192;

	void init();

	SOCKET udpsocket();
	SOCKET tcpsocket();

	bool resolveaddress(sockaddr_in* result, const char* hostname, int port);
	
	bool isreadavailable(SOCKET s, int msTimeout=50);

	void destroy();
}
