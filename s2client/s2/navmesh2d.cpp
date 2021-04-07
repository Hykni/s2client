#include "navmesh2d.hpp"

namespace s2 {
	navmesh2d::navmesh2d(int width, int height, float worldWidth, float worldHeight)
		: mWidth(width), mHeight(height), mWorldWidth(worldWidth), mWorldHeight(worldHeight) {
		mCellWidth = mWorldWidth / mWidth;
		mCellHeight = mWorldHeight / mHeight;
		mGraph.resize(size_t(width) * height);
		for (int y = 0; y < mHeight; y++) {
			for (int x = 0; x < mWidth; x++) {
				mGraph[size_t(y) * mWidth + x] = node{
					.worldx = x * mCellWidth,
					.worldy = y * mCellHeight,
					.index = (y * mWidth + x),
					.reachable = { }
				};
			}
		}
	}

	const navmesh2d::node& navmesh2d::get(int x, int y) const {
		return mGraph[size_t(y) * mWidth + x];
	}

	navmesh2d::node& navmesh2d::get(int x, int y) {
		return mGraph[size_t(y) * mWidth + x];
	}

	const navmesh2d::node& navmesh2d::getworld(float wx, float wy) const {
		return mGraph[size_t(worldytocell(wy)) * mWidth + worldxtocell(wx)];
	}

	int navmesh2d::generate(std::function<bool(float, float, float, float)> fnObstructed, float capsuleWidth) {
		int numlinks = 0;
		for (int y = 0; y < mHeight; y++) {
			for (int x = 0; x < mWidth; x++) {
				auto& node = get(x, y);
				for (int oy = -2; oy <= 2; oy++) {
					for (int ox = -2; ox <= 2; ox++) {
						if (ox == 0 && oy == 0)
							continue;
						if ((-ox) > x || (-oy) > y || ((x + ox) >= mWidth) || (y + oy) >= mHeight) {
							continue;
						}
                        vector3f from(x * mCellWidth, y * mCellHeight, 0.f);
						vector3f to((x + ox) * mCellWidth, (y + oy) * mCellHeight, 0.f);
                        vector3f capsuleRight = (to - from).perp2d().unit() * capsuleWidth;
						if (fnObstructed(from.x, from.y, to.x, to.y)
                         || fnObstructed(from.x + capsuleRight.x, from.y + capsuleRight.y,
                                        to.x + capsuleRight.x, to.y + capsuleRight.y)
                         || fnObstructed(from.x - capsuleRight.x, from.y - capsuleRight.y,
                                        to.x - capsuleRight.x, to.y - capsuleRight.y))
                            continue;
						float dx = ox * mCellWidth;
						float dy = oy * mCellHeight;
						float cost = sqrtf(dx * dx + dy * dy);
						node.reachable.push_back(navmesh2d::nodelink<navmesh2d::node>{
							.to = get(x + ox, y + oy),
							.cost = cost
						});
						numlinks++;
					}
				}
			}
		}
		return numlinks;
	}
	deque<std::pair<float, float>> navmesh2d::pathfind(float fromx, float fromy, float tox, float toy, const std::function<bool(const vector3f&,const vector3f&)>& prArrived) const {
		deque<std::pair<float, float>> result;
		vector3f src(fromx, fromy, 0.f);
		vector3f dst(tox, toy, 0.f);
		float epsilon = 1.f;
		auto h = [=](float wx, float wy) -> float {
			return epsilon * sqrtf((wx-tox)*(wx-tox) + (wy-toy)*(wy-toy));
		};

		auto fromxcell = worldxtocell(fromx);
		auto fromycell = worldytocell(fromy);
		auto toxcell = worldxtocell(tox);
		auto toycell = worldytocell(toy);
		if (fromxcell < 0 || fromycell < 0 || fromxcell >= mWidth || fromycell >= mHeight)
			return result;
		if (toxcell < 0 || toycell < 0 || toxcell >= mWidth || toycell >= mHeight)
			return result;


		auto& srcn = getworld(src.x, src.y);
		map<const node*, float> gScore;
		gScore[&srcn] = 0.f;
		map<const node*, float> fScore;
		fScore[&srcn] = h(src.x, src.y);

		map<const node*, const node*> cameFrom;

		struct entry {
			float fs;
			const navmesh2d::node* n;
			entry& operator=(const entry&) = default;
			constexpr bool operator>(const entry& o)const { return fs > o.fs; }
		};
		min_heap<entry> qs;

		qs.push(entry{
			.fs=fScore[&srcn],
			.n=&srcn
		});
		while (!qs.empty()) {
			entry e = qs.top();
			qs.pop();

			bool arrived = false;
			if (!prArrived) {
				float dist = (vector3f(e.n->worldx, e.n->worldy, 0.f) - dst).length();
				arrived = (dist <= 1.2f * mCellWidth || dist <= 1.2f * mCellHeight);
			}
			else {
				arrived = prArrived(vector3f(e.n->worldx, e.n->worldy, 0.f), dst);
			}

			if (arrived) {
				result.push_back({ e.n->worldx, e.n->worldy });
				const node* curr = e.n;
				auto it = cameFrom.find(curr);
				while (it != cameFrom.end()) {
					curr = it->second;
					it = cameFrom.find(curr);
					result.insert(result.begin(), { curr->worldx, curr->worldy });
				}
				break;
			}

			float nscore = gScore[e.n];
			for (auto& link : e.n->reachable) {
				float tentative = nscore + link.cost;
				float tentative_fs = tentative + h(link.to.worldx, link.to.worldy);
				auto vgs = gScore.find(&link.to);
				if (vgs == gScore.end() || tentative < vgs->second) {
					cameFrom[&link.to] = e.n;
					gScore[&link.to] = tentative;
					fScore[&link.to] = tentative_fs;
					qs.push(entry{
						.fs = tentative_fs,
						.n = &link.to
					});
				}
			}
		}

		return result;
	}
}
