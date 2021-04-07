#pragma once

#include <core/prerequisites.hpp>
#include <random>

namespace core {
	class random {
		__forceinline static random& Instance() {
			static random _Instance;
			return _Instance;
		}
		std::mt19937_64 mGen;
		std::random_device mRd;

		random() : mRd(), mGen(mRd()) {
		}
	public:
		static uint32_t uint32() {
			return static_cast<uint32_t>(Instance().mGen() & 0xFFFFFFFF);
		}
		static uint64_t uint64() {
			return static_cast<uint64_t>(Instance().mGen());
		}
		static uint32_t uint32(uint32_t max) {
			return uint32() % max;
		}
		static int32_t int32(int32_t max) {
			return static_cast<int32_t>(uint32() % max);
		}
		static int32_t int32(int32_t min, int32_t max) {
			return min + static_cast<int32_t>(uint32() % (max - min));
		}
	};
}
