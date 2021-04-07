#pragma once

#include <core/prerequisites.hpp>

namespace core {
	class rect {
	public:
		float x, y, w, h;
		rect(float X, float Y, float Width, float Height)
			: x(X), y(Y), w(Width), h(Height) { }
		rect(rect&& r) = default;
		rect(const rect& r) = default;

		bool contains(float px, float py)const {
			if (!(px >= x && px <= (x + w)))
				return false;
			if(!(py >= y && py <= (y + h)))
				return false;
			return true;
		}

		bool intersects(const rect& r)const {
			// SAT simplification
			if (x >= (r.x + r.w) || r.x >= (x + w))
				return false;
			if (y >= (r.y + r.h) || r.y >= (y + h))
				return false;
			return true;
		}

		std::array<rect, 4> split()const {
			auto hw = w * 0.5f;
			auto hh = h * 0.5f;
			return {
				rect(x, y, hw, hh),
				rect(x + hw, y, hw, hh),
				rect(x + hw, y + hh, hw, hh),
				rect(x, y + hh, hw, hh)
			};
		}
	};

	template<class TVal>
	class quadtree {
		struct entry {
			rect bounds;
			TVal* val;
		};
		class node {
		public:
			rect bounds;
			node* nw, * ne, * se, * sw;
			vector<entry> vals;
			node(rect bounds,
				node* nw = nullptr,
				node* ne = nullptr,
				node* sw = nullptr,
				node* se = nullptr) : bounds(bounds) {
				this->nw = nw;
				this->ne = ne;
				this->se = se;
				this->sw = sw;
			}
			void split() {
				if (!leaf())
					__debugbreak();
				assert(leaf());
				auto bs = bounds.split();
				nw = new node(bs[0]);
				ne = new node(bs[1]);
				se = new node(bs[2]);
				sw = new node(bs[3]);
				for (auto&& e : vals) {
					if (nw->bounds.intersects(e.bounds))
						nw->vals.push_back(e);
					if (ne->bounds.intersects(e.bounds))
						ne->vals.push_back(e);
					if (se->bounds.intersects(e.bounds))
						se->vals.push_back(e);
					if (sw->bounds.intersects(e.bounds))
						sw->vals.push_back(e);
				}
				vals.clear();
				vals.shrink_to_fit();
			}
			bool leaf()const {
				return !(nw || ne || se || sw);
			}
			void clear() {
				if (!leaf()) {
					nw->clear();
					ne->clear();
					se->clear();
					sw->clear();
				}
				vals.clear();
				delete nw; delete ne; delete se; delete sw;
				nw = ne = se = sw = nullptr;
			}
		};
		float mMinSize;
		int mCount;
		node mRoot;
	
		const int MaxNodesPerQuad = 2;
		bool shouldSplitNode(node* n) {
			if (n->vals.size() >= MaxNodesPerQuad) {
				auto sz = max(n->bounds.w, n->bounds.h);
				return sz >= (2.f*mMinSize);
			}
			return false;
		}
	public:
		quadtree(float width, float height, float minimumSize = 5.f)
			: mMinSize(minimumSize),
			mCount(0),
			mRoot(rect(0, 0, width, height)) {
		}
		~quadtree() {
			clear();
		}

		const node& root() { return mRoot; }

		void insert(rect r, TVal* tv) {
			vector<node*> nodes;
			nodes.reserve(32);
			nodes.push_back(&mRoot);
			while (!nodes.empty()) {
				node* current = nodes.back();
				nodes.pop_back();
				if (current->bounds.intersects(r)) {
					if (current->leaf()) {
						if (shouldSplitNode(current)) {
							current->split();
							nodes.push_back(current->nw);
							nodes.push_back(current->ne);
							nodes.push_back(current->se);
							nodes.push_back(current->sw);
						}
						else {
							current->vals.push_back({ r, tv });
						}
					}
					else {
						nodes.push_back(current->nw);
						nodes.push_back(current->ne);
						nodes.push_back(current->se);
						nodes.push_back(current->sw);
					}
				}
			}

			mCount++;
		}

		vector<TVal*> query(const rect& area) {
			vector<TVal*> results;
			vector<node*> nodes;
			nodes.push_back(&mRoot);
			while (!nodes.empty()) {
				node* current = nodes.back();
				nodes.pop_back();
				if (current->bounds.intersects(area)) {
					if (current->leaf()) {
						for (auto& e : current->vals) {
							if (e.bounds.intersects(area)) {
								auto it = std::lower_bound(results.begin(), results.end(), e.val);
								if (it == results.end() || *it != e.val)
									results.insert(it, e.val);
								//results.insert(e.val);
							}
						}
					}
					else {
						nodes.push_back(current->nw);
						nodes.push_back(current->ne);
						nodes.push_back(current->se);
						nodes.push_back(current->sw);
					}
				}
			}
			return results;
		}

		void clear() {
			mRoot.clear();
			mCount = 0;
			/*vector<node*> nodes;
			if (mRoot.leaf())
				return;
			else {
				nodes.push_back(mRoot.nw);
				nodes.push_back(mRoot.ne);
				nodes.push_back(mRoot.se);
				nodes.push_back(mRoot.sw);
			}
			vector<node*> toDelete;
			while (!nodes.empty()) {
				node* current = nodes.back();
				nodes.pop_back();
				if (!current->leaf()) {
					nodes.push_back(current->nw);
					nodes.push_back(current->ne);
					nodes.push_back(current->se);
					nodes.push_back(current->sw);

					toDelete.push_back(current->nw);
					toDelete.push_back(current->ne);
					toDelete.push_back(current->se);
					toDelete.push_back(current->sw);
				}
			}

			while (!toDelete.empty()) {
				node* current = toDelete.back();
				toDelete.pop_back();
				delete current;
			}*/

		}
	};
}
