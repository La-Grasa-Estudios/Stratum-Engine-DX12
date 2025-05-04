#pragma once

#include "znmsp.h"

#include <glm/ext.hpp>

BEGIN_ENGINE

constexpr int TEXTURE_TYPE_ALBEDO = 0;
constexpr int TEXTURE_TYPE_NORMAL = 1;
constexpr int TEXTURE_TYPE_ROUGHNESS = 2;
constexpr int TEXTURE_TYPE_METALNESS = 3;
constexpr int TEXTURE_TYPE_AO = 4;
constexpr int TEXTURE_TYPE_NORMAL2CH = 5;

constexpr int DXT_FORMAT_BC7 = 0;
constexpr int DXT_FORMAT_BC5 = 1;
constexpr int DXT_FORMAT_BC5_NORM = 2;

constexpr int TEXTURE_MAGIC = 0x58455443;
constexpr int MATERIAL_MAGIC = 0x54414D43;

constexpr int MATERIAL_VER_UVDISP = 0x2;

struct texturedxt_t
{
	int dxt_format;
	int dxt_size;
	int dxt_width;
	int dxt_height;
	int dxt_transparent;
	int nbChannels;
	int offset;
};

struct textureheader_t
{
	int magic = 0x58455443; // CTEX
	char checksum[64];
	int type;
	int dataLength;
	int mip_count;
};

struct materialheader_t
{
	int32_t magic; // CMAT
	char checksum[64];
	glm::vec4 albedo;
	float metalness;
	float roughness;
	int32_t version;
	float padding;
	bool cast_shadows;
	bool two_sided;
	int32_t blend_mode;
	int32_t texture_count;
};

struct materialheaderuv_t
{
	glm::vec2 uvOffset;
	glm::vec2 uvSpeed;
};

END_ENGINE