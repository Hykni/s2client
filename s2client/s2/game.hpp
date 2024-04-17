#pragma once

#include <core/prerequisites.hpp>
#include <s2/entity.hpp>
#include <s2/world.hpp>

namespace s2 {
	struct GameEvent {
		union {
			uint16_t flags;
			struct {
				bool Expire : 1;
				bool Ent : 1;
				bool Pos : 1;
				bool Angles : 1;
				bool Scale : 1;
				bool Ent2 : 1;
				bool Pos2 : 1;
				bool Angles2 : 1;
				bool Scale2 : 1;
				bool Effect : 1;
				bool Sound : 1;
			} has;
		};
		uint32_t expire;
		uint16_t ent;
		vector3f pos;
		vector3f angles;
		float scale;
		uint16_t ent2;
		vector3f pos2;
		vector3f angles2;
		float scale2;
		uint16_t effect;
		uint16_t sound;
	};
	struct ServerSnapshotHdr {
		uint32_t frameId;
		uint32_t prevFrameId;
		uint32_t timestamp;
		uint32_t lastReceivedClientTimestamp;
        uint8_t  stateStringSequence;
		vector<GameEvent> events;
	};

	struct GameInfo {
	};
	struct TeamInfo {
        uint16_t baseBuildingIndex = 0;
	};
	struct ClientInfo {
        string name = "";
        int clientNumber = 0;
		uint16_t playerEntityIndex = 0;
		uint16_t ping = 0;
	};
	class game {
		std::shared_ptr<world> mWorld;
		map<int, entity> mEntities;
		GameInfo mGameInfo;
		int mGameInfoEntNumber = -1;
		map<int, TeamInfo> mTeams;
		int mLocalClientNumber = -1;
		map<int, ClientInfo> mClients;
		entity* mLocalEnt = nullptr;
        uint8_t mStateStringSequence = 0;

		void updategame(entity& e);
		void updateteam(entity& e);
		void updateclient(entity& e);
		void newentity(int id, int type);
	public:
		const std::shared_ptr<world> currentworld()const;
		std::shared_ptr<world> currentworld();
		void resetworld();
		bool loadworld(string_view worldname, string_view worldchecksum);
		bool readentupdate(packet& pkt, int client=-1);
		
		void setclientnumber(int clientNumber);
		const map<int, entity>& entities()const;
		vector<entity*> entsoftype(string_view typ);
		entity* getent(int id);
		const entity* getent(int id)const;
		entity* localent();
		const GameInfo gameinfo()const;
        const ClientInfo clientinfo()const;
        const std::optional<ClientInfo> clientinfo(int clientNum)const;
		const TeamInfo teaminfo(int teamid)const;

		ServerSnapshotHdr rcvserversnapshot(packet& pkt, size_t length, uint8_t stateSeq, int client=-1);
	};
}
