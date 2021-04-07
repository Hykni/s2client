#include "world.hpp"

#include <core/utils/bresenham.hpp>
#include <core/io/bytestream.hpp>
#include <core/math/interp.hpp>
#include <core/math/mat4.hpp>
#include <core/io/logger.hpp>
#include <ext/tinyxml2/tinyxml2.h>
#include <ext/miniz/miniz.h>

namespace s2 {
	static bool LoadWorldConfig(worldconfig& config, mz_zip_archive* pArchive) {
		size_t datalen;
		const char* xmlconfig = (const char*)mz_zip_reader_extract_file_to_heap(pArchive, "worldconfig", &datalen, 0);
		if (!xmlconfig)
			return false;

		tinyxml2::XMLDocument doc;
		doc.Parse(xmlconfig, datalen);
		auto world = doc.FirstChildElement("world");
		const char* str = nullptr;
		world->QueryStringAttribute("name", &str);
		if (str) {
			config.name = str;
		}
		world->QueryIntAttribute("size", &config.size);
		world->QueryFloatAttribute("scale", &config.scale);
		world->QueryFloatAttribute("textureScale", &config.textureScale);
		world->QueryFloatAttribute("texelDensity", &config.texelDensity);
		world->QueryFloatAttribute("textureScale", &config.textureScale);
		world->QueryFloatAttribute("textureScale", &config.textureScale);
		world->QueryFloatAttribute("groundlevel", &config.groundlevel);
		world->QueryIntAttribute("minplayersperteam", &config.minplayersperteam);
		world->QueryIntAttribute("maxplayers", &config.maxplayers);
		for (int i = 1; tinyxml2::XML_SUCCESS == world->QueryStringAttribute(("music" + std::to_string(i)).c_str(), &str); i++)
			config.music.push_back(str);

		free((void*)xmlconfig);
		return true;
	}

	static bool LoadProps(vector<worldprop>& props, mz_zip_archive* pArchive) {
		size_t datalen;
		const char* entitylist = (const char*)mz_zip_reader_extract_file_to_heap(pArchive, "entitylist", &datalen, 0);
		if (!entitylist)
			return false;

		tinyxml2::XMLDocument doc;
		doc.Parse(entitylist, datalen);
		auto wel = doc.FirstChildElement("WorldEntityList");
		for (auto e = wel->FirstChildElement(); e != wel->LastChildElement(); e=e->NextSiblingElement()) {
			string_view model = e->Attribute("model");
			if (model.find("/tools/blocker") != string::npos)
				continue;
			if (model.find("/nature/waterfall") != string::npos)
				continue;
			if (model.find("/rock_arch") != string::npos)
				continue;
			if (model.find("/props/natural_bridge") != string::npos)
				continue;
			float scale = e->FloatAttribute("scale", 1.0f);
			string_view type = e->Attribute("type");
			string spos = e->Attribute("position");
			string sangles = e->Attribute("angles");
			vector3f vpos, vangles;
			{
				size_t idx = 0, nchars = 0;
				vpos.x = std::stof(spos, &nchars); idx += nchars;
				vpos.y = std::stof(spos.substr(idx), &nchars); idx += nchars;
				vpos.z = std::stof(spos.substr(idx), &nchars);
			}
			{
				size_t idx = 0, nchars = 0;
				vangles.x = std::stof(sangles, &nchars); idx += nchars;
				vangles.y = std::stof(sangles.substr(idx), &nchars); idx += nchars;
				vangles.z = std::stof(sangles.substr(idx), &nchars); idx += nchars;
			}
			string modelpath;
			if (model.find("_arch.mdf") != string::npos) {
				
			}
			if (model.find("/effects/") != string::npos)
				continue;
			if (model.ends_with(".mdf"))
				modelpath = string(model.substr(0, model.size() - 4)) + ".model";
			else
				modelpath = model;
			auto mi = gResourceManager->LookupModel(model);
			core::info("%s: %s @ %s; angles %s\n", type, model, vpos.str(), vangles.str());
			static map<string, float> predefinedSz = {
				{ "/trees/",  5.f },
				{ "/shrubs/", 5.f },
				{ "/rocks/",  10.f }
			};
			if (mi) {
				//if (0 == type.compare("Prop_Scenery") || 0 == type.compare("Prop_Mine") || 0 == type.compare("Prop_Scar")) {
				props.push_back(worldprop{ .modelname = model.data(), .type = string(type), .scale = scale, .pos = vpos, .angles = vangles, .model = mi });
				auto sz = (mi->BBMax() - mi->BBMin());
				core::info("Added prop size %fm2\n", (abs(sz.x * sz.y)));
				//}
			}
			else {
				for (auto& pd : predefinedSz) {

				}
			}
		}

		free((void*)entitylist);
		return true;
	}

	static bool LoadHeightmap(map2d<float>& map, mz_zip_archive* pArchive) {
		size_t datalen;
		uint8_t* pheightmap = (uint8_t*)mz_zip_reader_extract_file_to_heap(pArchive, "heightmap", &datalen, 0);
		if (!pheightmap)
			return false;

		auto stream = core::bytestream(pheightmap, datalen);
		int width = stream.readInt();
		int height = stream.readInt();
		bool heightbytes = width < 0;
		if (width < 0)
			width = -width;
		map.initialize(width, height);
		if (heightbytes) {
			for (int i = 0; i < (width * height); i++)
				map[i] = stream.readByte() * 0.00390625f;
			for (int i = 0; i < (width * height); i++)
				map[i] += float(stream.readByte());
			for (int i = 0; i < (width * height); i++)
				map[i] = ((256.f * stream.readByte()) + map[i]) - 32768.f;
		}
		else {
			for (int i = 0; i < (width * height); i++)
				map[i] = stream.readFloat();
		}

		free(pheightmap);
		core::info("Loaded %dx%d heightmap.\n", width, height);
		return true;
	}

	static bool LoadVertexBlockers(map2d<bool>& map, mz_zip_archive* pArchive) {
		size_t datalen;
		uint8_t* pblockers = (uint8_t*)mz_zip_reader_extract_file_to_heap(pArchive, "vertexblockermap", &datalen, 0);
		if (!pblockers)
			return false;

		auto stream = core::bytestream(pblockers, datalen);
		int width = stream.readInt();
		int height = stream.readInt();
		map.initialize(width, height);
		for (int i = 0; i < (width * height); i++)
			map[i] = stream.readByte() != 0;

		free(pblockers);
		core::info("Loaded %dx%d vertex blockers.\n", width, height);
		return true;
	}

	void world::generateobstructionmap(float cellsize) {
		int sz = int(mWorldSize / cellsize);
		mObstructionMap.initialize(sz, sz);
		for (int y = 0; y < sz; y++) {
			for (int x = 0; x < sz; x++) {
				float wx = x * cellsize + 0.5f * cellsize;
				float wy = y * cellsize + 0.5f * cellsize;
				if (isblocked(wx, wy) || terrainslope(wx, wy) > 0.95f || testrectinscenery(x*cellsize, y*cellsize, cellsize, cellsize, 20.f))
				{
					mObstructionMap.set(x, y, true);
				}
				else {
					mObstructionMap.set(x, y, false);
				}
			}
		}
	}

	void world::init() {
		assert(mConfig.size < 31);
		mWorldDefinitionSize = (1 << mConfig.size) + 1;
		mWorldSize = mConfig.scale * mWorldDefinitionSize;

		mPropsQt = std::make_unique<core::quadtree<worldprop>>(mWorldSize, mWorldSize, 50.f);
		for (auto& prop : mProps) {
			auto bbmin = prop.model->BBMin() * prop.scale;
			auto bbmax = prop.model->BBMax() * prop.scale;
			auto c1 = bbmin; auto c2 = vector3f(bbmin.x, bbmax.y, bbmin.z); auto c3 = bbmax; auto c4 = vector3f(bbmax.x, bbmin.y, bbmax.z);
			auto mrot = mat4f::zrotation(-prop.angles.z * float(M_PI) / 180.f);
			c1 = mrot * c1; c2 = mrot * c2; c3 = mrot * c3; c4 = mrot * c4;
			c1 += prop.pos; c2 += prop.pos; c3 += prop.pos; c4 += prop.pos;
			vector3f nmin(min({ c1.x, c2.x, c3.x, c4.x }), min({ c1.y, c2.y, c3.y, c4.y }), 0.f);
			vector3f nmax(max({ c1.x,c2.x,c3.x,c4.x }), max({ c1.y,c2.y,c3.y,c4.y }), 0.f);
			mPropsQt->insert(core::rect(nmin.x, nmin.y, nmax.x - nmin.x, nmax.y - nmin.y), &prop);
		}

		core::info("Generating obstruction map...\n");
		generateobstructionmap(32.f);
		core::info("Finished generating obstruction map.\n");
		
		float nmCellSize = 64.f;
		int nmSize = int(mWorldSize / nmCellSize);
		mNavmesh = std::make_shared<navmesh2d>(nmSize, nmSize, mWorldSize, mWorldSize);
		core::info("Generating navigation mesh...\n");
		int nlinks = mNavmesh->generate([this](auto&&...args) -> bool { return testlineobstructed(args...); }, 40.f);
		core::info("Generated %dx%d navmesh with %d links\n", nmSize, nmSize, nlinks);
	}

	bool world::testrectinscenery(float x, float y, float w, float h, float radius) {
		vector3f min, max;
		auto results = mPropsQt->query(core::rect(x - radius, y - radius, w + radius * 2.f, h + radius * 2.f));
		for (auto& pP : results) {
			auto& p = *pP;
			min = p.model->BBMin();
			min *= p.scale;
			min.z = 0.f;
			max = p.model->BBMax();
			max *= p.scale;
			max.z = 0.f;
			vector3f pos = vector3f(x - p.pos.x, y - p.pos.y, 0.f);
			auto largestSideSq = std::max((max.x - min.x)*(max.x-min.x), (max.y - min.y)*(max.y - min.y));
			if (p.angles.z == 0.f) {
				// test two axis-aligned rects w simplified SAT test
				if (!( (pos.x - max.x) > radius
					|| radius < (min.x - (pos.x + w))
					|| (pos.y - max.y) > radius
					|| radius < (min.y - (pos.y + h))))
					return true;
			}
			else if(largestSideSq > pos.lengthsq()) {
				auto c1 = min; auto c2 = vector3f(min.x, max.y, min.z); auto c3 = max; auto c4 = vector3f(max.x, min.y, max.z);
				c1.z = c2.z = c3.z = c4.z = 0.f;
				auto mrot = mat4f::zrotation(-p.angles.z * float(M_PI) / 180.f);
				c1 = mrot * c1; c2 = mrot * c2; c3 = mrot * c3; c4 = mrot * c4;
				auto xaxis = vector3f(1.f, 0.f, 0.f);
				auto yaxis = vector3f(0.f, 1.f, 0.f);
				auto a1 = mrot * xaxis;
				auto a2 = mrot * yaxis;
				vector3f ra[4] = { pos, pos + xaxis * w, pos + xaxis * w + yaxis * h, pos + yaxis * h };
				vector3f rb[4] = { c1, c2, c3, c4 };
				vector3f separating_axes[] = { xaxis, yaxis, a1, a2 };
				bool separated = false;
				for (int i = 0; i < (sizeof(separating_axes) / sizeof(separating_axes[0])); i++) {
					auto& axis = separating_axes[i];
					float aprojs[4] = { ra[0].dot(axis), ra[1].dot(axis), ra[2].dot(axis), ra[3].dot(axis) };
					float bprojs[4] = { rb[0].dot(axis), rb[1].dot(axis), rb[2].dot(axis), rb[3].dot(axis) };
					float mina = std::min(aprojs[0], std::min(aprojs[1], std::min(aprojs[2], aprojs[3])));
					float maxa = std::max(aprojs[0], std::max(aprojs[1], std::max(aprojs[2], aprojs[3])));
					float minb = std::min(bprojs[0], std::min(bprojs[1], std::min(bprojs[2], bprojs[3])));
					float maxb = std::max(bprojs[0], std::max(bprojs[1], std::max(bprojs[2], bprojs[3])));
					if ((mina - maxb) > radius
					 || (minb - maxa) > radius) {
						separated = true;
						break;
					}
				}
				if (!separated)
					return true;
			}
		}
		return false;
	}

	bool world::testpointinscenery(float x, float y) {
		auto results = mPropsQt->query(core::rect(x - 0.5f, y - 0.5f, 1.f, 1.f));
		for (auto& pP : results) {
			auto& p = *pP;
			auto min = p.model->BBMin() * p.scale; min.z = 0.f;
			auto max = p.model->BBMax() * p.scale; max.z = 0.f;
			vector3f pos = vector3f(x, y, 0.f) - p.pos;
			pos.z = 0.f;
			if (p.angles.z == 0.f) {
				if (pos.x >= min.x && pos.x <= max.x
					&& pos.y >= min.y && pos.y <= max.y)
					return true;
			}
			else {
				auto c1 = min; auto c2 = vector3f(min.x, max.y, min.z); auto c3 = max; auto c4 = vector3f(max.x, min.y, max.z);
				auto width = abs(max.x - min.x);
				auto height = abs(max.y - min.y);
				auto mrot = mat4f::zrotation(-p.angles.z * float(M_PI) / 180.f);
				c1 = mrot * c1; c2 = mrot * c2; c3 = mrot * c3; c4 = mrot * c4;
				auto c12pos = pos - c1;
				float xv = c12pos.dot((c4 - c1).unit());
				float yv = c12pos.dot((c2 - c1).unit());
				if (xv >= 0.f && xv <= width && yv >= 0.f && yv <= height)
					return true;
			}
		}
		return false;
	}

	bool world::testlineobstructed(float x0, float y0, float x1, float y1) {
		x0 = max(0.f, min(mWorldSize, x0));
		y0 = max(0.f, min(mWorldSize, y0));
		x1 = max(0.f, min(mWorldSize, x1));
		y1 = max(0.f, min(mWorldSize, y1));
		int x0i = int((x0 / mWorldSize) * mObstructionMap.getwidth());
		int y0i = int((y0 / mWorldSize) * mObstructionMap.getheight());
		int x1i = int((x1 / mWorldSize) * mObstructionMap.getwidth());
		int y1i = int((y1 / mWorldSize) * mObstructionMap.getheight());

		auto line = core::bresenham(x0i, y0i, x1i, y1i);
		while (!line.finished()) {
			auto nxt = line.next();

			if (mObstructionMap.get(nxt.x, nxt.y) == true)
				return true;
		}
		return false;
	}

	std::shared_ptr<world> world::LoadFromFile(string_view filename) {
		mz_zip_archive archive;
		memset(&archive, 0, sizeof(archive));
		if (mz_zip_reader_init_file(&archive, filename.data(), 0)) {
			struct construct_world : public s2::world {};
			std::shared_ptr<world> world = std::make_shared<construct_world>();
			if (!LoadWorldConfig(world->mConfig, &archive)) {
				mz_zip_reader_end(&archive);
				core::error("Failed to read worldconfig for world %s.", filename);
				return nullptr;
			}
			if (!LoadHeightmap(world->mHeightmap, &archive)) {
				mz_zip_reader_end(&archive);
				core::error("Failed to read heightmap for world %s.", filename);
				return nullptr;
			}
			if (!LoadVertexBlockers(world->mVertexBlockers, &archive)) {
				mz_zip_reader_end(&archive);
				core::error("Failed to read vertexblockermap for world %s.", filename);
				return nullptr;
			}
			if (!LoadProps(world->mProps, &archive)) {
				mz_zip_reader_end(&archive);
				core::error("Failed to read entitylist for world %s.", filename);
				return nullptr;
			}
			world->init();
			mz_zip_reader_end(&archive);
			return world;
		}
		return nullptr;
	}
	string_view world::name() const {
		return mConfig.name;
	}
	const vector<worldprop>& world::props() const {
		return mProps;
	}
	float world::worldsize() const {
		return mWorldSize;
	}
	bool world::isblocked(float x, float y) const {
		float hmx = ((x / mWorldSize) * mHeightmap.getwidth());
		float hmy = ((y / mWorldSize) * mHeightmap.getheight());
		int xi = int(hmx);
		int yi = int(hmy);
		return mVertexBlockers.get(xi, yi);
	}
	float world::terrainheight(float worldx, float worldy) const {
		float hmx = ((worldx / mWorldSize) * mHeightmap.getwidth());
		float hmy = ((worldy / mWorldSize) * mHeightmap.getheight());
		int xi = int(hmx);
		int yi = int(hmy);
		int xi2 = xi + 1;
		int yi2 = yi + 1;
		if (xi2 >= mHeightmap.getwidth())
			xi2 = xi;
		if (yi2 >= mHeightmap.getwidth())
			yi2 = yi;
		float q11 = mHeightmap.get(xi, yi);
		float q21 = mHeightmap.get(xi2, yi);
		float q12 = mHeightmap.get(xi, yi2);
		float q22 = mHeightmap.get(xi2, yi2);

		return core::bilinear(q11, q12, q21, q22, hmx - xi, hmy - yi);
	}
	float world::terrainslope(float worldx, float worldy) const {
		float ht = terrainheight(worldx, worldy);
		float bdx = terrainheight(worldx - 1.f, worldy) - ht;
		float fdx = terrainheight(worldx + 1.f, worldy) - ht;
		float bdy = terrainheight(worldx, worldy - 1.f) - ht;
		float fdy = terrainheight(worldx, worldy + 1.f) - ht;
		return max(sqrtf(fdx * fdx + fdy * fdy), sqrtf(bdx * bdx + bdy * bdy));

		float hmx = ((worldx / mWorldSize) * mHeightmap.getwidth());
		float hmy = ((worldy / mWorldSize) * mHeightmap.getheight());
		int xi = int(hmx);
		int yi = int(hmy);
		int xi2 = xi + 1;
		int yi2 = yi + 1;
		if (xi2 >= mHeightmap.getwidth())
			xi2 = xi;
		if (yi2 >= mHeightmap.getwidth())
			yi2 = yi;
		float q11 = mHeightmap.get(xi, yi);
		float q21 = mHeightmap.get(xi2, yi);
		float q12 = mHeightmap.get(xi, yi2);
		float q22 = mHeightmap.get(xi2, yi2);



		return max({ q11, q21, q12, q22 }) - min({ q11, q21, q12, q22 });
		float cellw = mWorldSize / mHeightmap.getwidth();
		float cellh = mWorldSize / mHeightmap.getheight();
		float fx = max(1e-9f, hmx - xi);
		float fy = max(1e-9f, hmy - yi);
		float ifx = 1.f - fx;
		float ify = 1.f - fy;
		fx *= cellw; ifx *= cellw;
		fy *= cellh; ify *= cellh;
		return max(
			max(
				abs(ht - q11) / (sqrtf(ifx*ifx+ify*ify)),
				abs(ht - q22) / (sqrtf(fx*fx+fy*fy))
			),
			max(
				abs(ht - q21) / (sqrtf(fx*fx+ify*ify)),
				abs(ht - q12) / (sqrtf(ifx*ifx+fy*fy))
			)
		);
	}
	vector<vector3f> world::navmeshlines() const {
		auto width = mNavmesh->width();
		auto height = mNavmesh->height();
		vector<vector3f> lines;
		lines.reserve(width * height * 8);
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				auto& node = mNavmesh->get(x, y);
				for (auto& l : node.reachable) {
					lines.push_back(vector3f(node.worldx, node.worldy, 0.f));
					lines.push_back(vector3f(l.to.worldx, l.to.worldy, 0.f));
				}
			}
		}
		return lines;
	}
	deque<vector3f> world::pathfind(const vector3f& from, const vector3f& to, const std::function<bool(const vector3f&, const vector3f&)>& prArrived) const {
		deque<vector3f> result;

		auto path2d = mNavmesh->pathfind(from.x, from.y, to.x, to.y, prArrived);
		for (auto& wp : path2d) {
			auto x = wp.first;
			auto y = wp.second;
			auto z = terrainheight(x, y);
			result.push_back(vector3f(x, y, z));
		}

		return result;
	}
}
