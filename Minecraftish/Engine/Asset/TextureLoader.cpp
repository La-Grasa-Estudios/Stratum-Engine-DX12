#include "TextureLoader.h"
#include "AssetMaterial.h"

#include <stb_image.h>

#include <vector>
#include "zlib/izlibstream.h"
#include "VFS/ZVFS.h"
#include <Core/Logger.h>


using namespace ENGINE_NAMESPACE;
using namespace Render;

struct ThreadedTextureIO {

	std::string path;
	bool inLocalFileSystem = false;
	uint8_t* data = NULL;
	int width = 0;
	int height = 0;
	int nbChannels = 0;
	bool fromStbi = false;
	bool isTransparent = false;

};

const uint32_t MinX = 16;
const uint32_t MinY = MinX;

ImageDescription CreateFromRawData(texturedxt_t* headers, int mipCount, PakStream pStream, float fLod)
{
	std::vector<ImageResourceData> ImageSubresourceData;
	int nrChannels = headers[0].nbChannels;
	int width = headers[0].dxt_width;
	int height = headers[0].dxt_height;
	

	int mipOffset = (int)glm::floor(mipCount * fLod);
	uint32_t totalMipCount = 0;
	uint32_t maxWidth = 0;
	uint32_t maxHeight = 0;
	uint32_t startMip = glm::clamp(mipOffset, 0, mipCount - 1);
	bool tooSmall = !(headers[startMip].dxt_width >= MinX && headers[startMip].dxt_height >= MinY);

	while (tooSmall && startMip != 0)
	{
		uint32_t w = headers[startMip].dxt_width;
		uint32_t h = headers[startMip].dxt_height;

		tooSmall = (w < MinX || h < MinY);

		if (tooSmall && startMip != 0)
		{
			startMip--;
		}
	}

	for (int i = startMip; i < mipCount; i++)
	{
		ENGINE_NAMESPACE::texturedxt_t header = headers[i];
		uint8_t* data = new uint8_t[header.dxt_size];

		totalMipCount++;

		maxWidth = glm::max(maxWidth, (uint32_t)header.dxt_width);
		maxHeight = glm::max(maxHeight, (uint32_t)header.dxt_height);

		pStream->AcquireLock();
		pStream->seekg(header.offset);
		zlib::izlibstream in(*pStream->GetStream());
		in.read((char*)data, header.dxt_size);
		in.close();
		pStream->ReleaseLock();

		if (header.nbChannels == 3)
		{
			uint8_t* rgba = new uint8_t[header.dxt_width * header.dxt_height * 4];
			for (int i = 0; i < header.dxt_width * header.dxt_height; i++)
			{
				rgba[i * 4 + 0] = data[i * 3 + 0];
				rgba[i * 4 + 1] = data[i * 3 + 1];
				rgba[i * 4 + 2] = data[i * 3 + 2];
				rgba[i * 4 + 3] = 255;
			}
			delete[] data;
			data = rgba;
			nrChannels = 4;
			header.nbChannels = 4;
		}

		ImageSubresourceData.push_back({ data, (uint32_t)header.dxt_width * (uint32_t)header.nbChannels });
	}

	ImageFormat format = ImageFormat::RGBA8_UNORM;

	if (nrChannels == 1)
	{
		format = ImageFormat::R8_UNORM;
	}
	if (nrChannels == 2)
	{
		format = ImageFormat::RG8_UNORM;
	}

	ImageDescription imageDesc{};

	imageDesc.Width = maxWidth;
	imageDesc.Height = maxHeight;
	imageDesc.MipLevels = totalMipCount;
	imageDesc.ArraySize = 1;
	imageDesc.Format = format;

	imageDesc.DefaultData = ImageSubresourceData;

	return imageDesc;

}

ImageDescription CreateFromCTEX(PakStream stream, TextureCreationCTEX ctex, const ITextureDescription& desc, ThreadedTextureIO& io, float fLod)
{

	int mipCount = ctex.header.mip_count;

	int baseOffset = sizeof(ENGINE_NAMESPACE::textureheader_t);

	std::vector<ENGINE_NAMESPACE::texturedxt_t> mipHeaders;
	stream->seekg(baseOffset);

	for (int i = 0; i < mipCount; i++)
	{
		ENGINE_NAMESPACE::texturedxt_t mipHeader;
		stream->read((char*)&mipHeader, sizeof(ENGINE_NAMESPACE::texturedxt_t));
		mipHeaders.push_back(mipHeader);
	}

	io.isTransparent = mipHeaders[0].dxt_transparent;

	if (mipHeaders[0].dxt_format == -1)
	{
		return CreateFromRawData(mipHeaders.data(), mipCount, stream, fLod);
	}

	std::vector<ImageResourceData> ImageSubresourceData;

	int mipOffset = (int)glm::floor(mipCount * fLod);
	uint32_t totalMipCount = 0;
	uint32_t maxWidth = 0;
	uint32_t maxHeight = 0;
	uint32_t startMip = glm::clamp(mipOffset, 0, mipCount - 1);
	bool tooSmall = !(mipHeaders[startMip].dxt_width >= MinX && mipHeaders[startMip].dxt_height >= MinY);

	while (tooSmall && startMip != 0)
	{
		uint32_t w = mipHeaders[startMip].dxt_width;
		uint32_t h = mipHeaders[startMip].dxt_height;

		tooSmall = (w < MinX || h < MinY);

		if (tooSmall && startMip != 0)
		{
			startMip--;
		}
	}

	for (int i = startMip; i < mipCount; i++)
	{
		ENGINE_NAMESPACE::texturedxt_t header = mipHeaders[i];

		uint8_t* data = new uint8_t[header.dxt_size];

		maxWidth = glm::max(maxWidth, (uint32_t)header.dxt_width);
		maxHeight = glm::max(maxHeight, (uint32_t)header.dxt_height);

		stream->AcquireLock();
		stream->seekg(header.offset);
		zlib::izlibstream in(*stream->GetStream());
		in.buf.is_encrypted = stream->is_encrypted();
		in.buf.g_pointer = stream->tellg();
		in.read((char*)data, header.dxt_size);
		in.close();
		stream->ReleaseLock();

		ImageSubresourceData.push_back({ data, (uint32_t)header.dxt_width * 4 });

		totalMipCount++;
	}

	ImageDescription imageDesc{};

	imageDesc.Width = maxWidth;
	imageDesc.Height = maxHeight;
	imageDesc.MipLevels = totalMipCount;
	imageDesc.ArraySize = 1;
	imageDesc.Format = ImageFormat::BC7_UNORM;

	imageDesc.DefaultData = ImageSubresourceData;

	return imageDesc;
}

ImageDescription CreateFromBC7Data(TextureCreationBC7 bc7, const ITextureDescription& desc)
{
	uint32_t* bc7head = bc7.bc7Header;
	bool bc7Capable = false;
	bool bc7File = false;
	bool bc7Encoded = false;

	uint32_t dataOffset = 0;
	uint32_t width = bc7head[1];
	uint32_t height = bc7head[2];

	void* data = bc7.pSysMem;

	bc7Capable = false;

	if (bc7head[0] == 0x375A4243) {
		if (bc7head[5] == 0) {
			dataOffset = sizeof(bc7.bc7Header);
		}
		else {
			bc7Encoded = true;
		}
		bc7File = true;
		bc7Capable = false;
	}

	int lSize = bc7head[6];

	uint8_t* buffer = new uint8_t[lSize];
	uint8_t* lodbuffer = NULL;

	unsigned char* dataPtr = (unsigned char*)data + 36;

	uint32_t mainSize = bc7head[6];
	uint32_t lodSize = bc7head[7];

	memcpy(buffer, dataPtr, mainSize);
	lodbuffer = new uint8_t[lodSize];
	memcpy(lodbuffer, dataPtr + mainSize, lodSize);

	uint32_t lodReduction = bc7head[4];

	ImageDescription imageDesc{};

	imageDesc.Width = width;
	imageDesc.Height = height;
	imageDesc.MipLevels = 1;
	imageDesc.ArraySize = 1;
	imageDesc.Format = ImageFormat::BC7_UNORM;

	imageDesc.DefaultData = { ImageResourceData(buffer, width * 4) };

	if (lodbuffer)
	{
		imageDesc.DefaultData.push_back(ImageResourceData(lodbuffer, width / lodReduction * 4));
	}

	return imageDesc;
}

ImageDescription CreateFromRGBAData(TextureCreationRGBA rgba, const ITextureDescription& desc)
{
	bool noLod = true;

	uint32_t dataOffset = 0;

	if ((desc.Width < 256 || desc.Height < 256) || !(desc.Width % 2 == 0 && desc.Height % 2 == 0) || desc.Streamable)
	{
		noLod = true;
	}

	int nrChannels = (rgba.Format == TextureFormat::RGBA8 ? 4 : rgba.Format == TextureFormat::RGB8 ? 3 : rgba.Format == TextureFormat::R8 ? 1 : 2);

	int lodReduction = 2;

	while (desc.Width / lodReduction > lodReduction && desc.Height / lodReduction > lodReduction && lodReduction < 4) {
		lodReduction += 2;
	}

	int32_t texelSize = 4;
	if (rgba.Format == TextureFormat::R8) texelSize = 1;
	if (rgba.Format == TextureFormat::RG8) texelSize = 2;
	int lSize = desc.Width * desc.Height * texelSize;
	int lodSize = (desc.Width / lodReduction) * (desc.Height / lodReduction) * texelSize;
	int iter = desc.Width * desc.Height;
	int iterLod = lSize / lodReduction;
	if (desc.Width < lodReduction || desc.Height < lodReduction) {
		lodSize = lSize;
		noLod = true;
	}

	uint8_t* buffer = (uint8_t*)malloc(lSize);
	uint8_t* lodbuffer = NULL;

	void* data = rgba.pSysMem;

	uint8_t* dataPtr = (uint8_t*)data;
	data = dataPtr + dataOffset;

	if (!noLod) {
		lodbuffer = (uint8_t*)malloc(lodSize);
		memset(lodbuffer, 0, lodSize);
	}

	memset(buffer, 0, lSize);

	if (rgba.Format == TextureFormat::RGBA8 || rgba.Format == TextureFormat::R8 || rgba.Format == TextureFormat::RG8) {
		memcpy(buffer, dataPtr, lSize);
	}

	for (int i = 0; i < iter; i++) {

		if (rgba.Format == TextureFormat::RGB8) {

			uint8_t r = dataPtr[i * 3 + 0];
			uint8_t g = dataPtr[i * 3 + 1];
			uint8_t b = dataPtr[i * 3 + 2];

			buffer[i * 4 + 0] = r;
			buffer[i * 4 + 1] = g;
			buffer[i * 4 + 2] = b;
			buffer[i * 4 + 3] = 255U;
		}

	}

	if (desc.Width > lodReduction && desc.Height > lodReduction && !noLod) {
		for (int x = 0; x < desc.Width / lodReduction; x++) {
			for (int y = 0; y < desc.Height / lodReduction; y++) {

				uint32_t avgr = 0;
				uint32_t avgg = 0;
				uint32_t avgb = 0;
				uint32_t avga = 0;

				uint32_t div = 0;

				for (int ix = 0; ix < lodReduction; ix++) {
					for (int iy = 0; iy < lodReduction; iy++) {

						div++;

						int index = (x * lodReduction + ix) + (y * lodReduction + iy) * desc.Width;

						if (rgba.Format == TextureFormat::R8)
						{
							avgr += buffer[index];
							continue;
						}

						if (rgba.Format == TextureFormat::RG8)
						{
							avgr += buffer[index * 2 + 0];
							avgg += buffer[index * 2 + 1];
							continue;
						}

						avgr += buffer[index * 4 + 0];
						avgg += buffer[index * 4 + 1];
						avgb += buffer[index * 4 + 2];
						avga += buffer[index * 4 + 3];

					}
				}

				int index = x + y * (desc.Width / lodReduction);

				if (rgba.Format == TextureFormat::R8)
				{
					lodbuffer[index] = avgr / div;
					continue;
				}

				if (rgba.Format == TextureFormat::RG8)
				{
					lodbuffer[index * 2 + 0] = avgr / div;
					lodbuffer[index * 2 + 1] = avgg / div;
					continue;
				}

				lodbuffer[index * 4 + 0] = avgr / div;
				lodbuffer[index * 4 + 1] = avgg / div;
				lodbuffer[index * 4 + 2] = avgb / div;
				lodbuffer[index * 4 + 3] = avga / div;

			}
		}
	}
	else if (!noLod) {
		memcpy(lodbuffer, buffer, lSize);
	}

	ImageFormat format = ImageFormat::RGBA8_UNORM;

	if (rgba.Format == TextureFormat::R8)
	{
		format = ImageFormat::R8_UNORM;
	}
	if (rgba.Format == TextureFormat::RG8)
	{
		format = ImageFormat::RG8_UNORM;
	}

	ImageDescription imageDesc{};

	imageDesc.Width = desc.Width;
	imageDesc.Height = desc.Height;
	imageDesc.MipLevels = 1;
	imageDesc.ArraySize = 1;
	imageDesc.Format = format;

	imageDesc.DefaultData = { ImageResourceData(buffer, desc.Width * texelSize) };

	if (lodbuffer)
	{
		imageDesc.DefaultData.push_back(ImageResourceData(lodbuffer, desc.Width / lodReduction * texelSize));
	}

	return imageDesc;
}

uint8_t* Load(std::string_view path, int& width, int& height, int& nrChannels, bool& isBc7, uint32_t bc7head[9], bool _inLocalFilesystem) {

	using namespace ENGINE_NAMESPACE;
	using namespace Render;

	if (!ZVFS::Exists(path.data())) return 0;

	if (path.ends_with("ctex")) return 0;

	RefBinaryStream in = ZVFS::GetFile(path.data());

	memcpy(bc7head, in->As<uint32_t>(), sizeof(uint32_t) * 9);

	unsigned char* data;

	if (bc7head[0] == 0x375A4243) {

		isBc7 = true;

		data = new unsigned char[in->Size()];
		memcpy(data, in->As<uint8_t>(), in->Size());

		width = bc7head[1];
		height = bc7head[2];
		nrChannels = bc7head[3];

	}
	else {
		if (_inLocalFilesystem) {
			data = stbi_load_from_memory(in->As<stbi_uc>(), in->Size(), &width, &height, &nrChannels, 0);

			if (path.find(".dds") != std::string::npos)
			{
				
			}

		}
		else {
			data = stbi_load(path.data(), &width, &height, &nrChannels, 4);
		}
	}
	return data;
}

Ref<ImageResource> TextureLoader::LoadFileToImage(const std::string& path, bool* bIsTransparent, float fRequestedLevelOfDetail)
{

	//Logger::LogInfo("Loading {}", path);

	ThreadedTextureIO TextureIO{};

	TextureIO.path = path;

	if (TextureIO.path.ends_with("ctex"))
	{

		//Logger::LogInfo("CTEX {}", path);

		PakStream stream = ZVFS::GetFileStream(TextureIO.path.c_str());

		ITextureDescription desc{};
		desc.Type = TextureCreationType::CTEX;

		//memcpy(&desc.CTEX.header, stream->As<char*>(), sizeof(textureheader_t));

		stream->read(&desc.CTEX.header, sizeof(textureheader_t));

		//desc.CTEX.pStream = stream->Stream();

		ImageDescription imageDesc = CreateFromCTEX(stream, desc.CTEX, desc, TextureIO, 1.0f - glm::clamp(fRequestedLevelOfDetail, 0.0f, 1.0f));

		imageDesc.Immutable = true;

		Ref<ImageResource> image = CreateRef<ImageResource>(imageDesc);

		for (int i = 0; i < imageDesc.DefaultData.size(); i++)
		{
			delete[](uint8_t*)imageDesc.DefaultData[i].pSysMem;
		}

		*bIsTransparent = TextureIO.isTransparent;

		return image;
	}

	int width, height;

	bool _inLocalFilesystem = true;

	int nrChannels;

	uint32_t bc7head[9]{};
	bool isBc7 = false;

	unsigned char* data = Load(TextureIO.path, width, height, nrChannels, isBc7, bc7head, _inLocalFilesystem);

	TextureIO.isTransparent = false;

	if (isBc7) {

		if (bc7head[8]) {
			TextureIO.isTransparent = true;
		}

		TextureIO.isTransparent = true;
	}
	else {
		for (int i = 0; i < width * height && nrChannels == 4 && !TextureIO.isTransparent && !isBc7; i++) {
			unsigned char alpha = ((char*)data)[i * 4 + 3];
			if (alpha <= 250) {
				if (alpha >= 10) TextureIO.isTransparent = true;
			}
		}
	}

	if (!data || width < 0 || height < 0) {
		return NULL;
	}

	int Size = width * height;

	TextureIO.nbChannels = nrChannels;
	TextureIO.width = width;
	TextureIO.height = height;
	TextureIO.fromStbi = bc7head[0] != 0x375A4243;
	TextureIO.data = data;

	//Logger::LogInfo("Loading {}, Resolution {} X {}, nrChannels {}, bc7 {}", TextureIO.path, width, height, TextureIO.nbChannels, bc7head[0] != 0x375A4243);

	ImageDescription imageDesc{};

	bool CTEX = false;
	bool BC7 = false;
	bool RGBA = false;

	TextureFormat format = TextureFormat::RGBA8;

	if (nrChannels == 4) format = TextureFormat::RGBA8;
	if (nrChannels == 3) format = TextureFormat::RGB8;
	if (nrChannels == 2) format = TextureFormat::RG8;
	if (nrChannels == 1) {
		format = TextureFormat::R8;
	}

	ITextureDescription desc{};
	desc.RGBA.Format = format;
	desc.RGBA.pSysMem = data;
	desc.Width = width;
	desc.Height = height;

	if (!TextureIO.fromStbi)
	{
		memcpy(desc.BC7.bc7Header, data, sizeof(desc.BC7.bc7Header));
		desc.BC7.pSysMem = data;
		if (desc.BC7.bc7Header[0] == 0x375A4243) desc.Type = TextureCreationType::BC7;
	}

	switch (desc.Type)
	{
	case TextureCreationType::RGBA:
		imageDesc = CreateFromRGBAData(desc.RGBA, desc);
		RGBA = true;
		break;
	case TextureCreationType::BC7:
		imageDesc = CreateFromBC7Data(desc.BC7, desc);
		BC7 = true;
		break;
	case TextureCreationType::CTEX:
		break;
	default:
		break;
	}

	imageDesc.Immutable = true;
	Ref<ImageResource> image = CreateRef<ImageResource>(imageDesc);

	if (CTEX || BC7)
	{
		for (int i = 0; i < imageDesc.DefaultData.size(); i++)
		{
			delete[] (uint8_t*)imageDesc.DefaultData[i].pSysMem;
		}
	}
	else
	{
		for (int i = 0; i < imageDesc.DefaultData.size(); i++)
		{
			free(imageDesc.DefaultData[i].pSysMem);
		}
	}

	if (TextureIO.fromStbi) {
		stbi_image_free(data);
	}
	else {
		delete[] data;
	}

	*bIsTransparent = TextureIO.isTransparent;

    return image;
}
