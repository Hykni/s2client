#include "logger.hpp"

namespace core {
	namespace impl {
        IWriter* _customWriter = nullptr;
		void set_color(console_color clr) {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), clr);
		}
	}

}
