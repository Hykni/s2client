#pragma once

#include <core/prerequisites.hpp>
#include <s2/model.hpp>

namespace s2 {
	struct modelinfo {
		struct {
			vector3f min, max;
		} bounds;
	};
	class modelregistry {
		map<string_view, int> mMdfToId;
	public:
		bool LoadModel()
		bool HasModel(string_view name);
		const model* LookupModelInfo(int id)const;
	private:
		static const map<const string, const modelinfo> Database;
	};
	extern modelregistry* gModelRegistry;
}
