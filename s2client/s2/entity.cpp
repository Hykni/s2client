#include "entity.hpp"
#include <core/utils/bitvector.hpp>

namespace s2 {
	string_view entity::typname() const {
        return mTypeInfo->name;
	}
	int entity::type() const {
		return mEntType;
	}
	int entity::id()const {
		return mEntId;
	}
	const typeinfo& entity::typevec() const {
		return *mTypeInfo;
	}
	entity::entity(int entid, int type) : mEntId(entid), mEntType(type) {
		typeregistry::LookupTypeInfo(type, &mTypeInfo);
	}

    static vector<uint8_t> bits2htree(uint8_t* bs, int nbits) {
        size_t cpo2 = 1;
        for (; cpo2 < nbits; cpo2 *= 2);
        core::bitvector bv(2 * cpo2);
        auto bcount = [&](int s, int e) -> int {
            int c = 0;
            for (int i = s; i < e; i++) {
                c += (bs[i / 8] & (1 << (i & 7))) ? 1 : 0;
            }
            return c;
        };
        for (size_t idx = 0; idx < cpo2; idx++) {
            if (bs[idx / 8] & (1 << (idx & 7))) {
                for (size_t i = (cpo2 + idx); i >= 1; i >>= 1)
                    bv[i] = 1;
            }
        }

        return bv.bytes();
    }

    // Decompress hierarchical bitflag structure
    static vector<uint8_t> bfhdecomp(int nfields, packet* pkt) {
        size_t cpo2 = 1;
        for (; cpo2 < nfields; cpo2 *= 2);
        if (cpo2 <= 8)
            return { pkt->readbyte() };
        else {
            core::bitvector bv(8 * (((cpo2 - 1) / 4) + 1));
            uint8_t input = pkt->readbyte();
            if (input & 1) {
                bv[1] = 1;
            }
            else
                return { 0 };
            size_t inputidx = 1;
            auto nextinput = [&]() -> bool {
                if ((inputidx & 7) == 0)
                    input = pkt->readbyte();
                return input & (1 << (inputidx++ & 7));
            };
            for (size_t idx = 1; idx < cpo2; idx++) {
                auto parent = bv[idx];
                if (!parent) {
                    //    parent=0           => children(00)
                    bv[2 * idx    ] = 0;
                    bv[2 * idx + 1] = 0;
                }
                else {
                    auto i1 = nextinput();
                    if (!i1) {
                        //    parent=1, input=0  => children(10)
                        bv[2 * idx] = 1;
                        bv[2 * idx + 1] = 0;
                    }
                    else {
                        auto i2 = nextinput();
                        if (i2) {
                            //    parent=1, input=11 => children(11)
                            bv[2 * idx] = 1;
                            bv[2 * idx + 1] = 1;
                        }
                        else {
                            //    parent=1, input=10 => children(01)
                            bv[2 * idx] = 0;
                            bv[2 * idx + 1] = 1;
                        }
                    }
                }
            }
            auto bs = bv.bytes();
            return vector<uint8_t>(bs.begin() + (cpo2 / 8), bs.end());
        }
    }

    static vector<uint8_t> bfhcomp(uint8_t* bs, int nbits) {
        vector<uint8_t> result;
        size_t cpo2 = 1;
        for (; cpo2 < nbits; cpo2 *= 2);
        if (cpo2 <= 8) {
            result.push_back(bs[0]);
            return result;
        }
        else {
            // Simple binary tree compression
            //    parent=1, input=11 => children(11)
            //    parent=1, input=10 => children(01)
            //    parent=1, input=0  => children(10)
            //    parent=0           => children(00)
            core::bitvector bv;
            auto treebs = bits2htree(bs, nbits);
            if (treebs[1] == 0) {
                result.push_back(0);
                return result;
            }
            bv.append(1);
            auto tbi = [&](size_t idx) -> bool { return 0 != (treebs[idx >> 3] & (1 << (idx & 7))); };
            for (size_t idx = 1; idx < cpo2; idx++) {
                if (tbi(idx)) {
                    bool c1 = tbi(2 * idx);
                    bool c2 = tbi(2 * idx + 1);
                    if (!c1) {
                        if (c2) {
                            bv.append(1);
                            bv.append(0);
                        }
                        else
                            assert(false);
                    }
                    else {
                        if (!c2)
                            bv.append(0);
                        else {
                            bv.append(1);
                            bv.append(1);
                        }
                    }
                }
            }
            return bv.bytes();
        }
    }
    std::pair<vector<varinfo>, vector<uint8_t>> entity::decodefieldsbitarray(packet& pkt, unsigned int version) const {
        vector<varinfo> vars;
        int nfields = 0;
        for (auto& field : mTypeInfo->vars) {
            if (field.minversion <= version && version < field.maxversion)
                nfields++;
        }
        if (nfields == 0)
            return std::pair<vector<varinfo>, vector<uint8_t>>{{}, {}};

        size_t cpo2 = 1;
        for (; cpo2 < nfields; cpo2 *= 2);
        vector<uint8_t> flagbs;
        if (cpo2 <= 8)
            flagbs.push_back(pkt.readbyte());
        else {
            flagbs = bfhdecomp(nfields, &pkt);
        }
        int bitidx = 0;
        for (auto& field : mTypeInfo->vars) {
            if (field.minversion <= version && version < field.maxversion) {
                if (flagbs[bitidx >> 3] & (1 << (bitidx & 7)))
                    vars.push_back(field);
                bitidx++;
            }
        }
        return std::pair<vector<varinfo>, vector<uint8_t>>{vars, flagbs};
	}
}
