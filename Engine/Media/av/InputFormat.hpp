#pragma once

#include "Decoder.hpp"
#include "Packet.hpp"
#include "common.hpp"
#include "VFS/ZVFS.h"

namespace av
{

	class SimpleInputFormat : NoCopyable
	{

		explicit SimpleInputFormat(AVFormatContext* ic) noexcept
			: ic_(ic)
		{}

	public:

		static int ReadFunc(void* ptr, uint8_t* buf, int buf_size)
		{
			using namespace ENGINE_NAMESPACE;
			PakStreamImpl* pStream = reinterpret_cast<PakStreamImpl*>(ptr);
			size_t size = 0;
			if (!(size = pStream->read(buf, buf_size)))
			{
				return AVERROR_EOF;  // Let FFmpeg know that we have reached eof
			}
			//InfoLog("Requested " << buf_size << " Got " << size);
			return size;
		}
		// whence: SEEK_SET, SEEK_CUR, SEEK_END (like fseek) and AVSEEK_SIZE
		static int64_t SeekFunc(void* ptr, int64_t pos, int whence)
		{
			using namespace ENGINE_NAMESPACE;
			// Quelle Abfragen:
			PakStreamImpl* pStream = reinterpret_cast<PakStreamImpl*>(ptr);

			if (whence == AVSEEK_SIZE)
			{
				return -1;
			}
			pStream->seekg(pos, whence);
			// Return the new position:
			return pStream->tellg();
		}

		static Expected<Ptr<SimpleInputFormat>> create(std::string_view url, bool enableAudio = false) noexcept
		{
			AVFormatContext* ic = avformat_alloc_context();

			using namespace ENGINE_NAMESPACE;

			PakStream pakStream = ZVFS::GetFileStream(url.data());

			//auto err = avformat_open_input(&ic, url.data(), nullptr, nullptr);

			if (!pakStream)
				RETURN_AV_ERROR("Cannot open input '{}'", url);

			// Create internal Buffer for FFmpeg:
			const int iBufSize = 64 * 1024;
			uint8_t* pBuffer = new uint8_t[iBufSize + AVPROBE_PADDING_SIZE];
			memset(pBuffer, 0, iBufSize + AVPROBE_PADDING_SIZE);

			// Allocate the AVIOContext:
			// The fourth parameter (pStream) is a user parameter which will be passed to our callback functions
			AVIOContext* pIOCtx = avio_alloc_context(pBuffer, iBufSize,  // internal Buffer and its size
				0,                  // bWriteable (1=true,0=false) 
				pakStream.get(),          // user data ; will be passed to our callback functions
				ReadFunc,
				0,                  // Write callback function (not used in this example) 
				SeekFunc);

			ic->pb = pIOCtx;

			size_t uBytesReaded = 0;
			uBytesReaded = pakStream->read(pBuffer, iBufSize);
			// Error Handling...

		// Don't forget to reset the data pointer back to the beginning!
			pakStream->seekg(0);
			// Error Handling...

		// Now we set the ProbeData-structure for av_probe_input_format:
			AVProbeData probeData{};
			probeData.buf = pBuffer;
			probeData.buf_size = uBytesReaded;
			probeData.filename = "";

			// Determine the input-format:
			ic->iformat = av_probe_input_format(&probeData, 1);
			ic->flags = AVFMT_FLAG_CUSTOM_IO;

		if (avformat_open_input(&ic, "", 0, 0) != 0)
		{
			RETURN_AV_ERROR("Failed to open stream: {}", url);
		}

		auto err = avformat_find_stream_info(ic, nullptr);
		if (err < 0)
		{
			avformat_close_input(&ic);
			RETURN_AV_ERROR("Cannot find stream info: {}", avErrorStr(err));
		}

		Ptr<SimpleInputFormat> res{new SimpleInputFormat{ic}};
		res->url_ = url;
		res->m_PakStream = pakStream;
		res->m_Buffer = pBuffer;

		{
			auto ret = res->findBestStream(AVMEDIA_TYPE_VIDEO);
			if (!ret)
				FORWARD_AV_ERROR(ret);
		}

		if (enableAudio)
		{
			auto ret = res->findBestStream(AVMEDIA_TYPE_AUDIO);
			if (!ret)
				FORWARD_AV_ERROR(ret);
		}

		//av_dump_format(ic, 0, nullptr, 0);

		return res;
	}

		AVFormatContext* GetAVFormat()
		{
			return ic_;
		}



	~SimpleInputFormat()
	{
		auto pIOCtx = ic_->pb;
		avformat_close_input(&ic_);
		av_free(pIOCtx);
		delete[] m_Buffer;
	}

	Expected<bool> readFrame(Packet& packet) noexcept
	{
		int err = 0;
		for (;;)
		{
			err = av_read_frame(ic_, *packet);

			if (err == AVERROR(EAGAIN))
				continue;

			if (err == AVERROR_EOF)
			{

				if (Repeat)
				{
					avio_seek(ic_->pb, 0, SEEK_SET);

					avformat_seek_file(ic_, std::get<0>(vStream_)->index, 0, 0, std::get<0>(vStream_)->duration, 0);
					av_seek_frame(ic_, std::get<0>(vStream_)->index, 0, 0);

					if (std::get<0>(aStream_))
					{
						avformat_seek_file(ic_, std::get<0>(aStream_)->index, 0, 0, std::get<0>(aStream_)->duration, 0);
						av_seek_frame(ic_, std::get<0>(aStream_)->index, 0, 0);
					}

					return readFrame(packet);
				}

				// flush cached frames from video decoder
				packet.native()->data = nullptr;
				packet.native()->size = 0;

				return false;
			}

			if (err < 0)
				RETURN_AV_ERROR("Failed to read frame: {}", avErrorStr(err));

			return true;
		}
	}

	auto& videoStream() noexcept
	{
		return vStream_;
	}
	auto& audioStream() noexcept
	{
		return aStream_;
	}

	bool Repeat = false;

	std::tuple<AVStream*, Ptr<Decoder>> vStream_;
	std::tuple<AVStream*, Ptr<Decoder>> aStream_;

private:
	Expected<void> findBestStream(AVMediaType type) noexcept
	{
		AVCodec* dec = nullptr;
		AVCodec** ppDec = &dec;
		int stream_i = av_find_best_stream(ic_, type, -1, -1, ppDec, 0);
		if (stream_i == AVERROR_STREAM_NOT_FOUND)
			RETURN_AV_ERROR("Failed to find {} stream in '{}'", av_get_media_type_string(type), url_);
		if (stream_i == AVERROR_DECODER_NOT_FOUND)
			RETURN_AV_ERROR("Failed to find decoder '{}' of '{}'", avcodec_get_name(ic_->streams[stream_i]->codecpar->codec_id), url_);

		if (type == AVMEDIA_TYPE_VIDEO)
		{
			const auto framerate = av_guess_frame_rate(ic_, ic_->streams[stream_i], nullptr);
			auto decContext      = Decoder::create(dec, ic_->streams[stream_i], framerate);

			if (!decContext)
				FORWARD_AV_ERROR(decContext);

			std::get<0>(vStream_) = ic_->streams[stream_i];
			std::get<1>(vStream_) = decContext.value();
		}
		else if (type == AVMEDIA_TYPE_AUDIO)
		{
			auto decContext = Decoder::create(dec, ic_->streams[stream_i]);

			if (!decContext)
				FORWARD_AV_ERROR(decContext);

			std::get<0>(aStream_) = ic_->streams[stream_i];
			std::get<1>(aStream_) = decContext.value();
		}
		else
			RETURN_AV_ERROR("Not supported stream type '{}'", av_get_media_type_string(type));

		return {};
	}

private:
	std::string url_;
	AVFormatContext* ic_{nullptr};
	ENGINE_NAMESPACE::PakStream m_PakStream;
	uint8_t* m_Buffer;

	
};

}// namespace av
