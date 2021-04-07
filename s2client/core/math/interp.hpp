#pragma once

#include <core/prerequisites.hpp>

namespace core {
	template<typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
	__forceinline constexpr T invlerp(T a, T b, T v) {
		return (v - a) / (b - a);
	}

	template<typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
	__forceinline constexpr T lerp(T a, T b, T v) {
		return  v * (b - a);
	}

	template<typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
	__forceinline constexpr T bilinear(T q11, T q12, T q21, T q22, T xfr, T yfr) {
		T fx1 = (T(1.0) - xfr) * q11 + xfr * q21;
		T fx2 = (T(1.0) - xfr) * q12 + xfr * q22;
		return (T(1.0) - yfr) * fx1 + yfr * fx2;
	}
}
