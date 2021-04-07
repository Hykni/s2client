#pragma once

#include <core/prerequisites.hpp>
#include <core/math/vector3.hpp>

namespace s2 {
	class navmesh2d {
		template<typename T>
		struct nodelink {
			T& to;
			float cost;
		};
		struct node {
			float worldx, worldy;
			int index;
			vector<nodelink<node>> reachable;
		};
		vector<node> mGraph;
		int mWidth, mHeight;
		float mWorldWidth, mWorldHeight;
		float mCellWidth, mCellHeight;

		inline int worldxtocell(float wx)const { return int(wx / mCellWidth);  }
		inline int worldytocell(float wy)const { return int(wy / mCellHeight); }
	public:
		navmesh2d(int width, int height, float worldWidth, float worldHeight);

		int width()const { return mWidth; }
		int height()const { return mHeight; }

		const node& get(int x, int y)const;
		node& get(int x, int y);
		const node& getworld(float wx, float wy)const;

		int generate(std::function<bool(float, float, float, float)> fnObstructed, float capsuleWidth=1.f);

		deque<std::pair<float, float>> pathfind(float fromx, float fromy, float tox, float toy, const std::function<bool(const vector3f&, const vector3f&)>& prArrived = nullptr)const;
	};
}
