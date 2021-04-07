#pragma once

#include <core/prerequisites.hpp>
#include <core/math/vector3.hpp>
#include <network/packet.hpp>

namespace s2 {
	class gameevent {
	public:
		uint16_t flags;
		uint32_t expire;
		float scale, scale2;
		uint16_t ent, ent2;
		vector3f pos, angles;
		vector3f pos2, angles2;
		uint16_t effect;
		uint16_t sound;

		gameevent() = default;
		void read(network::packet& pkt);
	};
	class entitysnapshot {
	public:
		entitysnapshot() = default;
		void read(network::packet& pkt);
	};
	class snapshot {
	public:
		uint32_t frame;
		uint32_t prevframe;
		uint32_t time0, time1;
		uint8_t numGameEvents;

		snapshot() = default;

		void read(network::packet& pkt);
		bool nextentity(entitysnapshot& es, int version);
	};
}
