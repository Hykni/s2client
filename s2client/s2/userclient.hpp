#pragma once

#include <core/prerequisites.hpp>
#include <s2/netclient.hpp>
#include <s2/entity.hpp>
#include <s2/world.hpp>
#include <s2/game.hpp>

namespace s2 {

	enum class ClientInput : uint16_t {
		Attack  = 1<<0,
		Dodge   = 1<<1,
		Block   = 1<<2,
		Forward = 1<<3,
		Back    = 1<<4,
		Left	= 1<<5,
		Right	= 1<<6,
		Sprint	= 1<<9
	};
	struct clientstate {
	public:
		uint8_t weaponidx = 0;
		float yaw = 0.0f;
		float pitch = 0.0f;
		uint16_t input = 0;
		inline void setinput(ClientInput inp) { input |= static_cast<uint16_t>(inp); }
		inline void clearinput(ClientInput inp) { input &= ~static_cast<uint16_t>(inp); }
		inline void clearinput() { input = 0; }
	};
	class userclient {
	private:
		std::unique_ptr<netclient> mNet;
		uint32_t mAccountId;
		uint32_t mLastServerTimestamp = ~0;
		std::chrono::steady_clock::time_point mLastServerFrameTimestamp;
		std::chrono::steady_clock::time_point mLastClientSnapshot;
		uint32_t mCurrentFrame = ~0;
		uint32_t mLastThinkFrame = ~0;
		uint32_t mLocalClientNumber = -1;
		uint64_t mSentSnapshots = 0;
		uint64_t mRecvdSnapshots = 0;
		int mGameInfoEntNumber = -1;
		long mServerFps = 20;
		long mPacketSendFps = 30;
		bool mConnected = false;
		bool mIngame = false;

		map<int, int> mTeamInfoEnts;

		enum {
			Disconnected,
			Connecting,
			LoadingWorld,
			Ready,
			Spectating,
			Spawning,
			Playing
		} mState = Disconnected;

		typedef map<string, string> VarSet;
		VarSet mCvars = {
			{ "cl_packetSendFPS", "30" },
			{ "net_FPS", "30" },
			{ "net_cookie", "" },
			//{ "net_maxBPS", "400000" },
			{ "net_maxPacketSize", "65535" },
			//{ "net_reliablePackets", "true" },
			{ "net_maxBPS", "20000" },
			{ "net_name", "noob123" },
			{ "net_sendCvars", "true" }
		};
		map<int, VarSet> mSvState;
		map<int, string> mStateFragments;
		map<uint32_t, packet> mSnapshotFragments;
		string mWorldName;
		string mWorldChecksum;
		vector<uint8_t> mWorldDownload;
		uint32_t mWorldDownloadLength = 0;
		game mGame;

		//map<int, entity> mEntities;
		clientstate mClientState;
		deque<vector3f> mWaypoints;
		vector3f mCurrentPathingDir;
		float mTargetSize;

		void reset();
		void resetworld();
		void resetlocalent();
	public:
		userclient(uint32_t accountid);

		const std::shared_ptr<world> currentworld()const;

		game& game();
		clientstate& state();
		bool connected()const;
		bool ingame()const;
		uint32_t servertime()const;
		uint64_t sentsnapshots()const;
		uint64_t recvdsnapshots()const;

		string_view cvar(string_view key);
		void cvar(string_view key, string_view value);

		bool connect(string_view ip, int port, string_view password="");
		void disconnect(string_view reason);

		int update();
		void think();

		entity* localent();
		const entity* getent(int id)const;
		const ClientInfo* clientinfo()const;
		const GameInfo* gameinfo()const;
		const TeamInfo* teaminfo(int id)const;
		void movetowards(const vector3f& target);
		void pathtowards(const vector3f& target, float targetSize=25.f);
		bool haspath()const;
		const deque<vector3f>& path()const;

		//const map<int, entity>& entities()const;
		//std::optional<entity> entfromid(int id)const;

	private:
		//void newentity(int eid, int etype);

		void sendclientsnapshot(int ping);
		void sendcvars();
		void requeststatestrings();
		void sendclientready();
		void sendclientjoin();
		void senddownloadworld();
		void sendgamedata(uint8_t id);
		void sendgamedata(uint8_t id, packet&& data);
		void teamrequest(uint16_t id);
		void unitrequest(uint16_t id);
		void preparespawn();
		void spawnrequest(uint32_t id);
		void pingminimap(float x, float y);
		void drawminimap(float x, float y);

		void updatestatestrings(int sid, const char* ssdata, size_t length);

		bool processserversnapshot(packet& pkt, size_t length=0);
		bool processcmd(uint8_t cmdid, packet& pkt);
		bool processgamedata(uint8_t id, packet& pkt);
	};
}
