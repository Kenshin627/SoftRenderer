#pragma once
#include <array>
#include <cmath>
#include <cstdint>

namespace ColorSpace
{
	inline float DecodeSrgb(uint8_t value)
	{
		static const auto table = [] {
			std::array<float, 256> values{};
			for (std::size_t i = 0; i < values.size(); ++i)
			{
				float s = float(i) / 255.0f;
				values[i] = s <= 0.04045f ? s / 12.92f : std::pow((s + 0.055f) / 1.055f, 2.4f);
			}
			return values;
		}();
		return table[value];
	}

	inline float EncodeSrgb(float linear)
	{
		return linear <= 0.0031308f ? 12.92f * linear : 1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f;
	}
}
