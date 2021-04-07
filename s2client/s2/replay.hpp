#pragma once

#include <core/prerequisites.hpp>

namespace s2 {
	class replay {
		replay();

		wstring mMap;

		typedef map<string, string> VarSet;
		map<int, VarSet> mState;
	public:

		static std::shared_ptr<replay> LoadFromFile(string_view filename);
	};
}
