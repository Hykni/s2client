#include <core/io/filestream.hpp>

namespace core {
	filestream::filestream(FILE* f)
		: mFileHandle(f), mReadIdx(0), mDataLength(0) {
		fseek(f, 0, SEEK_END);
		mDataLength = ftell(f);
		seek(0);
	}
	filestream::~filestream() {
		fclose(mFileHandle);
	}
	bool filestream::read(uint8_t* out, size_t len) {
		if ((mReadIdx + len) > mDataLength)
			return false;
		fread(out, len, 1, mFileHandle);
		mReadIdx += len;
		return true;
	}
}
