#pragma once

#include <core/prerequisites.hpp>

namespace s2 {
	namespace consts {
		constexpr string_view ConnectMagic = "S2_K2_CONNECT";
		constexpr string_view ClientVersion = "2.1.1.1";
		constexpr uint8_t ProtocolVersion = 1;
		constexpr uint32_t SeqUnreliable = 0xF197DE9A;
	}
}
