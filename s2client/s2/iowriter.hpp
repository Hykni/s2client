#pragma once
#include <core/io/logger.hpp>

namespace s2 {
    class iowriter : core::IWriter {
    public:
        static void Attach() {
            static iowriter* _instance = nullptr;
            if (!_instance)
                _instance = new iowriter();

            core::set_writer(_instance);
        }

        bool write(string_view s) {
            const char* p = s.data();
            for (int i = 0; i < s.length(); i++) {
                const char* c = s.data() + i;
                if (*c == '^') {
                    int remaining = s.length() - i;
                    if (remaining > 1) {
                        char n = *(c + 1);
                        if (n >= '0' && n <= '9' && remaining > 3) {
                            // ...
                            char n2 = *(c + 2);
                            char n3 = *(c + 3);
                            if (n2 >= '0' && n2 <= '9' && n3 >= '0' && n3 <= '9') {
                                // todo: translate these color codes
                                core::impl::set_color(core::CON_DEF);
                                i += 3;
                                continue;
                            }
                        }
                        else {
                            int clr = -1;
                            switch (n) {
                            case 'c':
                            case 'C':
                                clr = core::CON_CYN;
                                break;
                            case 'm':
                            case 'M':
                                clr = core::CON_MGN;
                                break;
                            case 'y':
                            case 'Y':
                                clr = core::CON_YLW;
                                break;
                            case 'k':
                            case 'K':
                                clr = 0;
                                break;
                            case 'r':
                            case 'R':
                                clr = core::CON_RED;
                                break;
                            case 'g':
                            case 'G':
                                clr = core::CON_GRN;
                                break;
                            case 'b':
                            case 'B':
                                clr = core::CON_BLU;
                                break;
                            case 'w':
                            case 'W':
                                clr = core::CON_WHT;
                                break;
                            }
                            if (clr >= 0) {
                                fwrite(p, 1, (int)(c - p), stdout);
                                core::impl::set_color((core::console_color)clr);
                                p = c + 2;
                                i += 1;
                                continue;
                            }
                        }
                    }
                }
            }

            fputs(p, stdout);
            core::impl::set_color(core::CON_DEF);
            return true;
        }
    };
}
