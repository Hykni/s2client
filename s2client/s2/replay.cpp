#include <s2/replay.hpp>
#include <s2/netids.hpp>
#include <network/packet.hpp>
#include <s2/game.hpp>
#include <core/io/logger.hpp>
#include <core/io/bytestream.hpp>
#include <ext/miniz/miniz.h>

namespace s2 {
	replay::replay() : mMap() {
	}

	std::shared_ptr<replay> replay::LoadFromFile(string_view filename) {
        game tmpGame;

		mz_zip_archive archive;
		memset(&archive, 0, sizeof(archive));
		if (mz_zip_reader_init_file(&archive, filename.data(), 0)) {
			size_t datalen;
			uint8_t* replaydata = (uint8_t*)mz_zip_reader_extract_file_to_heap(&archive, "replaydata", &datalen, 0);
			if (!replaydata) {
				mz_zip_reader_end(&archive);
				return nullptr;
			}

			auto fs = core::bytestream(replaydata, datalen);

			auto sig = fs.readDword();
			// 'S2R0'
			if (sig != '0R2S') {
				core::error("Invalid replay file signature %X\n", sig);
				mz_zip_reader_end(&archive);
				return nullptr;
			}

			auto version = fs.readDword();
			if (version > 27)
				core::error("Unsupported replay version %d (> 27)", version);

			struct construct_replay : public s2::replay {};
			std::shared_ptr<replay> out = std::make_shared<construct_replay>();

			out->mMap = fs.readWString();
			core::info("Replay map name %ls\n", out->mMap);

			// read state-strings
			for (int i = 1; i < 4; i++) {
				map<string, string> statestrings;
				// for some reason, replay state-strings are stored as widechars but only used as ascii
				string key, val;
				while (true) {
					key.clear(); val.clear();
					wchar_t c = fs.readWord();
					if (c == 0x00)
						break;
					key += (c & 0xFF);
					while ((c = fs.readWord()) != 0xFFFF)
						key += (c & 0xFF);
					while ((c = fs.readWord()) != 0xFFFF)
						val += (c & 0xFF);
					if(i == 1)
						core::info("   %s=%s\n", key, val);
					statestrings[key] = val;
				}
				out->mState[i] = statestrings;
				core::info("Read %d state strings of type %d\n", statestrings.size(), i);
			}

			core::info("Finished reading state strings @ %Xh\n", fs.tell());
			int snapshotCount = 0;
			while (!fs.eof()) {
				uint32_t snaplen = fs.readDword();
                auto endSnap = fs.tell() + snaplen;

                network::packet pkt;
                uint8_t* snapData = fs.data() + fs.tell();

                pkt.write(snapData, snaplen);
                pkt.seek(0);
                tmpGame.rcvserversnapshot(pkt, pkt.length(), 0);

                auto frameNo = fs.readDword();
                auto prevFrameNo = fs.readDword();
                auto timestamp = fs.readDword();
                auto lastClientTime = fs.readDword();
                auto stateStringSeq = fs.readByte();
                auto numEvents = fs.readByte();

                //core::info("  [%08d] Frame %d -> %d: seq %X; %d events\n", timestamp, prevFrameNo, frameNo, stateStringSeq, numEvents);

                /* Snapshot
                	// Read basic frame data
		            buffer >> m_uiFrameNumber
			            >> m_uiPrevFrameNumber
			            >> m_uiTimeStamp
			            >> m_uiLastReceivedClientTime
			            >> m_yStateStringSequence
			            >> m_yNumEvents;


                	// Store the rest of the packet for the game to interpret
		            if (buffer.GetLength() > 18)
			            m_bufferReceived.Write(buffer.Get(buffer.GetReadPos()), buffer.GetUnreadLength());
		            else
			            m_bufferReceived.Clear();
                */


				fs.seek(endSnap);
				snapshotCount++;

                // read reliable game data
				auto count = fs.readDword(); // num clients
				for (; count; count--) {
					int t = fs.readInt(); // index
					int i = fs.readInt();

                    core::bytestream reliableGameData(fs.data() + fs.tell(), i);
                    if (t == 1) {
                        auto id = reliableGameData.readByte();
                        switch (id) {
                        case Gamedata::ChatAll:
                        {
                            auto cid = reliableGameData.readDword();
                            auto msg = reliableGameData.readString();
                            auto client = tmpGame.clientinfo(cid);
                            core::print<core::CON_BLU>("[ALL] ");
                            core::print("%s: %s\n", client ? client->name : "UNKNOWN", msg);
                        } break;
                        case Gamedata::ChatTeam:
                        {
                            auto cid = reliableGameData.readDword();
                            auto msg = reliableGameData.readString();
                            auto client = tmpGame.clientinfo(cid);
                            core::print<core::CON_YLW>("[TEAM] ");
                            core::print("%s: %s\n", client ? client->name : "UNKNOWN", msg);
                        } break;
                        case Gamedata::ChatSquad:
                        {
                            auto cid = reliableGameData.readDword();
                            auto msg = reliableGameData.readString();
                            auto client = tmpGame.clientinfo(cid);
                            core::print<core::CON_CYN>("[SQUAD] ");
                            core::print("%s: %s\n", client ? client->name : "UNKNOWN", msg);
                        } break;
                        case Gamedata::ServerMessage:
                        {
                            auto msg = reliableGameData.readString();
                            core::print<core::CON_CYN>("[SERVER] Server message: ");
                            core::print("%s\n", msg);
                        } break;
                        case Gamedata::Message:
                        {
                            auto msg = reliableGameData.readString();
                            core::info("%s\n", msg);
                        } break;
                        case Gamedata::Death:
                        {
                            //core::info("someone died lol\n");
                            uint32_t idkiller = reliableGameData.readDword();
                            uint32_t idkilled = reliableGameData.readDword();
                            uint16_t idweapon = reliableGameData.readWord();
                            auto killedent = tmpGame.getent(idkilled);
                            if (killedent) {
                                killedent->killed = true;
                                auto killerent = tmpGame.getent(idkiller);
                                auto killer = killerent->typname();
                                auto killed = killedent->typname();
                                ClientInfo killedClient, killerClient;
                                if (killedent->m_iClientNum && tmpGame.clientinfo(killedent->m_iClientNum)) {
                                    killedClient = tmpGame.clientinfo(killedent->m_iClientNum).value();
                                }
                                if (killerent->m_iClientNum && tmpGame.clientinfo(killerent->m_iClientNum)) {
                                    killerClient = tmpGame.clientinfo(killerent->m_iClientNum).value();
                                }
                                core::info("%s(%s) was killed by %s(%s)\n",
                                    killedClient.name, killedent->typname(),
                                    killerClient.name, killerent->typname());
				}
                            //core::info("Received death packet id (killer:%d) (killed:%d) (weapon:%d).\n", idkiller, idkilled, idweapon);
                        } break;
                        }
                    }
                    fs.advance(i);
                    //for (; i; i--)
					//	fs.readByte();
				}

                // read game data
				auto count2 = fs.readDword(); // num clients
				for (; count2; count2--) {
					int t = fs.readInt(); // index
					int i = fs.readInt();
					for (; i; i--)
						fs.readByte();
				}

                // read state strings
				auto count3 = fs.readDword(); // num state strings
				for (; count3; count3--) {
					int t = fs.readInt(); // id
					fs.readWString();
				}
				if(count != 0 || count2 != 0 || count3 != 0)
					core::info("Snapshot length %Xh; counts %d,%d,%d\n", snaplen, count, count2, count3);
			}
            if (fs.tell() > fs.length())
                core::error("hmmm %d > %d\n", fs.tell(), fs.length());
			core::info("Finished processing %d snapshots from replay\n", snapshotCount);

			free(replaydata);
			mz_zip_reader_end(&archive);
			return out;
		}
		return nullptr;
	}
}
