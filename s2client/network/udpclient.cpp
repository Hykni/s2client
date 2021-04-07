#include "udpclient.hpp"

#include <network/network.hpp>

#include <core/io/logger.hpp>

namespace network {
	udpclient::udpclient(const char* hostname, int port) {
		mSock = network::udpsocket();
		int timeout = 50;
		setsockopt(mSock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
		network::resolveaddress(&mServer, hostname, port);
	}
	udpclient::~udpclient() {
		closesocket(mSock);
	}

	bool udpclient::isreadpending(int msTimeout) const {
		return network::isreadavailable(mSock, msTimeout);
	}

	int udpclient::send(const packet& p)const {
		int r = sendto(mSock, (const char*)p.data(), static_cast<int>(p.length()), 0, (const sockaddr*)&mServer, sizeof(mServer));
		if (r == SOCKET_ERROR) {
			core::error("sendto() failed %X", WSAGetLastError());
		}
		return r;
	}
	int udpclient::recv(packet* out) const {
		sockaddr_in from;
		int fromlen = sizeof(sockaddr_in);

		out->clear();
		out->resize(network::MAX_PACKET_SIZE);
		int nbytes = recvfrom(mSock, (char*)out->data(), static_cast<int>(out->length()), 0, (sockaddr*)&from, &fromlen);
		if (nbytes == SOCKET_ERROR) {
			auto err = WSAGetLastError();
			if (err == WSAETIMEDOUT)
				return -1;
			core::warning("recvfrom() winsock error %d (%Xh)\n", err, err);
			return -1;
		}
		else {
			out->resize(nbytes);
			assert(from.sin_addr.s_addr == mServer.sin_addr.s_addr);
			return nbytes;
		}
	}
}
