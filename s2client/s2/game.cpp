#include "game.hpp"

#include <s2/typeregistry.hpp>


//'game' class needs to have complete representation of the world, navigation, entities
//
//relevant code:
//	userclient::processserversnapshot
//	userclient::processcmd - LoadWorld
//	references to userclient::resetworld
//	updates of 'special' entity types: Entity_ClientInfo, Entity_GameInfo, Entity_TeamInfo

namespace s2 {
	void game::updategame(entity& e) {
		mGameInfoEntNumber = e.id();
	}
	void game::updateteam(entity& e) {
		TeamInfo ti;
		ti.baseBuildingIndex = e.m_uiBaseBuildingIndex;
		mTeams[e.m_iTeamID] = ti;
	}
	void game::updateclient(entity& e) {
		ClientInfo ci;
        ci.name = e.m_sName;
		ci.clientNumber = e.m_iClientNumber;
		ci.playerEntityIndex = e.m_uiPlayerEntityIndex;
		ci.ping = e.m_unPing;
		mClients[ci.clientNumber] = ci;
		if (e.m_iClientNumber == mLocalClientNumber)
			mLocalEnt = &e;
	}
	void game::newentity(int id, int type) {
		auto r = mEntities.emplace(id, entity(id, type));
		if (r.second == false) {
			if (r.first->second.type() != type) {
				mEntities.erase(r.first);
				mEntities.emplace(id, entity(id, type));
			}
		}
		return;
	}
	const std::shared_ptr<world> game::currentworld()const {
		return mWorld;
	}
	std::shared_ptr<world> game::currentworld() {
		return mWorld;
	}
	void game::resetworld() {
		mGameInfoEntNumber = -1;
		mTeams.clear();
		mClients.clear(); // r u sure?
		mEntities.clear();
	}
	bool game::loadworld(string_view worldname, string_view worldchecksum) {
		mWorld = world::LoadFromFile(core::format("maps/%s_%s.s2z", worldname, worldchecksum));
		resetworld();
		return mWorld != nullptr;
	}
	bool game::readentupdate(packet& pkt, int client) {
		uint16_t head = pkt.readword();
		bool entFromBaseline = head & 1;
		auto entid = head >> 1;
		uint16_t entType = 0;
		if (entFromBaseline) {
			entType = pkt.readword();
			//if (entType)
			//	pkt.readdword(); //?
			if (entType) {
                if (client == -1)
                    client = pkt.readdword();
                
				if (!typeregistry::HasType(entType)) {
					core::warning("Unknown entity type %Xh in snapshot\n", entType);
					pkt.advance(pkt.remaining());
					return false;
				}
				else
					newentity(entid, entType);
			}
			else {
                auto it = mEntities.find(entid);
				core::warning("Entity type was specified as zero for entity #%d.\n", entid);
                // this means entity is dead, remove it...
                if (it != mEntities.end()) {
                    core::info("Deleting entity %d\n", entid);
                    mEntities.erase(it);
                }
                return true;
				//return false;
			}
		}
		else {
			if (entid == 0) {
				core::warning("Skipped entity id 0\n");
				return false;
			}
			auto it = mEntities.find(entid);
			if (it == mEntities.end()) {
				core::warning("Entity id %Xh not found\n", entid);
				core::hexdump(pkt.nextdata(), pkt.remaining());
				pkt.seek(pkt.remaining());
				return false;
			}
			entType = it->second.type();
		}
		auto& ent = mEntities[entid];
		auto& typevec = ent.typevec();
		auto varsnbs = ent.decodefieldsbitarray(pkt);
		auto& vars = varsnbs.first;
		auto& bs = varsnbs.second;
		for (auto& var : vars) {
			assert(!pkt.end());
			//core::info("Reading type %d:%s\n", var.type, var.name);
			switch (var.type) {
			case VarType::Byte:
			{
				uint8_t b = pkt.readbyte();
				ent.updatefield(var.name, b);
			} break;
			case VarType::Short:
			{
				ent.updatefield(var.name, pkt.readword());
			} break;
			case VarType::Int:
			{
				ent.updatefield(var.name, pkt.readdword());
			} break;
			case VarType::Single:
			{
				ent.updatefield(var.name, pkt.readsingle());
			} break;
			case VarType::Qword:
			{
				ent.updatefield(var.name, pkt.readqword());
			} break;
			case VarType::Vector3:
			{
				float x = pkt.readsingle();
				float y = pkt.readsingle();
				float z = pkt.readsingle();
				ent.updatefield(var.name, vector3f(x, y, z));
			} break;
			case VarType::String:
			{
				auto str = pkt.readstring();
				ent.updatefield(var.name, str);
			} break;
			case VarType::WordEntityIndex:
			case VarType::WordHandle:
			case VarType::WordAngle:
			{
				ent.updatefield(var.name, pkt.readword());
			} break;
			case VarType::WordFloat:
			{
				uint16_t w = pkt.readword();
				ent.updatefield(var.name, float(w));
			} break;
			case VarType::ByteFloat:
			{
				uint8_t b = pkt.readbyte();
				ent.updatefield(var.name, float(b) / 255.f);
			} break;
			case VarType::WordVector3:
			{
				float x = float(pkt.readword());
				float y = float(pkt.readword());
				float z = float(pkt.readword());
				ent.updatefield(var.name, vector3f(x, y, z));
			} break;
			default:
				core::error("Unknown var type %Xh for var %s\n", var.type, var.name);
			}
		}
		if (ent.typname().compare("Entity_ClientInfo") == 0) {
			updateclient(ent);
			//if (ent.m_iClientNumber == mLocalClientNumber) {
			//	mLocalEntityNumber = ent.m_uiPlayerEntityIndex;
			//	mLocalClientInfoEntNumber = ent.id();
			//}
		}
		else if (ent.typname().compare("Entity_GameInfo") == 0) {
			//mGameInfoEntNumber = ent.id();
			updategame(ent);
		}
		else if (ent.typname().compare("Entity_TeamInfo") == 0) {
			//mTeamInfoEnts[ent.m_iTeamID] = ent.id();
			updateteam(ent);
		}
		return true;
	}
	void game::setclientnumber(int clientNumber) {
		mLocalClientNumber = clientNumber;
	}
	const map<int, entity>& game::entities() const {
		return mEntities;
	}
	vector<entity*> game::entsoftype(string_view typ) {
		vector<entity*> ents;

		for (auto& e : mEntities)
			if (e.second.typname().starts_with(typ))
				ents.push_back(&e.second);

		return ents;
	}
	entity* game::getent(int id) {
		auto it = mEntities.find(id);
		if (it != mEntities.end())
			return &((*it).second);
		else
			return nullptr;
	}
	const entity* game::getent(int id) const {
		auto it = mEntities.find(id);
		if (it != mEntities.end())
			return &((*it).second);
		else
			return nullptr;
	}
	entity* game::localent() {
		auto ci = clientinfo();
		if (!ci.playerEntityIndex)
			return nullptr;
		else
			return getent(ci.playerEntityIndex);
	}
	const GameInfo game::gameinfo() const {
		return mGameInfo;
	}
    const ClientInfo game::clientinfo()const {
		auto it = mClients.find(mLocalClientNumber);
		if (it != mClients.end())
            return it->second;
		else
			return {};
	}
    const std::optional<ClientInfo> game::clientinfo(int clientNum)const {
        auto it = mClients.find(clientNum);
        if (it != mClients.end())
            return it->second;
        else
            return {};
    }
	const TeamInfo game::teaminfo(int teamid) const {
		auto it = mTeams.find(teamid);
		if (it != mTeams.end())
			return it->second;
		else
			return {};
	}
	ServerSnapshotHdr game::rcvserversnapshot(packet& pkt, size_t length, uint8_t stateSeq, int client) {
		ServerSnapshotHdr hdr;

		if (length == 0)
			length = pkt.remaining();
		size_t p0 = pkt.tell();
		hdr.frameId = pkt.readdword();
		hdr.prevFrameId = pkt.readdword();
		hdr.timestamp = pkt.readdword();
		hdr.lastReceivedClientTimestamp = pkt.readdword();

		hdr.stateStringSequence = pkt.readbyte();
        if (hdr.stateStringSequence != stateSeq) {
            core::info("Dropping desync'd snapshot.. (S:%X != C:%X)\n", hdr.stateStringSequence, stateSeq);
            return hdr;
        }
		uint8_t numGameEvents = pkt.readbyte();
		if (numGameEvents != 0) {
			core::print("numGameEvents #%d\n", numGameEvents);
			hdr.events.resize(numGameEvents);
		}
		for (int i = 0; i < numGameEvents; i++) {
			GameEvent ev;
			ev.flags = pkt.readword();
			core::print("GameEvents #%d: %Xh; ", i, ev.flags);
			if (ev.has.Expire) {
				ev.expire = pkt.readdword();
				core::print("expire:%d; ", ev.expire);
			}
			if (ev.has.Ent) {
				ev.ent = pkt.readword();
				core::print("ent:%Xh; ", ev.ent);
			}
			if (ev.has.Pos) {
				ev.pos = vector3f((float)pkt.readword(), (float)pkt.readword(), (float)pkt.readword());
				core::print("pos:%s; ", ev.pos.str());
			}
			if (ev.has.Angles) {
				uint8_t yaw, pitch, roll;
				yaw = pkt.readbyte(); pitch = pkt.readbyte(); roll = pkt.readbyte();
				ev.angles = vector3f(float(yaw), float(pitch), float(roll));
				core::print("angles:%s; ", ev.angles.str());
			}
			if (ev.has.Scale) {
				ev.scale = pkt.readsingle();
				core::print("scale:%.2f; ", ev.scale);
			}
			if (ev.has.Ent2) {
				ev.ent2 = pkt.readword();
				core::print("ent2:%Xh; ", ev.ent2);
			}
			if (ev.has.Pos2) {
				ev.pos2 = vector3f((float)pkt.readword(), (float)pkt.readword(), (float)pkt.readword());
				core::print("pos2:%s; ", ev.pos2.str());
			}
			if (ev.has.Angles2) {
				uint8_t yaw, pitch, roll;
				yaw = pkt.readbyte(); pitch = pkt.readbyte(); roll = pkt.readbyte();
				ev.angles2 = vector3f((float(yaw) / 255.0f) * 360.f,
					(float(pitch) / 255.0f) * 360.f, (float(roll) / 255.0f) * 360.f);
				core::print("angles2:%s; ", ev.angles2.str());
			}
			if (ev.has.Scale2) {
				ev.scale2 = pkt.readsingle();
				core::print("scale2:%.2f; ", ev.scale2);
			}
			if (ev.has.Effect) {
				ev.effect = pkt.readword();
				core::print("effect:%Xh; ", ev.effect);
			}
			if (ev.has.Sound) {
				ev.sound = pkt.readword();
				core::print("sound:%Xh; ", ev.sound);
			}
			core::print("\n");
		}

		while ((pkt.tell() - p0) < length) {
			readentupdate(pkt, client);
		}

		return hdr;
	}
};
