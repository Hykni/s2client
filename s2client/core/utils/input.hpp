#pragma once

#include <core/prerequisites.hpp>

namespace core {
	namespace input {
		bool iskeydown(int vkey) {
			return GetAsyncKeyState(vkey) & 0x8000;
		}
	}
}
