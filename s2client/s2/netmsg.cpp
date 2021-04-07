#include "netmsg.hpp"

namespace s2 {
	netmsg netmsg::parse(packet& pkt) {
		netmsg m;
		m.mSeq = pkt.readdword();
		m.mFlags = pkt.readbyte();
		m.mSenderId = pkt.readword();
		m.mData.resize(pkt.remaining());
		pkt.read(m.mData.data(), pkt.remaining());
		return m;
	}
	bool netmsg::operator<(const netmsg& o) const noexcept {
		return seq() < o.seq();
	}
	uint32_t netmsg::seq()const {
		return mSeq;
	}
	bool netmsg::reliable()const {
		return mFlags & FLG_RELIABLE;
	}
	bool netmsg::ack()const {
		return mFlags & FLG_ACK;
	}

	const packet& netmsg::data()const {
		return mData;
	}
	uint16_t netmsg::senderid() const {
		return mSenderId;
	}
	packet& netmsg::data() {
		return mData;
	}
}
