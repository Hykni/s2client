#pragma once

#include <core/prerequisites.hpp>

namespace s2 {
	enum class VarType : uint8_t {
		Byte = 0,
		Short = 1,
		Int = 2,
		Single = 3,
		Qword = 4,
		Vector3 = 5,
		String = 6,
		WordEntityIndex = 7,
		WordHandle = 8,
		WordAngle = 9,
		WordFloat = 10,
		ByteFloat = 11,
		WordVector3 = 12
	};
	struct varinfo {
		string name;
		VarType type;
		uint32_t minversion;
		uint32_t maxversion;
	};
	struct typeinfo {
		string name;
		vector<varinfo> vars;
	};
	class typeregistry {
	public:
		static bool HasType(int id);
		static bool LookupTypeInfo(int id, const typeinfo** out);
	private:
		static const map<const int, const typeinfo> Database;
	};
}
