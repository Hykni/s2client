#pragma once

#include <core/prerequisites.hpp>

namespace core {
	vector<std::pair<int, int>>& fullbresenham(int x0, int y0, int x1, int y1) {
		static vector<std::pair<int, int>> result;
		result.clear();
		int dx = abs(x1 - x0);
		int sx = (x0 < x1) ? 1 : -1;
		int dy = -abs(y1 - y0);
		int sy = y0 < y1 ? 1 : -1;
		int err = dx + dy;
		while (true) {
			result.push_back({ x0, y0 });
			if (x0 == x1 && y0 == y1)
				break;
			int e2 = 2 * err;
			if (e2 >= dy) {
				err += dy;
				x0 += sx;
			}
			if (e2 <= dx) {
				err += dx;
				y0 += sy;
			}
		}
		return result;
	}

	class bresenham {
	public:
		struct Point { int x, y; };
		Point start;
		Point end;
		int dx, dy;
		int sx, sy;
		int err;

		Point curr;
		Point nxt;
		bresenham(int X0, int Y0, int X1, int Y1) : start{ .x=X0,.y=Y0 }, end{ .x=X1, .y=Y1 } {
			dx = abs(end.x - start.x);
			sx = (start.x < end.x) ? 1 : -1;
			dy = -abs(end.y - start.y);
			sy = start.y < end.y ? 1 : -1;
			err = dx + dy;

			nxt = curr = start;
		}

		bool finished()const { return curr.x == end.x && curr.y == end.y; }

		Point next() {
			assert(!finished());
			curr = nxt;

			int e2 = 2 * err;
			if (e2 >= dy) {
				err += dy;
				nxt.x += sx;
			}
			if (e2 <= dx) {
				err += dx;
				nxt.y += sy;
			}

			return curr;
		}
	};

	std::pair<int, int> bresenham_next(int x0, int y0, int x1, int y1) {
		int dx = abs(x1 - x0);
		int sx = (x0 < x1) ? 1 : -1;
		int dy = -abs(y1 - y0);
		int sy = y0 < y1 ? 1 : -1;
		int err = dx + dy;
		
		if (x0 == x1 && y0 == y1)
			return { x0, y0 };

		int xout = x0, yout = y0;
		int e2 = 2 * err;
		if (e2 >= dy)
			xout += sx;
		if (e2 <= dx)
			yout += sy;
		return { xout, yout };
	}
}
