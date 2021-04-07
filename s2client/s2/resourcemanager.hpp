#pragma once

#include <core/prerequisites.hpp>
#include <s2/model.hpp>
#include <core/io/zipfile.hpp>

namespace s2 {
	class resourcemanager {
		map<string, std::shared_ptr<model>> mModels;
		bool LoadMdf(core::zipfile& resources, string_view filename);
		static resourcemanager _Instance;
	public:
		static resourcemanager* Instance() {
			return &_Instance;
		}

		resourcemanager() = default;
		~resourcemanager();

		bool LoadResources(core::zipfile& resources);

		const std::shared_ptr<model> LookupModel(string mdf)const;
		const std::shared_ptr<model> LookupModel(string_view mdf)const { return LookupModel(string(mdf)); }
	};

	extern resourcemanager* gResourceManager;
}
