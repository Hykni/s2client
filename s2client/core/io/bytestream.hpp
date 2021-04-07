#pragma once

#include <core/prerequisites.hpp>

namespace core {
	class bytestream {
	protected:
		uint8_t* mData;
		size_t mDataLength;
		size_t mReadIdx;
	public:
		bytestream(uint8_t* data, size_t length);

		size_t tell() {
			return mReadIdx;
		}
		void seek(size_t idx) {
			mReadIdx = idx;
		}
		void advance(int offs) {
			mReadIdx += offs;
		}
		uint8_t* data() {
			return mData;
		}
		size_t length() {
			return mDataLength;
		}

		bool eof()const {
			return mReadIdx >= mDataLength;
		}

		bool read(uint8_t* out, size_t len);
		bool readBigEndian(uint8_t* out, size_t len);

		template<typename T>
		T read() {
			T r;
			read((uint8_t*)&r, sizeof(T));
			return r;
		}
		template<typename T>
		T readBigEndian() {
			T r;
			readBigEndian((uint8_t*)&r, sizeof(T));
			return r;
		}

		char     readChar()   { return read<char>(); }
		uint8_t  readByte()   { return read<uint8_t>(); }
		uint16_t readWord()   { return read<uint16_t>(); }
		int16_t  readShort()  { return read<int16_t>(); }
		int32_t  readInt()    { return read<int32_t>(); }
		uint32_t readDword() { return read<uint32_t>(); }
		uint32_t readDwordBE() { return readBigEndian<uint32_t>(); }
		uint64_t readQword()  { return read<uint64_t>(); }
		float    readFloat()  { return read<float>(); }
		double   readDouble() { return read<double>(); }
		string   readString(int len = -1) {
			string result;
			if(len == -1) {
				for (char c; (c = readChar()) != '\x00';)
					result.push_back(c);
			}
			else {
				result.reserve(len);
				for (int i = 0; i < len; i++)
					result.push_back(readChar());
			}
			return result;
		}
		wstring readWString(int len = -1) {
			wstring result;
			if (len == -1) {
				for (wchar_t c; (c = readWord()) != '\x00';)
					result.push_back(c);
			}
			else {
				result.resize(len);
				for (int i = 0; i < len; i++)
					result.push_back(readWord());
			}
			return result;
		}
	};
}
