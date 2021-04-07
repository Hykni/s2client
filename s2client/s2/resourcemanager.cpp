#include "resourcemanager.hpp"
#include <core/io/logger.hpp>
#include <ext/tinyxml2/tinyxml2.h>

namespace s2 {
	resourcemanager resourcemanager::_Instance;
	resourcemanager* gResourceManager = resourcemanager::Instance();

	bool resourcemanager::LoadMdf(core::zipfile& resources, string_view filename) {
		auto mdfdata = resources.file(filename);
		if (!mdfdata) {
			core::warning("Failed to load mdf from %s\n", filename);
			return false;
		}

		tinyxml2::XMLDocument doc;
		auto err = doc.Parse((const char*)mdfdata->data(), mdfdata->length());
		if (err != tinyxml2::XML_SUCCESS) {
			core::warning("Failed to parse mdf xml in %s\n", filename);
			return false;
		}

		auto emodel = doc.FirstChildElement("model");
		if (!emodel) {
			core::warning("Failed to find model definition in %s\n", filename);
			return false;
		}

		const char* xmlmdlfile;
		string mdlfile;
		if (tinyxml2::XML_SUCCESS != emodel->QueryStringAttribute("file", &xmlmdlfile)) {
			core::warning("No model file specified in %s\n", filename);
			return false;
		}
		if (xmlmdlfile[0] == '/')
			mdlfile = &xmlmdlfile[1];
		else
			mdlfile = string(filename.substr(0, filename.find_last_of('/') + 1)) + xmlmdlfile;

		auto mdldata = resources.file(mdlfile);
		if (!mdldata) {
			core::warning("Failed to load model file %s (mdf: %s)\n", mdlfile, filename);
			return false;
		}

		auto model = s2::model::Load(*mdldata);
		if (!model) {
			core::warning("Error loading model %s\n", mdlfile);
			return false;
		}
		mModels['/' + string(filename)] = model;

		return true;
	}
	resourcemanager::~resourcemanager() {
		mModels.clear();
	}
	bool resourcemanager::LoadResources(core::zipfile& resources) {
		auto numfiles = resources.numfiles();
		for (unsigned int i = 0; i < numfiles; i++) {
			auto name = resources.namebyidx(i);
			//core::info("Loading %s\n", name);
			if (name.ends_with(".mdf")) {
				if (LoadMdf(resources, name))
					core::info("Loaded %s successfully.\n", name);
			}
		}
		return true;
	}
	const std::shared_ptr<model> resourcemanager::LookupModel(string mdf) const {
		auto it = mModels.find(mdf);
		if (it == mModels.end())
			return nullptr;
		else
			return it->second;
	}
}
