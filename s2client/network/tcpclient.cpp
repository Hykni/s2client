#include "tcpclient.hpp"
#include <network/network.hpp>

namespace network {
	tcpclient::tcpclient()
		: mConnected(false) {
		mServer = { 0 };
		mSock = network::tcpsocket();
	}

	tcpclient::~tcpclient() {
		closesocket(mSock);
	}

	bool tcpclient::connect(const char* hostname, int port) {
		if(!network::resolveaddress(&mServer, hostname, port))
			return false;
		mConnected = SOCKET_ERROR != ::connect(mSock, (sockaddr*)&mServer, sizeof(mServer));
		if (!mConnected) {
			auto err = WSAGetLastError();
			OutputDebugStringA(("Winsock error: " + std::to_string(err)).c_str());
		}
		return mConnected;
	}

	void tcpclient::disconnect() {
		closesocket(mSock);
		mSock = network::tcpsocket();
	}

	bool tcpclient::connected() const {
		return mConnected;
	}
	bool tcpclient::isreadpending() const {
		return network::isreadavailable(mSock);
	}

	int tcpclient::send(const packet& p) const {
		return ::send(mSock, reinterpret_cast<const char*>(p.data()), static_cast<int>(p.length()), 0);
	}
	int tcpclient::recv(packet* out) const {
		out->clear();
		out->resize(network::MAX_PACKET_SIZE);
		int nbytes = 0, totalbytes = 0;
		do {
			nbytes = ::recv(mSock, reinterpret_cast<char*>(out->data() + totalbytes), static_cast<int>(out->length() - totalbytes), 0);
			totalbytes += nbytes;
		} while (nbytes != 0);
		out->resize(totalbytes);
		return totalbytes;
	}
	int tcpclient::recvstring(string* out) const {
		out->clear();
		out->resize(network::MAX_PACKET_SIZE);
		int nbytes = 0, totalbytes = 0;
		do {
			nbytes = ::recv(mSock, reinterpret_cast<char*>(out->data() + totalbytes), static_cast<int>(out->length() - totalbytes), 0);
			totalbytes += nbytes;
		} while (nbytes != 0);
		out->resize(totalbytes);
		return totalbytes;
	}
}
