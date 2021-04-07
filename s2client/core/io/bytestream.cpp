#include "bytestream.hpp"

namespace core {
	bytestream::bytestream(uint8_t* data, size_t length)
		: mData(data), mDataLength(length), mReadIdx(0) {
	}

	bool bytestream::read(uint8_t* out, size_t len) {
		if ((mReadIdx + len) > mDataLength)
			return false;
		memcpy(out, &mData[mReadIdx], len);
		mReadIdx += len;
		return true;
	}
	bool bytestream::readBigEndian(uint8_t* out, size_t len) {
		if ((mReadIdx + len) > mDataLength)
			return false;
		for (size_t i = len; i > 0; i--) {
			out[i - 1] = mData[mReadIdx + len - i];
		}
		mReadIdx += len;
		return true;
	}
}
