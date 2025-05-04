#pragma once

#include <stdint.h>

namespace BC7 {

	struct Bc7Result {
		int image_w;
		int image_h;
		uint8_t* buffer;
		uint32_t size;
	};

	class Compress {
		public:
		static Bc7Result Bc7Compress(uint8_t* srcBgra, int width, int height);
	};
}