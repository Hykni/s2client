#include <network/network.hpp>

namespace network {
	namespace _impl {
		WSADATA wsaData;
	};
	
	void init() {
		WSAStartup(MAKEWORD(2, 2), &_impl::wsaData);
	}

	SOCKET udpsocket() {
		return socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	}

	SOCKET tcpsocket() {
		return socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	}

	bool resolveaddress(sockaddr_in* result, const char* hostname, int port)
	{
		addrinfo* tmp = nullptr;
		addrinfo hints; memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_protocol = 0;
		if (0 == getaddrinfo(hostname, "0", &hints, &tmp)) {
			memset(result, 0, sizeof(sockaddr_in));
			result->sin_family = AF_INET;
			result->sin_addr.S_un.S_addr = ((sockaddr_in*)tmp->ai_addr)->sin_addr.S_un.S_addr;
			result->sin_port = htons(port);
			freeaddrinfo(tmp);
			return true;
		}
		return false;
	}

	bool isreadavailable(SOCKET s, int msTimeout) {
		fd_set readset = { 1, { s } };
		timeval timeout = { 0 };
		timeout.tv_usec = msTimeout * 1000;
		auto result = select(1, &readset, nullptr, nullptr, &timeout);
		assert(result != SOCKET_ERROR);
		return result > 0;
	}

	void destroy() {
		WSACleanup();
	}
};
