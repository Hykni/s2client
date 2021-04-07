#pragma once

#include <core/prerequisites.hpp>

namespace core {
	class bitvector {
		class bit {
			uint8_t& b;
			int idx;
		public:
			bit(uint8_t& B, int Idx) : b(B), idx(Idx&7) { }
			bit& operator=(bool v) {
				if (v)
					b |= (1 << idx);
				else
					b &= ~(1 << idx);
				return *this;
			}
			operator bool()const {
				return 0 != (b & (1 << idx));
			}
		};
		vector<uint8_t> mBs;
		size_t mCount;
	public:
		bitvector(size_t nbits=0) : mCount(nbits) {
			if(nbits > 0)
				mBs.resize(((nbits - 1) / 8) + 1);
		}

		void append(bool b) {
			if ((mCount / 8) >= mBs.size()) {
				mBs.push_back(b);
			}
			else {
				(*this)[mCount] = b;
			}
			mCount++;
		}

		bool operator[](size_t idx)const {
			return mBs[idx / 8] & (1 << (idx & 7));
		}
		bit operator[](size_t idx) {
			return bit(mBs[idx / 8], idx & 7);
		}
		vector<uint8_t> bytes()const {
			return mBs;
		}
	};
}
