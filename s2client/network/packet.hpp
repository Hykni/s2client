#pragma once

#include <core/prerequisites.hpp>

namespace network {
	class packet {
	private:
		vector<uint8_t> mData;
		size_t mReadIdx;
	public:
		packet() noexcept;
		packet(string&& s);
		packet(const packet& o) = delete;
		packet(packet&& other) noexcept;

		const packet& operator=(const packet& other) = delete;
		const packet& operator=(packet&& other) noexcept;

		uint8_t* data();
		uint8_t* nextdata();
		const uint8_t* data()const;
		size_t length()const;
		void clear();
		void resize(size_t length);

		size_t tell()const;
		void seek(size_t idx);
		void advance(int64_t offs);
		size_t remaining()const;
		bool end()const;
		bool read(uint8_t* data, size_t length);

		template<typename T>
		T read() {
			T out;
			if (!read(reinterpret_cast<uint8_t*>(&out), sizeof(T)))
				assert(FALSE);
			return out;
		}
		uint8_t  readbyte();
		uint16_t readword();
		uint32_t readdword();
		uint64_t readqword();
		float    readsingle();
		string   readstring();

		void write(const uint8_t* data, size_t length);

		template<typename T>
		void write(const T& data) {
			return write(reinterpret_cast<const uint8_t*>(&data), sizeof(T));
		}
		void writebyte(uint8_t b);
		void writeword(uint16_t w);
		void writedword(uint32_t dw);
		void writeqword(uint64_t qw);
		void writesingle(float s);
		void writestring(string_view s);
	};
}
