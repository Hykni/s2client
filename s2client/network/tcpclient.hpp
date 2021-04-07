#pragma once

#include <core/prerequisites.hpp>
#include <network/packet.hpp>

namespace network {
	class tcpclient {
	private:
		SOCKET mSock;
		bool mConnected;
		sockaddr_in mServer;
	public:
		tcpclient();
		~tcpclient();

		bool connect(const char* hostname, int port);
		void disconnect();

		bool connected()const;
		bool isreadpending()const;

		int send(const packet& p)const;
		int recv(packet* out)const;
		int recvstring(string* out)const;
	};
}
