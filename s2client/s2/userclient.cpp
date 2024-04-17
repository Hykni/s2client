#include "userclient.hpp"

#include <s2/world.hpp>
#include <s2/consts.hpp>
#include <s2/netids.hpp>
#include <s2/typeregistry.hpp>

#include <core/io/logger.hpp>
#include <core/utils/random.hpp>
#include <core/math/vector3.hpp>
#include <ext/miniz/miniz.h>

#include <sstream>
using std::ostringstream;

template<class A, class B>
constexpr auto MsDuration(const std::chrono::duration<A, B>& _Dur) {
	return std::chrono::duration_cast<std::chrono::milliseconds, A, B>(_Dur);
}

namespace s2 {
	void userclient::reset() {
		mServerFps = 20;
		mState = Disconnected;
		mConnected = false;
		mLocalClientNumber = -1;
        m_yStateStringSequence = 0;
		resetworld();
	}
	void userclient::resetworld() {
		mGame.resetworld();
		mIngame = false;
		mCurrentFrame = ~0;
		mSvState.clear();
		mStateFragments.clear();
		mSnapshotFragments.clear();
		resetlocalent();
	}
	void userclient::resetlocalent() {
		mWaypoints.clear();
		mClientState.clearinput();
	}
	userclient::userclient(uint32_t accountid)
		: mNet(nullptr), mAccountId(accountid) {
		reset();
	}

	const std::shared_ptr<world> userclient::currentworld() const {
		return mGame.currentworld();
	}

	game& userclient::game() {
		return mGame;
	}

	clientstate& userclient::state() {
		return mClientState;
	}

	bool userclient::connected() const {
		return mConnected;
	}

	bool userclient::ingame() const {
		return mIngame;
	}

	uint32_t userclient::servertime() const {
		auto mselapsed = static_cast<uint32_t>(MsDuration(std::chrono::high_resolution_clock::now() - mLastServerFrameTimestamp).count());
		return mLastServerTimestamp + mselapsed;
	}

	uint64_t userclient::sentsnapshots() const {
		return mSentSnapshots;
	}

	uint64_t userclient::recvdsnapshots() const {
		return mRecvdSnapshots;
	}

	string_view userclient::cvar(string_view key) {
		return mCvars[key.data()];
	}
	void userclient::cvar(string_view key, string_view value) {
		mCvars[key.data()] = value;
	}

	bool userclient::connect(string_view ip, int port, string_view password) {
		reset();
        mHostname = ip;
        mPort = port;
		mNet = std::make_unique<netclient>(ip.data(), port);

		packet pkt;
		pkt.writestring(consts::ConnectMagic);
		pkt.writestring(consts::ClientVersion);
		pkt.writebyte(consts::ProtocolVersion);
		pkt.writestring(password);
		pkt.writeword(mNet->clientid());
		pkt.writedword(mAccountId);
		pkt.writestring(""); // ?

		int nbsent = mNet->sendunreliable(ClientCmd::Connect, std::move(pkt));
		if (nbsent > 0) {
			for (int i = 0; i < 10; i++) {
				if (update() > 0) {
					//mState = Connecting;
					//mConnected = true;
					break;
				}
				Sleep(100);
			}
		}
		else {
			mConnected = false;
			mState = Disconnected;
		}
		return mConnected;
	}

	void userclient::disconnect(string_view reason) {
		packet pkt;
		pkt.writestring(reason);
		mNet->sendreliable(ClientCmd::Disconnect, std::move(pkt));
		reset();
		mState = Disconnected;
		mConnected = false;
	}

	int userclient::update() {
		auto now = std::chrono::high_resolution_clock::now();
		netmsg m;
		int count = 0;
		if (mNet->readmsg(&m)) {
			assert(m.data().length() > 0);
			while (!m.data().end()) {
				auto cmdid = m.data().readbyte();
				processcmd(cmdid, m.data());
				count++;
			}
		}
		if (mIngame) {
			auto mselapsed = MsDuration(now - mLastClientSnapshot).count();
			auto cli = clientinfo();
			int ping = cli.ping;
			if (mselapsed >= (1000 / mPacketSendFps)) {
				sendclientsnapshot(ping);
			}
		}

		think();
		return count;
	}

	void userclient::think() {
		if (mCurrentFrame == mLastThinkFrame)
			return;
		mLastThinkFrame = mCurrentFrame;
		switch (mState) {
        case WaitingFirstFrame:
        {

        } break;
		case Spectating:
		{
			if (ingame() && clientinfo().ping) {
				teamrequest(2);
				teamrequest(1);
				mState = Spawning;
				Sleep(100);
			}
		} break;
		case Spawning:
		{
			auto info = clientinfo();
			auto local = getent(mGame.clientinfo().playerEntityIndex);
			if (!local || local->dormant()) {
				if (local && local->m_iTeam == 1)
					unitrequest(705);
				else
					unitrequest(704);
			}

			if (local) {
				if (local->alive()) {
					mState = Playing;
				}
				else {
					auto team = teaminfo(local->m_iTeam);
					if (team.baseBuildingIndex) {
						preparespawn();
						spawnrequest(team.baseBuildingIndex);
					}
				}
			}
		} break;
		case Playing:
		{
			/*if ((core::random::uint32() & 0xFF) < 10) {
				auto plyr = entfromtype("Player_");
				pingminimap(plyr->m_v3Position.x, plyr->m_v3Position.y);
			}*/

			auto local = getent(mGame.clientinfo().playerEntityIndex);
			if (local) {
				if (local->dormant()) {
					mState = Spectating;
					mWaypoints.clear();
					return;
				}
				auto pos = local->m_v3Position;
				if (pos.lengthsq() < 1.f) {
					return;
				}
				while (mWaypoints.size() > 1) {
					auto next = mWaypoints.front();
					auto n2 = mWaypoints.at(1);
					auto move = n2 - next;
					auto movelen = move.length();
					auto fromnext = pos - next;
					//fromnext.z = 0.f;
					if (fromnext.dot(mCurrentPathingDir) >= -5.f) {// || fromnext.lengthsq() < (50.f*50.f)) {
						//core::info("fromnext %.2f,%.2f,%.2f\n", fromnext.x, fromnext.y, fromnext.z);
						mWaypoints.pop_front();
						mCurrentPathingDir = (mWaypoints.front() - next).unit();
						if (!mWaypoints.empty()) {
							auto fromnext2 = pos - mWaypoints.front();
							//core::info("fromnext2 %.2f,%.2f,%.2f\n", fromnext2.x, fromnext2.y, fromnext2.z);
						}
					}
					else
						break;
				}
				if (!mWaypoints.empty()) {
					auto dst = mWaypoints.back();
					float distance;
					if (dst.z == 0.f)
						distance = (dst - vector3f(pos.x, pos.y, 0.f)).length();
					else
						distance = (dst - pos).length();
					if(distance < mTargetSize) {
						auto prev = mWaypoints.front();
						mWaypoints.pop_front();
						if(mWaypoints.empty())
							mClientState.clearinput();
						else
							mCurrentPathingDir = (mWaypoints.front() - prev).unit();
					}
					else {
						movetowards(mWaypoints.front());
					}
				}
			}
		} break;
		}
	}

	entity* userclient::localent() {
		if (!ingame())
			return nullptr;
		return mGame.localent();
	}

	const entity* userclient::getent(int id) const {
		return mGame.getent(id);
	}

	const ClientInfo userclient::clientinfo()const {
		if (!connected())
            return {};
		return mGame.clientinfo();
	}

	const GameInfo userclient::gameinfo()const {
		if (!connected())
            return {};
		return mGame.gameinfo();
	}
	const TeamInfo userclient::teaminfo(int id)const {
		return mGame.teaminfo(id);
	}

	void userclient::movetowards(const vector3f& target) {
		if (!ingame())
			return;
		auto local = mGame.getent(mGame.clientinfo().playerEntityIndex);
		if (local) {
			auto pos = local->m_v3Position;
			auto delta = target - pos;
			mClientState.yaw = 180.f * atan2f(-delta.x, delta.y) / M_PI;
			mClientState.pitch = 180.f * atan2f(delta.z, sqrtf(delta.x * delta.x + delta.y * delta.y)) / M_PI;
			mClientState.setinput(ClientInput::Forward);
		}
	}

	void userclient::pathtowards(const vector3f& target, float targetSize) {
		if (!ingame())
			return;
		auto local = getent(mGame.clientinfo().playerEntityIndex);
		if (local) {
			auto pos = local->m_v3Position;
			mWaypoints = mGame.currentworld()->pathfind(pos, target, [=](const vector3f& from, const vector3f& to) -> bool {
				return (to - from).lengthsq() < (targetSize * targetSize);
			});
			if (!mWaypoints.empty()) {
				mCurrentPathingDir = (mWaypoints.front() - pos).unit();
				movetowards(mWaypoints.front());
				mTargetSize = targetSize;
			}
		}
	}

	bool userclient::haspath() const {
		return !mWaypoints.empty();
	}

	const deque<vector3f>& userclient::path() const {
		return mWaypoints;
	}

	//const map<int, entity>& userclient::entities() const {
	//	return mEntities;
	//}

	//std::optional<entity> userclient::entfromid(int id) const {
	//	auto it = mEntities.find(id);
	//	if (it == mEntities.end())
	//		return std::nullopt;
	//	else
	//		return std::make_optional<entity>(it->second);
	//}

	//void userclient::newentity(int eid, int etype) {
	//	auto r = mEntities.emplace(eid, entity(eid, etype));
	//	if (r.second == false) {
	//		if (r.first->second.type() != etype) {
	//			mEntities.erase(r.first);
	//			mEntities.emplace(eid, entity(eid, etype));
	//		}
	//	}
	//	return;
	//}

	void userclient::sendclientsnapshot(int ping) {
		if (mCurrentFrame == ~0)
			return;

		auto svrtime = servertime();
		auto mselapsed = static_cast<uint32_t>(MsDuration(std::chrono::high_resolution_clock::now() - mLastServerFrameTimestamp).count());
		// if u send for a frame that u haven't received yet, the server won't send u that frame update
		int frameDelta = 0;// (svrtime - mLastServerTimestamp) / (1000 / mServerFps);
		
		packet pkt;
		pkt.writedword(mCurrentFrame + frameDelta);
		pkt.writedword(svrtime);// -ping);
		pkt.writebyte(mClientState.weaponidx);
		pkt.writeword(mClientState.input);
		pkt.writeword(0); //??
		pkt.writebyte(5); // pitch & yaw
		pkt.writesingle(mClientState.pitch);
		pkt.writesingle(mClientState.yaw);
		mNet->sendunreliable(ClientCmd::Snapshot, std::move(pkt));
		mLastClientSnapshot = std::chrono::high_resolution_clock::now();
		//core::info("Sent client snapshot for frame #%d (ts %d)\n", mCurrentFrame + frameDelta, svrtime);
		mSentSnapshots++;
	}

	void userclient::sendcvars() {
		ostringstream ss;
		for (auto d : mCvars) {
			ss << d.first;
			ss.write("\xFF", 1);
			ss << d.second;
			ss.write("\xFF", 1);
		}
		packet pkt;
		uint32_t length = (uint32_t)ss.tellp();
		pkt.writedword(length);
		pkt.write((uint8_t*)ss.str().data(), length);
		pkt.writebyte(0xC2);
		mNet->sendreliable(ClientCmd::Vars, std::move(pkt));
	}
	void userclient::requeststatestrings() {
		mNet->sendreliable(ClientCmd::RequestStateStrings);
	}

	void userclient::sendclientready() {
		mIngame = false;
		mState = Ready;
		mNet->sendreliable(ClientCmd::Ready);
	}

	void userclient::sendclientjoin() {
		//mIngame = true;
		mState = WaitingFirstFrame;
		core::info("Sending client join.\n");
		mNet->sendreliable(ClientCmd::Join);
	}

	void userclient::senddownloadworld() {
		mIngame = false;
		mState = LoadingWorld;
		core::info("Sending download world request.\n");
		mNet->sendreliable(ClientCmd::DownloadWorld);
	}

	void userclient::sendgamedata(uint8_t id) {
		packet pkt;
		pkt.writebyte(id);
		mNet->sendreliable(ClientCmd::Gamedata, std::move(pkt));
	}
	void userclient::sendgamedata(uint8_t id, packet&& data) {
		packet pkt;
		pkt.writebyte(id);
		pkt.write(data.data(), data.length());
		mNet->sendreliable(ClientCmd::Gamedata, std::move(pkt));
	}

	void userclient::teamrequest(uint16_t id) {
		packet p; p.writeword(id);
		sendgamedata(Gamedata::RequestTeam, std::move(p));
	}

	void userclient::unitrequest(uint16_t id) {
		packet p; p.writeword(id);
		sendgamedata(Gamedata::RequestUnit, std::move(p));
	}

	void userclient::preparespawn() {
		sendgamedata(Gamedata::Spawn);
	}

	void userclient::spawnrequest(uint32_t id) {
		packet p; p.writedword(id);
		sendgamedata(Gamedata::RequestSpawn, std::move(p));
	}

	void userclient::pingminimap(float x, float y) {
		packet p;
		p.writesingle(x / mGame.currentworld()->worldsize());
		p.writesingle(y / mGame.currentworld()->worldsize());
		p.writebyte(0xFF);
		sendgamedata(Gamedata::MinimapPing, std::move(p));
	}

	void userclient::drawminimap(float x, float y) {
		packet p;
		p.writesingle(x / mGame.currentworld()->worldsize());
		p.writesingle(y / mGame.currentworld()->worldsize());
		p.writebyte(0xFF);
		sendgamedata(Gamedata::MinimapDraw, std::move(p));
	}

	void userclient::updatestatestrings(int sid, const char* ssdata, size_t length) {
		auto& varset = mSvState[sid];
		enum class states { read_key, read_val } state = states::read_key;
		string key, val;
		int updatecount = 0;
		for (size_t idx = 0; idx < length; idx++) {
			char c = ssdata[idx];
			if (c == '\xFF') {
				if (state == states::read_val) {
					varset[key] = val;
					key.clear();
					val.clear();
					state = states::read_key;
					updatecount++;
				}
				else
					state = states::read_val;
			}
			else if (state == states::read_key) {
				key.push_back(c);
			}
			else if (state == states::read_val) {
				val.push_back(c);
			}
		}
		core::info("Updated %d state strings.\n", updatecount);
	}

	bool userclient::processserversnapshot(packet& pkt, size_t length) {
        if (mState == WaitingFirstFrame) {
            mIngame = true;
            mState = Spectating;
        }
		mRecvdSnapshots++;
		auto hdr = mGame.rcvserversnapshot(pkt, length, m_yStateStringSequence, mLocalClientNumber);
		mCurrentFrame = hdr.frameId;
		return true;
	}

	bool userclient::processcmd(uint8_t cmdid, packet& pkt) {
		switch (cmdid) {
		case ServerCmd::KickClient:
		{
			auto reason = pkt.readstring();
			core::warning("Server kicked us: %s\n", reason);
			reset();
			mConnected = false;
		} break;
		case ServerCmd::ShuttingDown:
		{
			core::warning("Server shutting down.\n");
			reset();
			mConnected = false;
		} break;
		case ServerCmd::RequestVars:
		{
			mLocalClientNumber = pkt.readdword();
			mGame.setclientnumber(mLocalClientNumber);
			mState = Connecting;
			mConnected = true;
			sendcvars();
			requeststatestrings();
		} break;
		case ServerCmd::DenyConnect:
		{
			string reason = string((const char*)pkt.data()+1, pkt.remaining()-1);
			core::warning("Server denied connection: %s\n", reason);
			reset();
			mConnected = false;
			pkt.advance(pkt.remaining());
		} break;
		case ServerCmd::StateReset:
		{
			core::info("Clearing server state strings.\n");
            m_yStateStringSequence = 0;
			mSvState.clear();
		} break;
		case ServerCmd::StateUpdate:
		{
			uint16_t stateid = pkt.readword();
			uint32_t statelen = pkt.readdword();
			updatestatestrings(stateid, (const char*)pkt.nextdata(), statelen);
			pkt.advance(statelen);
            m_yStateStringSequence++;
            core::info("m_yStateStringSequence %d\n", m_yStateStringSequence);
		} break;
		case ServerCmd::CompressedStateUpdate:
		{
			uint16_t stateid = pkt.readword();
			uint32_t statelen = pkt.readdword();
			mz_ulong decomplen = pkt.readdword();
			string stateupdate;
			stateupdate.resize(decomplen);
			auto mzresult = mz_uncompress((unsigned char*)stateupdate.data(), &decomplen, pkt.nextdata(), statelen);
			pkt.advance(statelen);
			if (mzresult == MZ_OK) {
				if (decomplen != stateupdate.size())
					core::warning("Decompressed length didn't match specified %Xh != %Xh", stateupdate.size(), decomplen);
				updatestatestrings(stateid, stateupdate.data(), decomplen);
			}
			else
				core::error("Compressed state string update failed.\n");
            m_yStateStringSequence++;
            core::info("m_yStateStringSequence %d\n", m_yStateStringSequence);
		} break;
		case ServerCmd::StateFragment:
		{
			uint16_t stateid = pkt.readword();
			uint16_t statelen = pkt.readword();
			mStateFragments[stateid].append((const char*)pkt.nextdata(), statelen);
			pkt.advance(statelen);
		} break;
		case ServerCmd::StateTerminate:
		{
			core::info("Received final state strings packet.\n");
			uint16_t stateid = pkt.readword();
			uint16_t statelen = pkt.readword();
			mStateFragments[stateid].append((const char*)pkt.nextdata(), statelen);
			pkt.advance(statelen);
			updatestatestrings(stateid, mStateFragments[stateid].data(), mStateFragments[stateid].length());
			mStateFragments.erase(stateid);
            m_yStateStringSequence++;
            core::info("m_yStateStringSequence %d\n", m_yStateStringSequence);
		} break;
		case ServerCmd::CompressedStateTerminate:
		{
			core::info("Received final compressed state strings packet.\n");
			uint16_t stateid = pkt.readword();
			uint16_t statelen = pkt.readword();
			mz_ulong decomplen = pkt.readdword();
			string stateupdate;
			stateupdate.resize(decomplen);
			string& ssdata = mStateFragments[stateid];
			ssdata.append((const char*)pkt.nextdata(), statelen);
			auto mzresult = mz_uncompress((unsigned char*)stateupdate.data(), &decomplen, (unsigned char*)ssdata.data(), static_cast<mz_ulong>(ssdata.size()));
			pkt.advance(statelen);
			if (mzresult == MZ_OK) {
				if (decomplen != stateupdate.size())
					core::warning("Decompressed length didn't match specified %Xh != %Xh", stateupdate.size(), decomplen);
				updatestatestrings(stateid, stateupdate.data(), decomplen);
			}
			else
				core::warning("Compressed state string update failed.\n");
			mStateFragments.erase(stateid);
            m_yStateStringSequence++;
            core::info("m_yStateStringSequence %d\n", m_yStateStringSequence);
		} break;
		case ServerCmd::StateStringsEnd:
		{
			core::info("svar[svr_clientConnectedTimeout] = %s\n", mSvState[1]["svr_clientConnectedTimeout"]);
			core::info("svar[svr_clientConnectingTimeout] = %s\n", mSvState[1]["svr_clientConnectingTimeout"]);
			core::info("svar[svr_gameFPS] = %s\n", mSvState[1]["svr_gameFPS"]);
			mServerFps = std::stol(mSvState[1]["svr_gameFPS"]);
			core::info("Received end of state strings. Sending client ready...\n");
			sendclientready();
		} break;
		case ServerCmd::LoadWorld:
		{
			mWorldName = pkt.readstring();
			mWorldChecksum = pkt.readstring();
			core::info("Received world load request for \"%s\" (%s)\n", mWorldName, mWorldChecksum);
			auto worldpath = core::format("maps/%s_%s.s2z", mWorldName, mWorldChecksum);
			auto success = mGame.loadworld(mWorldName, mWorldChecksum);
			resetworld();
			if (success) {
				sendclientjoin();
			}
			else {
				senddownloadworld();
			}
		} break;
		case ServerCmd::DownloadStart:
		{
			mWorldDownloadLength = pkt.readdword();
			core::info("Received world download start %d KB\n", mWorldDownloadLength / 1024);
			mWorldDownload.clear();
			mWorldDownload.reserve(mWorldDownloadLength);
		} break;
		case ServerCmd::DownloadWorld:
		{
			auto len = pkt.readword();
			mWorldDownload.insert(mWorldDownload.end(), pkt.nextdata(), pkt.nextdata() + len);
			pkt.advance(len);
			core::info("[%d/%d] Received %d bytes of world download\n", mWorldDownload.size(), mWorldDownloadLength, len);
		} break;
		case ServerCmd::DownloadFinished:
		{
			core::info("World download finished.\n");
			auto worldfilename = core::format("maps/%s_%s.s2z", mWorldName, mWorldChecksum);
			FILE* f = fopen(worldfilename.c_str(), "wb");
			if (!f) {
				if(0 == _mkdir("maps"))
					f = fopen(worldfilename.c_str(), "wb");
			}
			if (f) {
				fwrite(mWorldDownload.data(), mWorldDownload.size(), 1, f);
				fclose(f);
				core::info("Wrote world data to %s\n", worldfilename);
			}
			else {
				core::error("Couldn't open world file %s\n", worldfilename);
			}
			mWorldDownload.clear();
			
			auto success = mGame.loadworld(mWorldName, mWorldChecksum);
			if (success) {
				sendclientjoin();
			}
			else {
				core::error("Failed to load downloaded world from %s", worldfilename);
			}
		} break;
		case ServerCmd::ClientAuthenticated:
		{
			core::info("Client authenticated.\n");
		} break;
		case ServerCmd::NewVoiceClient:
		{
			auto a = pkt.readdword();
			auto b = pkt.readbyte();
			core::info("New voice user joined %X %X\n", a, b);
		} break;
		case ServerCmd::UpdateVoiceClient:
		{
			auto x = pkt.readbyte();
			auto y = pkt.readword();
			uint8_t numclients = pkt.readbyte();
			core::info("Received vc update. Num clients = %d\n", numclients);
			for (uint8_t i = 0; i < numclients; i++) {
				auto ida = pkt.readdword();
				auto idb = pkt.readbyte();
				core::info(" Updated vc client #%d: %X %X\n", i, ida, idb);
			}
		} break;
		case ServerCmd::RemoveVoiceClient:
		{
			auto uid = pkt.readbyte();
			core::info("Voice user %X left.\n", uid);
		} break;
		case ServerCmd::Gamedata:
		{
			auto gdid = pkt.readbyte();
			processgamedata(gdid, pkt);
		} break;
		case ServerCmd::Snapshot:
		{
			auto snapshotlen = pkt.readdword();
			size_t p0 = pkt.tell();
			packet copy;
			copy.write(pkt.nextdata(), snapshotlen);
			processserversnapshot(copy);
			pkt.advance(snapshotlen);
			if ((pkt.tell() - p0) != snapshotlen) {
				core::error("Advanced %d bytes (specified snapshot length was %d bytes)\n", pkt.tell() - p0, snapshotlen);
			}
			assert((pkt.tell() - p0) == snapshotlen);
		} break;
		case ServerCmd::CompressedSnapshot:
		{
			mz_ulong snapshotlen = pkt.readdword();
			mz_ulong decomplen = pkt.readdword();
			size_t p0 = pkt.tell();
			string snapshot;
			snapshot.resize(decomplen);
			auto mzresult = mz_uncompress((unsigned char*)snapshot.data(), &decomplen, (unsigned char*)pkt.nextdata(), snapshotlen);
			pkt.advance(snapshotlen);
			if (mzresult == MZ_OK) {
				if (decomplen != snapshot.size())
					core::warning("Decompressed snapshot length didn't match specified %Xh != %Xh", snapshot.size(), decomplen);
				auto snapdata = packet(std::move(snapshot));
				processserversnapshot(snapdata, decomplen);
			}
			else
				core::error("Compressed state string update failed.\n");
			assert((pkt.tell() - p0) == snapshotlen);
		} break;
		case ServerCmd::SnapshotFragment:
		{
			uint32_t frame = pkt.readdword();
			uint8_t snapid = pkt.readbyte();
			mSnapshotFragments[frame].write(pkt.nextdata(), pkt.remaining());
			pkt.advance(static_cast<int>(pkt.remaining()));
		} break;
		case ServerCmd::SnapshotTerminate:
		{
			uint32_t frame = pkt.readdword();
			uint8_t snapid = pkt.readbyte();
			uint16_t datalen = pkt.readword();
			if (datalen > 0) {
				mSnapshotFragments[frame].write(pkt.nextdata(), datalen);
				pkt.advance(datalen);
			}
			if (datalen == 0 && mSnapshotFragments.find(frame) == mSnapshotFragments.end())
				core::warning("Received empty snapshot termination for frame not in fragment list #%d", frame);
			else {
				processserversnapshot(mSnapshotFragments[frame]);
				mSnapshotFragments.erase(frame);
			}
		} break;
		case ServerCmd::CompressedSnapshotTerminate:
			__debugbreak();
		default:
			core::warning("Unhandled command id %Xh\n", cmdid);
			core::hexdump(pkt.data(), pkt.remaining());
			pkt.seek(pkt.length());
			return false;
		}
		return true;
	}
	bool userclient::processgamedata(uint8_t id, packet& pkt) {
		switch (id) {
		case Gamedata::ChatAll:
		{
			auto cid = pkt.readdword();
			auto msg = pkt.readstring();
			core::print<core::CON_BLU>("[ALL] ");
			core::print("%s\n", msg);
		} break;
		case Gamedata::ChatTeam:
		{
			auto cid = pkt.readdword();
			auto msg = pkt.readstring();
			core::print<core::CON_YLW>("[TEAM] ");
			core::print("%s\n", msg);
		} break;
		case Gamedata::ChatSquad:
		{
			auto cid = pkt.readdword();
			auto msg = pkt.readstring();
			core::print<core::CON_CYN>("[SQUAD] ");
			core::print("%s\n", msg);
		} break;
		case Gamedata::ServerMessage:
		{
			auto msg = pkt.readstring();
			core::print<core::CON_CYN>("[SERVER] Server message: ");
			core::print("%s\n", msg);
		} break;
		case Gamedata::Message:
		{
			auto msg = pkt.readstring();
			core::info("%s\n", msg);
		} break;
		case Gamedata::HitFeedback:
		{
			if (pkt.readbyte() == 13) {
				pkt.readsingle(); pkt.readsingle(); pkt.readsingle();
			}
			else {
				pkt.readword();
			}
		} break;
		case Gamedata::MinimapDraw:
		{
			float x = pkt.readsingle();
			float y = pkt.readsingle();
			core::info("Received minimap draw @ %.2f, %.2f\n", x, y);
		} break;
		case Gamedata::MinimapPing:
		{
			float x = pkt.readsingle();
			float y = pkt.readsingle();
			core::info("Received minimap ping @ %.2f, %.2f\n", x, y);
		} break;
		case Gamedata::PermanentItems:
		{
			for (int i = 0; i < 5; i++) {
				auto count = pkt.readword();
				auto id = pkt.readdword();
			}
			core::info("Received permanent item list.\n");
		} break;
		case Gamedata::SendMessage:
		{
			auto msg = pkt.readstring();
			core::info("Received server message \"%s\"\n", msg);
		} break;
		case Gamedata::VoiceCommand:
		{
			auto a = pkt.readdword();
			auto team = pkt.readstring();
			auto c = pkt.readdword();
			auto d = pkt.readbyte();
			enum vctype {
				Team = 0,
				Squad = 1,
				Commander = 2,
				All = 3
			};
			core::info("Voice command %d %s %d %d\n", a, team, c, d);
		} break;
		case Gamedata::ConstructionComplete:
		{
			auto id = pkt.readword();
			core::info("Building construction completed %d\n", id);
		} break;
		case Gamedata::StartConstructBuilding:
		{
			auto id = pkt.readword();
			core::info("Building construction started %d\n", id);
		} break;
		case Gamedata::GoldmineLow:
		{
			auto id = pkt.readdword();
			core::info("Goldmine low! %d\n", id);
		} break;
		case Gamedata::BuildingDestroyed:
		{
			auto id = pkt.readword();
			auto wot = pkt.readbyte();
			core::info("Building destroyed %d; %d\n", id, wot);
		} break;
		case Gamedata::Death:
		{
			uint32_t idkiller = pkt.readdword();
			uint32_t idkilled = pkt.readdword();
			uint16_t idweapon = pkt.readword();
			auto killedent = mGame.getent(idkilled);
			if (killedent) {
				killedent->killed = true;
				core::info("%s was killed.\n", killedent->typname());
			}
			core::info("Received death packet id (killer:%d) (killed:%d) (weapon:%d).\n", idkiller, idkilled, idweapon);
		} break;
		case Gamedata::ExecScript:
		{
			auto script = pkt.readstring();
			auto nargs = pkt.readword();
			core::info("Received execscript command \"%s\"; nargs=%d\n", script, nargs);
			for (int i = 0; i < nargs; i++) {
				auto key = pkt.readstring();
				auto value = pkt.readstring();
				core::info("\t\"%s\" = \"%s\"\n", key, value);
			}
            if (script == "playerreconnect") {
                core::info("Reconnecting...\n");
                disconnect("reconnecting...");
                connect(mHostname, mPort);
            }
		} break;
		default:
			core::warning("Unknown gamedata id = %Xh\n", id);
			pkt.seek(pkt.length());
			return false;
		}
		return true;
	}
}
