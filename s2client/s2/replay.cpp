#include <s2/replay.hpp>
#include <core/io/logger.hpp>
#include <core/io/bytestream.hpp>
#include <ext/miniz/miniz.h>

namespace s2 {
	replay::replay() : mMap() {
	}

	std::shared_ptr<replay> replay::LoadFromFile(string_view filename) {
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
				fs.advance(snaplen);
				snapshotCount++;
				auto count = fs.readDword();
				for (; count; count--) {
					int t = fs.readInt();
					int i = fs.readInt();
					for (; i; i--)
						fs.readByte();
				}
				auto count2 = fs.readDword();
				for (; count2; count2--) {
					int t = fs.readInt();
					int i = fs.readInt();
					for (; i; i--)
						fs.readByte();
				}
				auto count3 = fs.readDword();
				for (; count3; count3--) {
					int t = fs.readInt();
					fs.readWString();
				}
				if(count != 0 || count2 != 0 || count3 != 0)
					core::info("Snapshot length %Xh; counts %d,%d,%d\n", snaplen, count, count2, count3);
			}
			core::info("Finished processing %d snapshots from replay\n", snapshotCount);

			free(replaydata);
			mz_zip_reader_end(&archive);
			return out;
		}
		return nullptr;
	}
}
