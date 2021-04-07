#pragma once

#include <core/prerequisites.hpp>
#include <core/utils/format.hpp>

namespace core {
	template<typename T, std::size_t N, typename = std::enable_if_t<std::is_arithmetic_v<T>> >
	class vectorN {
		template<typename F, std::size_t... Is>
		vectorN<T, N> produce(const vectorN<T, N>& o, std::index_sequence<Is...>)const {
			return vectorN<T, N>{F{}(v[Is], o.v[Is])...};
		}
		template<typename F>
		vectorN<T, N> produce(const vectorN<T, N>& o)const {
			return produce<F>(o, std::make_index_sequence<N>{});
		}
		template<typename F, std::size_t... Is>
		vectorN<T, N> produce(const T& v, std::index_sequence<Is...>) {
			return vectorN<T, N>{F{}(v[Is], v)...};
		}
		template<typename F>
		vectorN<T, N> produce(const T& v) {
			return produce<F>(v, std::make_index_sequence<N>{});
		}
		template<typename F, std::size_t... Is>
		void apply(const vectorN<T, N>& o, std::index_sequence<Is...>) {
			v = { F{}(v[Is], o.v[Is])... };
		}
		template<typename F>
		void apply(const vectorN<T, N>& o) {
			return apply<F>(o, std::make_index_sequence<N>{});
		}
		template<typename F, std::size_t... Is>
		void apply(std::index_sequence<Is...>) {
			v = { F{}(v[Is])... };
		}
		template<typename F>
		void apply() {
			return apply<F>(std::make_index_sequence<N>{});
		}
	public:
		T v[N];

		vectorN() : v{ T(0) } { }
		template<typename... Args>
		vectorN(Args&&... args) : v{ args... } { }

		vectorN<T, N> operator+(const vectorN<T, N>& o)const {
			return produce<std::plus<T>>(o);
		}
		vectorN<T, N> operator-(const vectorN<T, N>& o)const {
			return produce<std::minus<T>>(o);
		}
		vectorN<T, N>& operator+=(const vectorN<T, N>& o) {
			apply<std::plus<T>>(o);
			return *this;
		}
		vectorN<T, N>& operator-=(const vectorN<T, N>& o) {
			apply<std::minus<T>>(o);
			return *this;
		}
		vectorN<T, N> operator*(T m)const {
			return produce<std::multiplies<T>>(m);
		}

		T dot(const vectorN<T, N>& o)const {
			T r = T(0);
			for (auto i = 0; i < N; i++)
				r += v[i] * o.v[i];
			return r;
		}
	};
}
