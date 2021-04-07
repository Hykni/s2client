#include "logger.hpp"

namespace core {
	namespace impl {
		void set_color(console_color clr) {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), clr);
		}
	}

}
