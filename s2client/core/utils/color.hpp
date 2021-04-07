#pragma once

#include <core/prerequisites.hpp>

namespace core {
	class color {
	public:
		float R, G, B, A;
		
		constexpr color()
			: R(0.f), G(0.f), B(0.f), A(1.f) { }
		constexpr color(uint32_t clr) // no alpha consideration
			: color(clr & 0xFF, (clr >> 8) & 0xFF, (clr >> 16) & 0xFF) { }
		constexpr color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
			: R(r / 255.f), G(g / 255.f), B(b / 255.f), A(a / 255.f) { }
		constexpr color(float r, float g, float b, float a)
			: R(r), G(g), B(b), A(a) { }
		constexpr color(const color& c) = default;
		constexpr color(color&& c)noexcept = default;

		color& operator=(const color& c) = default;

		constexpr color operator-(const color& c)const {
			return color(R - c.R, G - c.G, B - c.B, A - c.A);
		}
		constexpr color operator+(const color& c)const {
			return color(R + c.R, G + c.G, B + c.B, A + c.A);
		}
		constexpr color operator*(float mul)const {
			return color(R * mul, G * mul, B * mul, A * mul);
		}

		static constexpr color FromHSV(float hue, float saturation, float value) {
			auto f = [=](float n) -> auto {
				float k = fmod(n + hue / 60.f, 6.f);
				return value - value * saturation * std::max(0.f, std::min(k, std::min(4.f - k, 1.f)));
			};
			return color(f(5), f(3), f(1), 1.f);
		}
	};
}
namespace Color {
	static constexpr core::color White(255, 255, 255);
	static constexpr core::color Red(255, 65, 54);
	static constexpr core::color Brown(165, 42, 42);
	static constexpr core::color Lightgreen(144, 238, 144);
	static constexpr core::color Green(46, 204, 64);
	static constexpr core::color Forestgreen(34, 139, 34);
	static constexpr core::color Lime(0, 255, 0);
	static constexpr core::color Blue(0, 116, 217);
	static constexpr core::color Cyan(0, 255, 255);
	static constexpr core::color Steelblue(70, 130, 180);
	static constexpr core::color Black(0, 0, 0);
	static constexpr core::color Olive(61, 153, 112);
	static constexpr core::color Yellow(255, 220, 0);
	static constexpr core::color Orange(255, 133, 27);
	static constexpr core::color Gold(255, 215, 0);
	static constexpr core::color Magenta(255, 0, 255);
	static constexpr core::color Violet(128, 0, 128);
	static constexpr core::color Indigo(75, 0, 130);
}
