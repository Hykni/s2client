#pragma once

#include <core/prerequisites.hpp>
#include <core/math/vector3.hpp>

namespace math {
	static bool LineIntersectsPlane(const vector3f& A, const vector3f& B, const vector3f& N, const vector3f& P, float* t_out=nullptr) {
		auto a2b = B - A;
		auto c = N.dot(a2b);
		float t_p = N.dot(P - A) / c;
		if (t_p < 0.f || t_p > 1.0f)
			return false;
		if (t_out) {
			*t_out = t_p;
		}
		return true;
	}

	static bool LineIntersectsTriangle(const vector3f& A, const vector3f& B,
		const vector3f& C1, const vector3f& C2, const vector3f& C3, float* t_out=nullptr) {
		vector3f N = (C2 - C1).cross(C3 - C1);
		float t_p;
		// Check line intersects plane formed by triangle
		if (!LineIntersectsPlane(A, B, N, C1, &t_p))
			return false;
		
		// Check plane intersection point is inside triangle
		vector3f Q = A + t_p * (B - A);
		auto j = (C2 - C1).cross(Q - C1).dot(N);
		auto k = (C3 - C2).cross(Q - C2).dot(N);
		auto l = (C1 - C3).cross(Q - C3).dot(N);
		if (!(signbit(j) == signbit(k) && signbit(k) == signbit(l)))
			return false;

		if (t_out)
			*t_out = t_p;

		return true;
	}
}
