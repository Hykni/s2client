#include "netclient.hpp"
#include <core/utils/random.hpp>
#include <core/io/logger.hpp>
#include <s2/consts.hpp>

using core::random;

namespace s2 {
	packet netclient::newframe(uint32_t seq, uint8_t flags) {
		packet frame;
		frame.writedword(seq);
		frame.writebyte(1 | flags);
		frame.writeword(mClientId);
		return frame;
	}
	netclient::netclient(const char* hostname, int port)
		: udpclient(hostname, port) {
		reset();
	}

	uint32_t netclient::clientid() const {
		return mClientId;
	}

	void netclient::reset() {
		mClientId = random::uint32();
		mSeqNo = 1;
		mExpectedSeq = 1;
		mQueuedSeqs.clear();
	}
	bool netclient::readmsg(netmsg* result) {
		if (!mQueuedSeqs.empty() && mQueuedSeqs.begin()->seq() == mExpectedSeq) {
			*result = std::move(mQueuedSeqs.extract(mQueuedSeqs.begin()).value());
			core::info("Returning queued seq %Xh\n", mExpectedSeq);
			mExpectedSeq++;
			return true;
		}
		if (!isreadpending())
			return false;
		if (this->recv(&mRecvData) > 0) {
			mRecvData.seek(0);
			netmsg msg = netmsg::parse(mRecvData);
			if (msg.reliable()) {
				sendack(msg.seq());
				if (msg.seq() != mExpectedSeq) {
					if (msg.seq() > mExpectedSeq) {
						core::info("Queueing seq=%Xh received out of order\n", msg.seq());
						mQueuedSeqs.emplace(std::move(msg));
					}
					else {
						core::warning("Discarding previously seen seq=%Xh\n", msg.seq());
					}
					// return next msg if available
					return isreadpending() ? readmsg(result) : false;
				}
				else {
					mExpectedSeq++;
					//core::info("Returning msg seq=%Xh\n", msg.seq());
				}
			}
			else if (msg.ack()) {
				return isreadpending() ? readmsg(result) : false;
			}
			*result = std::move(msg);
			return true;
		}

		return false;
	}
	int netclient::sendunreliable(uint8_t cmdid, packet&& data) {
		packet pkt(newframe(consts::SeqUnreliable));
		pkt.writebyte(cmdid);
		pkt.write(data.data(), data.length());
		return send(pkt);
	}
	int netclient::sendreliable(uint8_t cmdid, packet&& data) {
		packet pkt(newframe(mSeqNo++, netmsg::FLG_RELIABLE));
		pkt.writebyte(cmdid);
		pkt.write(data.data(), data.length());
		return send(pkt);
	}
	int netclient::sendreliable(uint8_t cmdid) {
		packet pkt(newframe(mSeqNo++, netmsg::FLG_RELIABLE));
		pkt.writebyte(cmdid);
		return send(pkt);
	}
	int netclient::sendack(uint32_t seqno) {
		packet pkt(newframe(consts::SeqUnreliable, netmsg::FLG_ACK));
		pkt.writedword(seqno);
		return send(pkt);
	}
}
