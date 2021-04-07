#pragma once

#include <core/prerequisites.hpp>
#include <core/utils/format.hpp>

#include <iostream>

namespace core {
	enum console_color {
		CON_DEF = 7,
		CON_BLU = 9,
		CON_GRN = 10,
		CON_CYN = 11,
		CON_RED = 12,
		CON_YLW = 14,
		CON_BLK = 112
	};
	namespace impl {
		void set_color(console_color clr);

		template<typename ...Args>
		void write(string_view fmt, Args&&... args) {
			if constexpr (sizeof...(args) == 0)
				fputs(fmt.data(), stdout);
			else
				fprintf(stdout, fmt.data(), fixfmtarg(args)...);
		}
	}

	static string inputline() {
		string line;
		line.resize(80);
		fgets(line.data(), 80, stdin);
		line.resize(strlen(line.data()));
		return line;
	}

	template<typename ...Args>
	void print(string_view fmt, Args&&... args) {
		impl::write(fmt, std::forward<Args>(args)...);
	}
	template<console_color clr, typename ...Args>
	void print(string_view fmt, Args&&... args) {
		impl::set_color(clr);
		impl::write(fmt, std::forward<Args>(args)...);
		impl::set_color(CON_DEF);
	}

	template<typename ...Args>
	void info(string_view fmt, Args&&... args) {
		print("[?] %s", format(fmt, std::forward<Args>(args)...).c_str());
	}

	template<typename ...Args>
	void warning(string_view fmt, Args&&... args) {
		print<CON_YLW>("[!] %s", format(fmt, std::forward<Args>(args)...).c_str());
	}

	template<typename ...Args>
	void error(string_view fmt, Args&&... args) {
		print<CON_RED>("[*] %s", format(fmt, std::forward<Args>(args)...).c_str());
		__debugbreak();
		abort();
	}

	template<int bsPerRow = 8>
	void hexdump(const uint8_t* bs, size_t length) {
		auto nrows = length / bsPerRow;
		auto excess = length % bsPerRow;
		for (unsigned int i = 0; i < nrows; i++) {
			auto row = &bs[i * bsPerRow];
			for (int j = 0; j < bsPerRow; j++) {
				print("%02X ", row[j]);
			}
			print("   ");
			for (int j = 0; j < bsPerRow; j++) {
				print("%c", std::isprint(row[j]) ? row[j] : '.');
			}
			print("\n");
		}
		if (excess > 0) {
			auto rowlast = &bs[nrows * bsPerRow];
			for (unsigned int i = 0; i < bsPerRow; i++) {
				if (i < excess)
					print("%02X ", rowlast[i]);
				else
					print("   ");
			}
			print("   ");
			for (unsigned int i = 0; i < excess; i++) {
				print("%c", std::isprint(rowlast[i]) ? rowlast[i] : '.');
			}
			print("\n");
		}
	}
};
