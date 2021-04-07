#pragma once

#include <core/prerequisites.hpp>
#include <core/math/vector3.hpp>
#include <s2/resourcemanager.hpp>
#include <s2/navmesh2d.hpp>
#include <core/utils/quadtree.hpp>

namespace s2 {
	template<typename T>
	class map2d {
		T* data = nullptr;
		int width = 0;
		int height = 0;
		void release() {
			if (data)
				delete[] data;
			data = nullptr;
			width = height = 0;
		}
	public:
		map2d<T>() = default;
		map2d<T>(const map2d<T>& o) noexcept {
			initialize(o.width, o.height);
			for (int i = 0; i < (width * height); i++)
				data[i] = o.data[i];
		}
		map2d<T>(map2d<T>&& o) noexcept {
			release();
			data = o.data;
			width = o.width; height = o.height;
			o.data = nullptr; o.width = 0; o.height = 0;
		}
		~map2d<T>() {
			release();
		}
		const map2d<T>& operator=(const map2d<T>& o) noexcept {
			release();
			initialize(o.width, o.height);
			for (int i = 0; i < (width * height); i++)
				data[i] = o.data[i];
			return *this;
		}
		const map2d<T>& operator=(map2d<T>&& o) noexcept {
			release();
			data = o.data;
			width = o.width; height = o.height;
			o.data = nullptr; o.width = 0; o.height = 0;
			return *this;
		}
		void initialize(int Width, int Height) {
			width = Width; height = Height;
			data = new T[size_t(width) * height];
		}

		const T& get(int x, int y)const {
			return data[y * width + x];
		}
		void set(int x, int y, const T& val) {
			data[y * width + x] = val;
		}

		const T& operator[](int idx)const {
			return data[idx];
		}
		T& operator[](int idx) {
			return data[idx];
		}

		const int getwidth()const { return width; }
		const int getheight()const { return height; }
	};
	struct worldconfig {
		string name;
		int size;
		float scale;
		float textureScale;
		float texelDensity;
		float groundlevel;
		int minplayersperteam;
		int maxplayers;
		vector<string> music;
	};
	struct worldprop {
		string modelname;
		string type;
		float scale;
		vector3f pos;
		vector3f angles;
		std::shared_ptr<s2::model> model;
	};
	class world {
	private:
		world() = default;

		map2d<float> mHeightmap;
		map2d<bool> mVertexBlockers;
		map2d<bool> mObstructionMap;
		worldconfig mConfig;
		vector<worldprop> mProps;
		std::unique_ptr<core::quadtree<worldprop>> mPropsQt;
		std::shared_ptr<navmesh2d> mNavmesh;

		uint32_t mWorldDefinitionSize = 0;
		float mWorldSize = 0.0f;
		void generateobstructionmap(float cellSize);
		void init();
	public:
		bool testrectinscenery(float x, float y, float w, float h, float radius = 1.f);
		bool testpointinscenery(float x, float y);
		bool testlineobstructed(float x0, float y0, float x1, float y1);
		world(world&& o) = default;
		world(const world& o) = default;
		world& operator=(const world& o) = default;
		world& operator=(world&& o) noexcept = default;

		static std::shared_ptr<world> LoadFromFile(string_view filename);

		string_view name()const;
		const vector<worldprop>& props()const;
		float worldsize()const;
		bool isblocked(float x, float y)const;
		float terrainheight(float x, float y)const;
		float terrainslope(float x, float y)const;
		vector<vector3f> navmeshlines()const;

		deque<vector3f> pathfind(const vector3f& from, const vector3f& to, const std::function<bool(const vector3f&, const vector3f&)>& prArrived = nullptr)const;
	};
}
