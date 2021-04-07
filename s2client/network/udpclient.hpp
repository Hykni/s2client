#pragma once

#include <core/prerequisites.hpp>
#include <network/packet.hpp>

namespace network {
	class udpclient {
	private:
		SOCKET mSock;
	public:
		udpclient(const char* hostname, int port);
		~udpclient();

		bool isreadpending(int msTimeout=50)const;

		int send(const packet& p)const;
		int recv(packet* out)const;

	private:
		sockaddr_in mServer;
	};
}
