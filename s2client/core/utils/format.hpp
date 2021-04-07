#pragma once

#include <core/prerequisites.hpp>

namespace core {
	namespace impl {
		template<typename T, typename t = std::remove_cvref_t<T>>
		auto fixfmtarg(T& arg) {
			if constexpr (std::is_same_v<t, std::string> || std::is_same_v<t, std::wstring>) {
				return arg.data();
			}
			else if constexpr (std::is_same_v<t, string_view>)
				return arg.data();
			else
				return std::forward<T>(arg);
		}
	}

	template<typename ...Args>
	string format(string_view fmt, Args&&... args) {
		string buffer;
		buffer.resize(snprintf(nullptr, 0, fmt.data(), impl::fixfmtarg(args)...));
		snprintf(buffer.data(), buffer.size() + 1, fmt.data(), impl::fixfmtarg(args)...);
		return buffer;
	}
}
