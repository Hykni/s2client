#pragma once

#include <core/prerequisites.hpp>
#include <core/math/vector3.hpp>
#include <core/io/logger.hpp>
#include <core/utils/fnv.hpp>
#include <network/packet.hpp>
#include <s2/typeregistry.hpp>

using network::packet;

namespace s2 {
	class entity {
		int mEntId = -1;
		int mEntType = -1;
		const typeinfo* mTypeInfo = nullptr;
	public:
		string_view typname()const;
		int type()const;
		int id()const;
		const typeinfo& typevec()const;

		// Use same as s2 typenames
		int m_uiNetFlags = 0;
		vector3f m_v3Position;
		vector3f m_v3Angles;
		int m_iClientNum = 0;
		int m_iClientNumber = 0;
		int m_iAccountID = 0;
		uint16_t m_uiPlayerEntityIndex = 0;
		string m_sName;
		uint16_t m_unPing = 0;

		uint8_t m_yStatus = 0;
		enum class Status : uint8_t {
			Alive = 0,
			Spawning = 1,
			Dead = 3,
			Dormant2 = 5,
		};

		uint32_t m_uiGamePhase = 0;
		enum class GamePhase : uint32_t {
			Warmup = 6
		};

		uint8_t m_iTeam = 0;
		int m_iTeamID = 0;
		uint16_t m_uiBaseBuildingIndex = 0;
		float m_fHealth = 0.f;
		// ------------------------

		bool killed = false;

		bool alive()const { return m_yStatus == 0; }
		bool dormant()const { return m_yStatus == 1 || m_yStatus == 5; }

		entity(int entid, int type);
		entity() { __debugbreak(); }

		std::pair<vector<varinfo>,vector<uint8_t>> decodefieldsbitarray(packet& pkt, unsigned int idx=27)const;

		template<typename T>
		void updatefield(string_view name, const T& value) {
			//if (typname().starts_with("Building") || typname().starts_with("Entity_")) {
			//	if constexpr (std::is_same_v<T, string> || std::is_same_v<T, string_view>)
			//		core::info("Updating %s::%s (\"%s\")\n", mTypeInfo.name, name, value);
			//	else if constexpr (std::is_same_v<T, float>)
			//		core::info("Updating %s::%s (%.2f)\n", mTypeInfo.name, name, value);
			//	else if constexpr (std::is_integral_v<T>)
			//		core::info("Updating %s::%s (%d)\n", mTypeInfo.name, name, value);
			//}
			//core::info("Updating %s::%s\n", mTypeInfo.name, name);
			auto hash = fnv::hash_runtime(name.data());
			switch (hash) {
			case FNV("m_uiNetFlags"):
				if constexpr (std::is_integral_v<T> && sizeof(T) <= 4)
					m_uiNetFlags = value;
				break;
			case FNV("m_v3Position"):
				if constexpr (std::is_same_v<T, vector3f>)
					m_v3Position = value;
				break;
			case FNV("m_v3Angles"):
				if constexpr (std::is_same_v<T, vector3f>)
					m_v3Angles = value;
				break;
			case FNV("m_iClientNum"):
				if constexpr (std::is_integral_v<T> && sizeof(T) <= 4)
					m_iClientNum = value;
				break;
			case FNV("m_iClientNumber"):
				if constexpr (std::is_integral_v<T> && sizeof(T) <= 4)
					m_iClientNumber = value;
				break;
			case FNV("m_iAccountID"):
				if constexpr (std::is_integral_v<T> && sizeof(T) <= 4)
					m_iAccountID = value;
				break;
			case FNV("m_uiPlayerEntityIndex"):
				if constexpr (std::is_integral_v<T> && sizeof(T) <= 2)
					m_uiPlayerEntityIndex = value;
				break;
			case FNV("m_sName"):
				if constexpr (std::is_same_v<T, string> || std::is_same_v<T, string_view>)
					m_sName = value;
				break;
			case FNV("m_unPing"):
				if constexpr (std::is_integral_v<T> && sizeof(T) <= 2)
					m_unPing = value;
				break;
			case FNV("m_yStatus"):
				if constexpr (std::is_integral_v<T> && sizeof(T) <= 1)
					m_yStatus = value;
				break;
			case FNV("m_uiGamePhase"):
				if constexpr (std::is_integral_v<T> && sizeof(T) <= 4)
					m_uiGamePhase = value;
				break;
			case FNV("m_iTeam"):
				if constexpr (std::is_integral_v<T> && sizeof(T) <= 1)
					m_iTeam = value;
				break;
			case FNV("m_iTeamID"):
				if constexpr (std::is_integral_v<T> && sizeof(T) <= 4)
					m_iTeamID = value;
				break;
			case FNV("m_uiBaseBuildingIndex"):
				if constexpr (std::is_integral_v<T> && sizeof(T) <= 2)
					m_uiBaseBuildingIndex = value;
				break;
			case FNV("m_fHealth"):
				if constexpr(std::is_floating_point_v<T>)
					m_fHealth = value;
				break;
			//default:
				//if constexpr(std::is_same_v<T,string> || std::is_same_v<T,string_view>)
				//	core::warning("Discarded %s::%s (\"%s\")\n", mTypeInfo.name, name, value);
				//else
				//	core::warning("Discarded %s::%s\n", mTypeInfo.name, name);
			}
		}
	};
}
