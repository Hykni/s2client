#pragma once

#include <core/prerequisites.hpp>
#include <core/math/vector3.hpp>

namespace core {
#pragma pack(push,1)
	template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	class mat4 {
	public:
		T _11, _12, _13, _14;
		T _21, _22, _23, _24;
		T _31, _32, _33, _34;
		T _41, _42, _43, _44;
	
		constexpr mat4() { }
		constexpr mat4(const mat4& o)
			: _11(o._11), _12(o._12), _13(o._13), _14(o._14),
			  _21(o._21), _22(o._22), _23(o._23), _24(o._24),
			  _31(o._31), _32(o._32), _33(o._33), _34(o._34),
			  _41(o._41), _42(o._42), _43(o._43), _44(o._44) { }
		constexpr mat4(std::initializer_list<T> l) {
			//static_assert(l.size() == 16, "Invalid length initializer list for 4x4 matrix");
			auto it = l.begin();
			_11 = *it++; _12 = *it++; _13 = *it++; _14 = *it++;
			_21 = *it++; _22 = *it++; _23 = *it++; _24 = *it++;
			_31 = *it++; _32 = *it++; _33 = *it++; _34 = *it++;
			_41 = *it++; _42 = *it++; _43 = *it++; _44 = *it++;
		}
		static constexpr mat4 identity() {
			return mat4 {
				T(1), 0, 0, 0,
				0, T(1), 0, 0,
				0, 0, T(1), 0,
				0, 0, 0, T(1)
			};
		}

		constexpr mat4 transpose() {
			return mat4{
				_11, _21, _31, _41,
				_12, _22, _32, _42,
				_13, _23, _33, _43,
				_14, _24, _34, _44
			};
		}

		static constexpr mat4 scale(T sx, T sy, T sz) {
			return mat4{
				sx, 0, 0, 0,
				0, sy, 0, 0,
				0, 0, sz, 0,
				0, 0, 0, T(1)
			};
		}

		static constexpr mat4 translate(T tx, T ty, T tz) {
			return mat4{
				T(1), 0, 0, tx,
				0, T(1), 0, ty,
				0, 0, T(1), tz,
				0, 0, 0, T(1)
			};
		}

		static constexpr mat4 zrotation(float r) {
			return mat4{
				cosf(r), sinf(r), 0, 0,
				-sinf(r), cosf(r), 0, 0,
				0, 0, T(1), 0,
				0, 0, 0, T(1)
			};
		}

		template<typename = std::enable_if<std::is_floating_point_v<T>>>
		static constexpr mat4 ortho(T left, T right, T top, T bottom, T _near, T _far)
		{
			return mat4{
				T(2) / (right - left), 0, 0, -(right + left) / (right - left),
				0, T(2) / (top - bottom), 0, -(top + bottom) / (top - bottom),
				0, 0, T(2) / (_far - _near), -(_far + _near) / (_far - _near),
				0, 0, 0, T(1)
			};
		}

		template<typename = std::enable_if<std::is_floating_point_v<T>>>
		static constexpr mat4 perspective(T fovy, T aspect, T zNear, T zFar)
		{
			auto fovy_rads = (fovy * M_PI) / T(180);
			auto f = tan(M_PI_2 - (fovy_rads / T(2))); // cotangent(fovy/2)
			return mat4{
				f / aspect, 0, 0, 0,
				0, f, 0, 0,
				0, 0, (zFar + zNear) / (zNear - zFar), (T(2) * zFar * zNear) / (zNear - zFar),
				0, 0, T(-1), 0
			};
		}
		
		constexpr vector3<T> operator*(const vector3<T>& v)const {
			vector3<T> r;
			r.x = v.x * _11 + v.y * _12 + v.z * _13 + _14;
			r.y = v.x * _21 + v.y * _22 + v.z * _23 + _24;
			r.z = v.x * _31 + v.y * _32 + v.z * _33 + _34;
			return r;
		}

		constexpr mat4 operator*(const mat4& o)const {
			mat4 r;
			r._11 = _11 * o._11 + _12 * o._21 + _13 * o._31 + _14 * o._41;
			r._12 = _11 * o._12 + _12 * o._22 + _13 * o._32 + _14 * o._42;
			r._13 = _11 * o._13 + _12 * o._23 + _13 * o._33 + _14 * o._43;
			r._14 = _11 * o._14 + _12 * o._24 + _13 * o._34 + _14 * o._44;

			r._21 = _21 * o._11 + _22 * o._21 + _23 * o._31 + _24 * o._41;
			r._22 = _21 * o._12 + _22 * o._22 + _23 * o._32 + _24 * o._42;
			r._23 = _21 * o._13 + _22 * o._23 + _23 * o._33 + _24 * o._43;
			r._24 = _21 * o._14 + _22 * o._24 + _23 * o._34 + _24 * o._44;

			r._31 = _31 * o._11 + _32 * o._21 + _33 * o._31 + _34 * o._41;
			r._32 = _31 * o._12 + _32 * o._22 + _33 * o._32 + _34 * o._42;
			r._33 = _31 * o._13 + _32 * o._23 + _33 * o._33 + _34 * o._43;
			r._34 = _31 * o._14 + _32 * o._24 + _33 * o._34 + _34 * o._44;

			r._41 = _41 * o._11 + _42 * o._21 + _43 * o._31 + _44 * o._41;
			r._42 = _41 * o._12 + _42 * o._22 + _43 * o._32 + _44 * o._42;
			r._43 = _41 * o._13 + _42 * o._23 + _43 * o._33 + _44 * o._43;
			r._44 = _41 * o._14 + _42 * o._24 + _43 * o._34 + _44 * o._44;
			
			return r;
		}

		constexpr const mat4& operator*=(const mat4& o) {
			return (*this = *this * o);
		}
	};
#pragma pack(pop)
}

using mat4f = core::mat4<float>;
