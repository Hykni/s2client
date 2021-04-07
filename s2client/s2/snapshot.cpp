#include "snapshot.hpp"

namespace s2 {
	void gameevent::read(network::packet& pkt) {
		flags = pkt.readword();
		
		if(flags&1)
			expire = pkt.readdword();
		if (flags & 2)
			ent = pkt.readword();
		if(flags&4)
			pos = vector3f((float)pkt.readword(), (float)pkt.readword(), (float)pkt.readword());
		if (flags & 8) {
			uint8_t yaw, pitch, roll;
			yaw = pkt.readbyte(); pitch = pkt.readbyte(); roll = pkt.readbyte();
			angles = vector3f(float(yaw), float(pitch), float(roll));
		}
		if (flags & 0x10) {
			scale = pkt.readsingle();
		}
		if (flags & 0x20) {
			ent2 = pkt.readword();
		}
		if (flags & 0x40) {
			pos2 = vector3f((float)pkt.readword(), (float)pkt.readword(), (float)pkt.readword());
		}
		if (flags & 0x80) {
			uint8_t yaw, pitch, roll;
			yaw = pkt.readbyte(); pitch = pkt.readbyte(); roll = pkt.readbyte();
			angles2 = vector3f((float(yaw) / 255.0f) * 360.f,
				(float(pitch) / 255.0f) * 360.f, (float(roll) / 255.0f) * 360.f);
		}
		if (flags & 0x100) {
			scale2 = pkt.readsingle();
		}
		if (flags & 0x200) {
			effect = pkt.readword();
		}
		if (flags & 0x400) {
			sound = pkt.readword();
		}
	}

	void entitysnapshot::read(network::packet& pkt) {
		uint16_t head = pkt.readword();
		auto entChangeType = head & 1;
		auto entid = head >> 1;
		uint16_t entType;
		if (entChangeType) {
			entType = pkt.readword();
			if (entType) {

			}
		}
	}

	void snapshot::read(network::packet& pkt) {
		frame = pkt.readdword();
		prevframe = pkt.readdword();
		time1 = pkt.readdword();
		time0 = pkt.readdword();
		numGameEvents = pkt.readbyte();
	}
	bool snapshot::nextentity(entitysnapshot& es, int version) {
		
		return false;
	}
}
