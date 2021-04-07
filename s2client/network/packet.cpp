#include "packet.hpp"

namespace network {
	packet::packet() noexcept
		: mData(), mReadIdx(0) {
	}

	packet::packet(string&& s) : mReadIdx(0) {
		write((const uint8_t*)s.data(), s.length());
	}

	packet::packet(packet&& other) noexcept
		: mData(std::move(other.mData)), mReadIdx(other.mReadIdx) {
	}

	const packet& packet::operator=(packet&& other) noexcept {
		mData = std::move(other.mData);
		mReadIdx = other.mReadIdx;
		return *this;
	}

	uint8_t* packet::data() {
		return mData.data();
	}
	uint8_t* packet::nextdata() {
		return mData.data() + mReadIdx;
	}

	const uint8_t* packet::data() const {
		return mData.data();
	}

	size_t packet::length() const {
		return mData.size();
	}

	void packet::clear() {
		mData.clear();
	}

	void packet::resize(size_t length) {
		mData.resize(length);
	}

	size_t packet::tell()const {
		return mReadIdx;
	}
	void packet::seek(size_t idx) {
		mReadIdx = max(size_t(0), min(length(), idx));
	}
	void packet::advance(int64_t offs) {
		mReadIdx += offs;
		assert(mReadIdx <= length());
	}

	size_t packet::remaining() const {
		return (length() - mReadIdx);
	}

	bool packet::end() const {
		return mReadIdx >= length();
	}

	bool packet::read(uint8_t* data, size_t length) {
		assert((mReadIdx + length) <= mData.size());
		if ((mReadIdx + length) > mData.size())
			return false;
		memcpy(data, mData.data() + mReadIdx, length);
		mReadIdx += length;
		return true;
	}

	uint8_t packet::readbyte() {
		return read<uint8_t>();
	}

	uint16_t packet::readword() {
		return read<uint16_t>();
	}

	uint32_t packet::readdword() {
		return read<uint32_t>();
	}

	uint64_t packet::readqword() {
		return read<uint64_t>();
	}

	float packet::readsingle() {
		return read<float>();
	}
	string packet::readstring() {
		string str;
		for (char c = read<char>(); c != '\x00'; c = read<char>()) {
			str.append(1, c);
		}
		return str;
	}

	void packet::write(const uint8_t* data, size_t length) {
		mData.insert(mData.end(), data, data + length);
	}
	void packet::writebyte(uint8_t b) {
		return write<uint8_t>(b);
	}
	void packet::writeword(uint16_t w) {
		return write<uint16_t>(w);
	}
	void packet::writedword(uint32_t dw) {
		return write<uint32_t>(dw);
	}
	void packet::writeqword(uint64_t qw) {
		return write<uint64_t>(qw);
	}
	void packet::writesingle(float s) {
		return write<float>(s);
	}
	void packet::writestring(string_view s) {
		write((const uint8_t*)s.data(), s.length());
		writebyte(0);
	}
}
