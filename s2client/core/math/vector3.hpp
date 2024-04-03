#pragma once

#include <core/prerequisites.hpp>
#include <core/utils/format.hpp>

namespace core {
	template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	class vector3 {
	public:
		T x, y, z;
		
		vector3() : x(0), y(0), z(0) { }
		vector3(T _x, T _y, T _z) : x(_x), y(_y), z(_z) { }

		vector3<T> operator+(const vector3<T>& o)const {
			return vector3<T>(x + o.x, y + o.y, z + o.z);
		}
		vector3<T> operator-(const vector3<T>& o)const {
			return vector3<T>(x - o.x, y - o.y, z - o.z);
		}
		vector3<T>& operator+=(const vector3<T>& o) {
			x += o.x; y += o.y; z += o.z;
			return *this;
		}
		vector3<T>& operator-=(const vector3<T>& o) {
			x -= o.x; y -= o.y; z -= o.z;
			return *this;
		}
		vector3<T>& operator*=(T m) {
			x *= m; y *= m; z *= m;
			return *this;
		}
		vector3<T> operator*(T m)const {
			return vector3<T>(x * m, y * m, z * m);
		}

		T dot(const vector3<T>& o)const {
			return x * o.x + y * o.y + z * o.z;
		}
		vector3<T> cross(const vector3<T>& o)const {
			return vector3<T>(y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x);
		}
		vector3<T> perp2d()const {
			return vector3<T>(y, -x, 0.f);
		}
        void normalize() {
            auto len = length();
            x /= len;
            y /= len;
            z /= len;
        }

		vector3<T> unit(std::enable_if_t<std::is_floating_point_v<T>>* = nullptr)const {
			auto len = length();
			return vector3<T>(x / len, y / len, z / len);
		}
		T length(std::enable_if_t<std::is_floating_point_v<T>>* = nullptr)const {
			return std::sqrt(x * x + y * y + z * z);
		}
		T lengthsq(std::enable_if_t<std::is_floating_point_v<T>>* = nullptr)const {
			return (x * x + y * y + z * z);
		}

        string str()const {
            if constexpr (std::is_same_v<T, float>) {
                return core::format("(%.2f, %.2f, %.2f)", x, y, z);
            }
            else if constexpr (std::is_integral_v<T>) {
                return core::format("(%d, %d, %d)", x, y, z);
            }
            else {
                []<bool flag = false>() { static_assert(flag, "Unprintable vector3 type"); } ();
            }
        }
	};
	template<typename T>
	vector3<T> operator*(T m, const vector3<T>& v) {
		return vector3<T>(v.x * m, v.y * m, v.z * m);
	}
}
using vector3f = core::vector3<float>;
using vector3i = core::vector3<int>;
using vector3h = core::vector3<short>;
