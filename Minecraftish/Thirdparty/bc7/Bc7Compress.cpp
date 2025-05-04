// Bc7Compress.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "Bc7Core.h"
#include "Bc7Tables.h"
#include "Bc7Pca.h"
#include "Worker.h"
#include "Bc7Compress.h"

#include <chrono>
#include <string>
#include <vector>

static ALWAYS_INLINED int Max(int x, int y) noexcept
{
	return (x > y) ? x : y;
}

static INLINED void FullMask(uint8_t* mask_agrb, int stride, int src_w, int src_h) noexcept
{
	for (int y = 0; y < src_h; y++)
	{
		int* w = (int*)&mask_agrb[y * stride];

		for (int x = 0; x < src_w; x++)
		{
			w[x] = -1;
		}
	}
}

static INLINED void ComputeAlphaMaskWithOutline(uint8_t* mask_agrb, const uint8_t* src_bgra, int stride, int src_w, int src_h, int radius)
{
	if (radius < 0)
		return;

	const int full_w = radius + src_w + radius;

	// row buffer to accumulate Y sum
	int* buf = new int[full_w + size_t(15)];
	memset(buf, 0, full_w * sizeof(int));

	// add kernel rows except last
	for (int y = 0; y < radius; y++)
	{
		const uint32_t* __restrict r = reinterpret_cast<const uint32_t*>(&src_bgra[y * stride]);

		for (int i = 0; i < src_w; i++)
		{
			const uint8_t v = uint8_t((r[i] & 0xFF000000u) != 0);
			buf[radius + i] += v;
		}
	}

	for (int y = 0; y < src_h; y++)
	{
		// add last kernel row
		if (y + radius < src_h)
		{
			const uint32_t* __restrict r = reinterpret_cast<const uint32_t*>(&src_bgra[(y + radius) * stride]);

			for (int i = 0; i < src_w; i++)
			{
				const uint8_t v = uint8_t((r[i] & 0xFF000000u) != 0);
				buf[radius + i] += v;
			}
		}

		int32_t* __restrict p = reinterpret_cast<int32_t*>(&mask_agrb[y * stride]);

		// filter current row
		int sum = 0;
		for (int i = 0; i < radius + radius; i++)
		{
			sum -= buf[i];
		}
		for (int i = 0; i < src_w; i++)
		{
			// include last kernel pixel
			sum -= buf[radius + radius + i];

			const int32_t v = (sum < 0) ? -1 : 0;

			// remove first kernel pixel
			sum += buf[i];

			p[i] = v | 0xFF;
		}

		// subtract first kernel row
		if (y >= radius)
		{
			const uint32_t* __restrict r = reinterpret_cast<const uint32_t*>(&src_bgra[(y - radius) * stride]);

			for (int i = 0; i < src_w; i++)
			{
				const uint8_t v = uint8_t((r[i] & 0xFF000000u) != 0);
				buf[radius + i] -= v;
			}
		}
	}

	delete[] buf;
}

static void PackTexture(const IBc7Core& bc7Core, uint8_t* dst_bc7, uint8_t* src_bgra, uint8_t* mask_agrb, int stride, int src_w, int src_h, PBlockKernel blockKernel, size_t block_size, int64_t& pErrorAlpha, int64_t& pErrorColor, BlockSSIM& pssim)
{
	auto start = std::chrono::high_resolution_clock::now();

	ProcessTexture(dst_bc7, src_bgra, mask_agrb, stride, src_w, src_h, blockKernel, block_size, pErrorAlpha, pErrorColor, pssim);

	auto finish = std::chrono::high_resolution_clock::now();

	if (blockKernel == bc7Core.pCompress)
	{
		int pixels = src_h * src_w;

		int span = Max((int)std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count(), 1);

		int kpx_s = pixels / span;

		PRINTF("    Compressed %d blocks, elapsed %i ms, throughput %d.%03d Mpx/s", pixels >> 4, span, kpx_s / 1000, kpx_s % 1000);

#if defined(OPTION_COUNTERS)
		CompressStatistics();
#endif
	}
}

static INLINED void VisualizePartitionsGRB(uint8_t* dst_bc7, int size)
{
	for (int i = 0; i < size; i += 16)
	{
		uint64_t data0 = *(const uint64_t*)&dst_bc7[i + 0];
		uint64_t data1 = *(const uint64_t*)&dst_bc7[i + 8];

		if (data0 & 0xF)
		{
			if (data0 & 3)
			{
				if (data0 & 1)
				{
					data0 &= (1u << (4 + 1)) - 1u;

					data0 |=
						(0xFuLL << 29) + // G0
						(0xFuLL << 13); // R2

					data1 = 
						(0xFuLL << 5); // B4
				}
				else
				{
					data0 &= (1u << (6 + 2)) - 1u;

					data0 |=
						(0x3FuLL << 32) + // G0
						(0x3FuLL << 20); // R2

					data1 = 0;
				}
			}
			else
			{
				if (data0 & 4)
				{
					data0 &= (1u << (6 + 3)) - 1u;

					data0 |=
						(0x1FuLL << 39) + // G0
						(0x1FuLL << 19); // R2

					data1 =
						(0x1FuLL << 25); // B4
				}
				else
				{
					data0 &= (1u << (6 + 4)) - 1u;

					data0 |=
						(0x7FuLL << 38) + // G0
						(0x7FuLL << 24); // R2

					data1 = 0;
				}
			}
		}
		else if (data0 & 0xF0)
		{
			if (data0 & 0x30)
			{
				if (data0 & 0x10)
				{
					data0 &= (1u << 5) - 1u;

					data0 |=
						(0x1FuLL << 18) + // G0
						(0x1FuLL << 8) + // R0
						(0x3FuLL << 38); // A0

					data1 = 0;
				}
				else
				{
					data0 &= (1u << 6) - 1u;

					data0 |=
						(0x7FuLL << 22) + // G0
						(0x7FuLL << 8) + // R0
						(0xFFuLL << 50); // A0

					data1 = 0;
				}
			}
			else
			{
				if (data0 & 0x40)
				{
					if ((data0 == 0x40) && (data1 == 0))
						continue;

					data0 &= (1u << 7) - 1u;

					// data1 = InsertZeroBit(data1 >> 1, 3);
					data1 = (data1 & ~0xFuLL) + ((data1 & 0xFuLL) >> 1);

					if (((data1 >> 2) ^ data1) & 0x3333333333333333uLL)
					{
						data0 |=
							(0x50uLL << 21) + // G0
							(0x50uLL << 7) + // R0
							(0x50uLL << 35) + // B0
							(0x7FuLL << 49); // A0
					}
					else
					{
						data0 |=
							(0x30uLL << 21) + // G0
							(0x30uLL << 7) + // R0
							(0x30uLL << 35) + // B0
							(0x7FuLL << 49); // A0
					}

					data1 = 0;
				}
				else
				{
					data0 &= (1u << (6 + 8)) - 1u;

					data0 |=
						(0x1FuLL << 34) + // G0
						(0x1FuLL << 24); // R2

					data1 =
						(0x1FuLL << 10) + // A0
						(0x1FuLL << 20); // A2
				}
			}
		}

		*(uint64_t*)&dst_bc7[i + 0] = data0;
		*(uint64_t*)&dst_bc7[i + 8] = data1;
	}
}

bool GetBc7Core(void* bc7Core);

BC7::Bc7Result BC7::Compress::Bc7Compress(uint8_t* src_image_bgra, int src_image_w, int src_image_h)
{

	IBc7Core bc7Core{};
	if (!GetBc7Core(&bc7Core))
	{
		PRINTF("Unsupported CPU");
		exit(-9);
	}

	bool doDraft = true;
	bool doNormal = true;
	bool doSlow = false;

	bool flip = true;
	bool mask = true;
	int border = 1;
	bool linearData = false;

	doDraft = true;
	doNormal = false;
	doSlow = false;

	int src_texture_w = (Max(4, src_image_w) + 3) & ~3;
	int src_texture_h = (Max(4, src_image_h) + 3) & ~3;

	if (Max(src_texture_w, src_texture_h) > 16384)
	{
		delete[] src_image_bgra;
		exit(-10);
	}

	int c = 4;
	int src_image_stride = src_image_w * c;
	int src_texture_stride = src_texture_w * c;

	uint8_t* src_texture_bgra = new uint8_t[src_texture_h * src_texture_stride];

	for (int i = 0; i < src_image_h; i++)
	{
		memcpy(&src_texture_bgra[i * src_texture_stride], &src_image_bgra[i * src_image_stride], src_image_stride);

		for (int j = src_image_stride; j < src_texture_stride; j += c)
		{
			memcpy(&src_texture_bgra[i * src_texture_stride + j], &src_image_bgra[i * src_image_stride + src_image_stride - c], c);
		}
	}

	for (int i = src_image_h; i < src_texture_h; i++)
	{
		memcpy(&src_texture_bgra[i * src_texture_stride], &src_texture_bgra[(src_image_h - 1) * src_texture_stride], src_texture_stride);
	}

	//PRINTF("  Image %dx%d, Texture %dx%d", src_image_w, src_image_h, src_texture_w, src_texture_h);

	src_image_bgra = nullptr;

	uint8_t* dst_texture_bgra = new uint8_t[src_texture_h * src_texture_stride];

	memcpy(dst_texture_bgra, src_texture_bgra, src_texture_h* src_texture_stride);

	int Size = src_texture_h * src_texture_w;

	bc7Core.pInitTables(doDraft, doNormal, doSlow, linearData);

	uint8_t* mask_agrb = new uint8_t[src_texture_h * src_texture_stride];

	if (mask)
	{
		ComputeAlphaMaskWithOutline(mask_agrb, src_texture_bgra, src_texture_stride, src_texture_w, src_texture_h, border);
	}
	else
	{
		FullMask(mask_agrb, src_texture_stride, src_texture_w, src_texture_h);
	}

	uint8_t* dst_bc7 = new uint8_t[Size];
	memset(dst_bc7, 0, Size);

	//LoadBc7(dst_name, sizeof(head), dst_bc7, Size);

	int64_t mse_alpha = 0;
	int64_t mse_color = 0;
	BlockSSIM ssim = BlockSSIM(0, 0);
	PackTexture(bc7Core, dst_bc7, src_texture_bgra, mask_agrb, src_texture_stride, src_texture_w, src_texture_h, bc7Core.pCompress, 16, mse_alpha, mse_color, ssim);

	int pixels = src_texture_h * src_texture_w;

#ifdef _DEBUG
	if (mse_alpha > 0)
	{
		const double weightAlpha = bc7Core.pGetWeightAlpha();

		PRINTF("      SubTexture A qMSE = %.1f, qPSNR = %f, SSIM_4x4 = %.8f",
			(1.0 / weightAlpha) * mse_alpha / pixels,
			10.0 * log((255.0 * 255.0) * weightAlpha * pixels / mse_alpha) / log(10.0),
			ssim.Alpha * 16.0 / pixels);
	}
	else
	{
		PRINTF("      Whole A");
	}

	if (mse_color > 0)
	{
		const double weightColor = bc7Core.pGetWeightColor();

		PRINTF("      SubTexture RGB qMSE = %.1f, qPSNR = %f, wSSIM_4x4 = %.8f",
			(1.0 / weightColor) * mse_color / pixels,
			10.0 * log((255.0 * 255.0) * weightColor * pixels / mse_color) / log(10.0),
			ssim.Color * 16.0 / pixels);
	}
	else
	{
		PRINTF("      Whole RGB");
	}
#endif

	Bc7Result result{};
	result.buffer = dst_bc7;
	result.size = Size;
	result.image_w = src_image_w;
	result.image_h = src_image_h;

	delete[] mask_agrb;

	delete[] dst_texture_bgra;
	delete[] src_texture_bgra;

	return result;
}
