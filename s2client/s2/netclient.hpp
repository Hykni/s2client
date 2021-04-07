#pragma once

#include <core/prerequisites.hpp>
#include <network/network.hpp>
#include <network/udpclient.hpp>
#include <s2/netmsg.hpp>
using network::packet;

namespace s2 {
	class netclient : protected network::udpclient {
	private:
		uint32_t mSeqNo;
		uint32_t mExpectedSeq;
		uint32_t mClientId;
		packet mRecvData;
		set<netmsg> mQueuedSeqs;

		packet newframe(uint32_t seq, uint8_t flags=0);
	public:
		netclient(const char* hostname, int port);

		uint32_t clientid()const;

		void reset();
		bool readmsg(netmsg* result);

		int sendunreliable(uint8_t cmdid, packet&& data);
		int sendreliable(uint8_t cmdid, packet&& data);
		int sendreliable(uint8_t cmdid);
		int sendack(uint32_t seqno);
	};
}
