#pragma once

#include "Frame.hpp"
#include "Packet.hpp"
#include "common.hpp"

namespace av
{

class Decoder : NoCopyable
{
	explicit Decoder(AVCodecContext** codecContext) noexcept
	{
		memcpy(this->codecContext_, codecContext, sizeof(codecContext_));
	}

public:
	static Expected<Ptr<Decoder>> create(AVCodec* codec, AVStream* stream, AVRational framerate = {})
	{

		AVCodecContext* ctxs[4]{};

		for (int i = 0; i < 4; i++)
		{
			if (!av_codec_is_decoder(codec))
				RETURN_AV_ERROR("{} is not a decoder", codec->name);

			auto codecContext = avcodec_alloc_context3(codec);
			if (!codecContext)
				RETURN_AV_ERROR("Could not alloc an encoding context");

			auto ret = avcodec_parameters_to_context(codecContext, stream->codecpar);
			if (ret < 0)
			{
				avcodec_free_context(&codecContext);
				RETURN_AV_ERROR("Failed to copy parameters to context: {}", avErrorStr(ret));
			}

			if (codecContext->codec_type == AVMEDIA_TYPE_VIDEO)
			{
				if (!framerate.num && !framerate.den)
				{
					avcodec_free_context(&codecContext);
					RETURN_AV_ERROR("Framerate is not set");
				}

				codecContext->framerate = framerate;
			}

			codecContext->thread_count = 0;
			codecContext->thread_type = FF_THREAD_FRAME;

			AVDictionary* opts = nullptr;
			ret = avcodec_open2(codecContext, codecContext->codec, &opts);
			if (ret < 0)
			{
				avcodec_free_context(&codecContext);
				RETURN_AV_ERROR("Could not open video codec: {}", avErrorStr(ret));
			}
			ctxs[i] = codecContext;
		}

		return Ptr<Decoder>{new Decoder{ ctxs }};
	}

	~Decoder()
	{
		if (codecContext_[0])
		{
			for (int i = 0; i < 4; i++)
			{
				avcodec_close(codecContext_[i]);
				avcodec_free_context(&codecContext_[i]);
			}
		}
	}

	auto* operator*() noexcept
	{
		return codecContext_[0];
	}
	const auto* operator*() const noexcept
	{
		return codecContext_[0];
	}

	auto* native() noexcept
	{
		return codecContext_[0];
	}
	const auto* native() const noexcept
	{
		return codecContext_[0];
	}

	Expected<Result> decode(Packet& packet, Frame& frame, int decindex = 0) noexcept
	{
		int err = avcodec_send_packet(codecContext_[decindex], *packet);

		if (err == AVERROR(EAGAIN))
			return Result::kEAGAIN;

		if (err == AVERROR_EOF)
			return Result::kEOF;

		if (err < 0)
			RETURN_AV_ERROR("Decoder error: {}", avErrorStr(err));

		err = avcodec_receive_frame(codecContext_[decindex], *frame);

		if (err == AVERROR(EAGAIN))
			return Result::kEAGAIN;

		if (err == AVERROR_EOF)
			return Result::kEOF;

		if (err < 0)
			RETURN_AV_ERROR("Decoder error: {}", avErrorStr(err));

		return Result::kSuccess;
	}

private:

	AVCodecContext* codecContext_[4] = { nullptr , nullptr, nullptr , nullptr };

};

}// namespace av
