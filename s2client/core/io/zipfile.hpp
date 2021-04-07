#pragma once

#include <core/prerequisites.hpp>
#include <core/io/bytestream.hpp>
#include <ext/miniz/miniz.h>

namespace core {
	class zipstream : public bytestream {
	public:
		zipstream(uint8_t* data, size_t len) : bytestream(data, len) { }
		~zipstream() { free((void*)mData); }
	};
	class zipfile {
	private:
		mz_zip_archive* mArchive;

	public:
		zipfile(mz_zip_archive* a) : mArchive(a) { }
		~zipfile() {
			mz_zip_reader_end(mArchive);
			delete mArchive;
		}
		static std::shared_ptr<zipfile> Load(string_view filename) {
			mz_zip_archive* archive = new mz_zip_archive{};
			memset(archive, 0, sizeof(*archive));
			if (!mz_zip_reader_init_file(archive, filename.data(), 0))
				return nullptr;
			else
				return std::make_shared<zipfile>(archive);
		}

		unsigned int numfiles() {
			return mz_zip_reader_get_num_files(mArchive);
		}

		string namebyidx(unsigned int idx) {
			string name;
			name.resize(mz_zip_reader_get_filename(mArchive, idx, nullptr, 0));
			mz_zip_reader_get_filename(mArchive, idx, (char*)name.data(), (mz_uint)name.length());
			name.resize(name.length() - 1);
			return name;
		}

		std::shared_ptr<zipstream> file(string_view name) {
			size_t len;
			uint8_t* data = (uint8_t*)mz_zip_reader_extract_file_to_heap(mArchive, name.data(), &len, 0);
			if (!data)
				return nullptr;
			else
				return std::make_shared<zipstream>(data, len);
		}
	};
}
