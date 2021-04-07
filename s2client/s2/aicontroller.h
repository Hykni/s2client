#pragma once

#include <core/prerequisites.hpp>

namespace s2 {
	class aicontroller {
	public:
		virtual ~aicontroller() = 0;

		virtual void think(class userclient* client) = 0;
	};

	class simpleaidefender : public aicontroller {
	private:
		enum class DefenderState {
			Patrolling,
			AttackingPlayer
		};
	public:
		~simpleaidefender();

		void think(class userclient* client);
	};
}
