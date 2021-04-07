#pragma once

#include <network/packet.hpp>
using network::packet;

namespace s2 {
	class netmsg {
	private:
		//netmsg() = default;

		uint32_t mSeq = 0;
		uint8_t mFlags = 0;
		uint16_t mSenderId = 0;
		packet mData;
	public:
		netmsg() = default;

		enum flags : uint8_t {
			FLG_UNK = 1,
			FLG_RELIABLE = 2,
			FLG_ACK = 4
		};

		netmsg(netmsg&& other) noexcept = default;
		netmsg& operator=(netmsg&& other) noexcept = default;
		static netmsg parse(packet& pkt);

		bool operator<(const netmsg& o)const noexcept;

		uint32_t seq()const;
		bool reliable()const;
		bool ack()const;
		const packet& data()const;
		uint16_t senderid()const;
		packet& data();
	};
}
