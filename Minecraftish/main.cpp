#include "DevTools/ShaderCompiler.h"

#include "VFS/ZVFS.h"

#include "Renderer/GraphicsCommandBuffer.h"
#include "Renderer/ComputeCommandBuffer.h"
#include "Renderer/ShapeProvider.h"
#include "Renderer/Post/PostProcessingStack.h"

#include "Util/PathUtils.h"

#include "Core/JobManager.h"
#include "Core/Window.h"
#include "Core/Timer.h"
#include "Core/Logger.h"

#include "Asset/TextureLoader.h"
#include "Sound/AudioEngine.h"

#include "Media/VideoDecode.h"

#include "win32/win32_common.h"

using namespace ENGINE_NAMESPACE;

struct VertexPosUV
{
	glm::vec3 position;
	glm::vec2 uv;
};

struct VertexMesh
{
	VertexMesh(const Render::BufferDescription& v, const Render::BufferDescription& i, uint32_t indexCount)
	{
		vertexBuffer = new Render::Buffer(v);
		indexBuffer = new Render::Buffer(i);

		vertexView = new Render::VertexBuffer(vertexBuffer);
		indexView = new Render::IndexBuffer(indexBuffer);

		this->indexCount = indexCount;
	}

	Render::VertexBuffer* vertexView;
	Render::IndexBuffer* indexView;

	Render::Buffer* vertexBuffer;
	Render::Buffer* indexBuffer;

	uint32_t indexCount;
};

int main()
{

	using namespace Internal;

	JobManager::Init();
	ZVFS::Init();

	AudioEngine audioEngine = {};
	
	audioEngine.Init();

	GLSLToSpirV::build_object("Resources/blit.hlsl", "Resources/blit.cso", GLSLToSpirV::shader_type::vertex);

	GLSLToSpirV::build_object("Resources/shader.hlsl", "Resources/shader.cso", GLSLToSpirV::shader_type::vertex);

	GLSLToSpirV::build_object("Resources/shadow.hlsl", "Resources/shadow.cso", GLSLToSpirV::shader_type::vertex);

	GLSLToSpirV::build_object("Resources/video.hlsl", "Resources/video.cso", GLSLToSpirV::shader_type::vertex);

	GLSLToSpirV::build_object("Resources/compute.hlsl", "Resources/compute.cso", GLSLToSpirV::shader_type::compute);

	GLSLToSpirV::build_object("Resources/shaders/post/bloom_downsample.hlsl", "Resources/shaders/post/bloom_downsample.cso", GLSLToSpirV::shader_type::fragment);

	GLSLToSpirV::build_object("Resources/shaders/post/bloom_upsample.hlsl", "Resources/shaders/post/bloom_upsample.cso", GLSLToSpirV::shader_type::fragment);

	GLSLToSpirV::build_object("Resources/shaders/tone_map.hlsl", "Resources/shaders/tone_map.cso", GLSLToSpirV::shader_type::fragment);

	GLSLToSpirV::build_object("Resources/shaders/compute/compute_bloom_filter.hlsl", "Resources/shaders/compute/compute_bloom_filter.cso", GLSLToSpirV::shader_type::compute);
	GLSLToSpirV::build_object("Resources/shaders/compute/compute_avg_luminance.hlsl", "Resources/shaders/compute/compute_avg_luminance.cso", GLSLToSpirV::shader_type::compute);
	GLSLToSpirV::build_object("Resources/shaders/compute/compute_luminance.hlsl", "Resources/shaders/compute/compute_luminance.cso", GLSLToSpirV::shader_type::compute);

	ZVFS::Mount("Resources", true);

	Render::RendererContext* context = new Render::RendererContext();

	context->InitializeApi(Render::RendererAPI::DX12);

	Window* window = new Internal::Window(context, "DX12 Test");
	window->SetInfo(Internal::WindowEnum::WINDOW_START_MAXIMIZED,true);
	window->SetInfo(Internal::WindowEnum::WINDOW_IMGUI, false);
	window->SetInfo(Internal::WindowEnum::WINDOW_FULLSCREEN, false);
	window->SetVSync(false);

	window->Create(1280, 720);

	Render::GraphicsCommandBuffer cmdBuffer{};
	Render::ComputeCommandBuffer computeCmdBuffer(&cmdBuffer);

	uint64_t frames = 0;

	Timer timer;
	float lastTime = timer.GetMillis();
	uint32_t lastFrames = 0;

	if (ZVFS::Exists("media/startupvids.txt"))
	{

		std::string path = "media/startupvids.txt";

		RefBinaryStream vids = ZVFS::GetFile(path.c_str());

		std::string line;

		Render::PipelineDescription pipelineDesc;

		pipelineDesc.ShaderPath = "video.cso";

		pipelineDesc.NumRenderTargets = 1;
		pipelineDesc.RenderTarget = window->GetFramebuffer().get();

		pipelineDesc.RasterizerState.DepthTest = false;

		pipelineDesc.DepthTargetFormat = Render::ImageFormat::DEPTH16;

		Render::GraphicsPipeline videoPipeline(pipelineDesc);

		auto fullScreenQuad = Render::ShapeProvider::GenerateFullScreenQuad();

		while (std::getline(*vids->Stream(), line))
		{
			std::string vpath = PathUtils::ResolvePath(line);

			VideoDecode decode(vpath, &audioEngine);
			VideoParams params = decode.GetSize();

			if (params.width == 0) continue;

			int size = params.width * params.height * 4;

			Render::ImageDescription desc;
			desc.Width = params.width;
			desc.Height = params.height;
			desc.Format = Render::ImageFormat::RGBA8_UNORM;

			Ref<Render::ImageResource> surface = CreateRef<Render::ImageResource>(desc);
			Render::TextureSamplerDescription samplerDesc{};
			Render::TextureSampler sampler(samplerDesc);

			float frameTime = decode.GetFrametime();
			float accum = 0.0f;

			float globalTime = 0;
			float sleepTime = globalTime;
			int fIndex = 0;

			int frames = 0;

			bool firstFrameReady = false;

			bool end = false;

			bool frameIndex = 0;

			while (!decode.Finished() && !window->CloseRequested())
			{

				window->SetVSync(false);

				frameIndex = !frameIndex;

				Time::BeginProfile();

				decode.Step();

				JobManager::ExecuteMainJobs();

				accum += Time::DeltaTime;

				cmdBuffer.Begin();

				while (accum >= frameTime && !decode.Finished())
				{
					auto frame = decode.GetFrame();
					if (frame)
					{
						auto cmd = cmdBuffer.GetNativeCommandList();

						cmd->writeTexture(surface->Handle, 0, 0, frame->native()->data[0], params.width * 4);

						firstFrameReady = true;

						decode.PushFrame(frame);
						fIndex++;
					}
					accum -= frameTime;
				}

				if (!decode.Finished()) decode.StepAudio(globalTime);

				Render::Viewport vp{};
				vp.width = window->GetWidth();
				vp.height = window->GetHeight();

				cmdBuffer.SetViewport(&vp);
				cmdBuffer.SetPipeline(&videoPipeline);
				cmdBuffer.SetFramebuffer(window->GetFramebuffer().get());
				cmdBuffer.SetTextureResource(surface.get(), 0);
				cmdBuffer.SetTextureSampler(&sampler, 0);

				cmdBuffer.SetVertexBuffer(fullScreenQuad->GetVertexBuffer(), 0);
				cmdBuffer.SetIndexBuffer(fullScreenQuad->GetIndexBuffer());

				cmdBuffer.Draw(3);

				cmdBuffer.End();

				cmdBuffer.Submit();

				window->Update();

				JobManager::Wait();

				globalTime += Time::DeltaTime;
				sleepTime += frameTime;

				float diff = sleepTime - globalTime;

				if (diff > 0.0f)
				{
					int nano = diff * 1000 * 1000 * 1000;
					std::this_thread::sleep_for(std::chrono::nanoseconds(nano));
				}

				Time::EndProfile();

			}
		}


	}
	else
	{
		Z_WARN("Failed to open file media/startupvids.txt");
	}

	window->SetVSync(true);

	Render::ImageDescription shadowMapDesc{};

	shadowMapDesc.Width = 4096;
	shadowMapDesc.Height = 4096;
	shadowMapDesc.AllowFramebufferUsage = true;
	shadowMapDesc.ClearValue = glm::vec4(1.0f);
	shadowMapDesc.Format = Render::ImageFormat::DEPTH32;

	Render::ImageResource shadowMap(shadowMapDesc);

	Render::FramebufferDesc fbDesc;
	fbDesc.Attachments.push_back({ &shadowMap });

	Ref<Render::Framebuffer> shadowMapRt = CreateRef<Render::Framebuffer>(fbDesc);

	Render::PipelineDescription pipelineDesc;

	pipelineDesc.ShaderPath = "shader.cso";

	pipelineDesc.VertexLayout = Render::Vertex::GetLayout();
	pipelineDesc.VertexLayout.Stride = sizeof(Render::Vertex);

	pipelineDesc.NumRenderTargets = 1;
	pipelineDesc.RenderTarget = window->GetFramebuffer().get();

	pipelineDesc.RasterizerState.DepthTest = true;

	pipelineDesc.DepthTargetFormat = Render::ImageFormat::DEPTH16;

	pipelineDesc.BindingItems.push_back(nvrhi::BindingLayoutItem::PushConstants(0, 64));

	Render::GraphicsPipeline pipeline(pipelineDesc);

	Render::PipelineDescription blitPipelineDesc;

	blitPipelineDesc.ShaderPath = "blit.cso";

	blitPipelineDesc.NumRenderTargets = 1;
	blitPipelineDesc.RenderTarget = window->GetFramebuffer().get();

	blitPipelineDesc.RasterizerState.DepthTest = false;
	blitPipelineDesc.StencilState.DepthEnable = false;

	blitPipelineDesc.DepthTargetFormat = Render::ImageFormat::DEPTH16;

	Render::GraphicsPipeline blitPipeline(blitPipelineDesc);

	Render::PipelineDescription shadowPipelineDesc{};

	shadowPipelineDesc.ShaderPath = "shadow.cso";

	shadowPipelineDesc.VertexLayout = Render::Vertex::GetLayout();
	shadowPipelineDesc.VertexLayout.Stride = sizeof(Render::Vertex);

	shadowPipelineDesc.NumRenderTargets = 1;
	shadowPipelineDesc.RenderTarget = shadowMapRt.get();

	shadowPipelineDesc.DepthTargetFormat = Render::ImageFormat::DEPTH32;

	shadowPipelineDesc.BindingItems.push_back(nvrhi::BindingLayoutItem::PushConstants(0, 64));

	Render::GraphicsPipeline shadowPipeline(shadowPipelineDesc);

	Render::ComputePipelineDesc computeDesc{};

	computeDesc.path = "compute.cso";

	Render::ComputePipeline computePipeline(computeDesc);

	Render::Viewport viewport{};
	viewport.width = window->GetWidth();
	viewport.height = window->GetHeight();

	glm::mat4 projection = glm::perspective(glm::radians(70.0f), viewport.width / (float)viewport.height, 0.01f, 100.0f);
	glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 3.0f, -5.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	glm::mat4 pv[3]
	{
		projection,
		view,
		glm::identity<glm::mat4>(),
	};

	Render::ConstantBuffer cb(sizeof(glm::mat4) * 3);

	Render::TextureSamplerDescription samplerDesc{};
	Render::TextureSamplerDescription shadowMapSamplerDesc{};

	samplerDesc.AddressMode = Render::TextureWrapMode::CLAMP;

	shadowMapSamplerDesc.AddressMode = Render::TextureWrapMode::CLAMP;
	shadowMapSamplerDesc.Filter = Render::TextureFilterMode::POINT;

	Render::TextureSampler sampler(samplerDesc);
	Render::TextureSampler shadowMapSampler(shadowMapSamplerDesc);

	auto Cube = Render::ShapeProvider::GenerateBox(glm::vec3(1.0f));
	auto GroundPlane = Render::ShapeProvider::GenerateQuad(glm::vec2(1.0f));

	bool transparent = false;
	Ref<Render::ImageResource> cubeImage = Render::TextureLoader::LoadFileToImage("pene.jpg", &transparent);
	Ref<Render::ImageResource> floorImage = Render::TextureLoader::LoadFileToImage("spnza.ctex", &transparent);

	Ref<Render::ImageResource> renderTargetColor;
	Ref<Render::ImageResource> renderTargetDepth;
	Ref<Render::Framebuffer> renderTarget;

	Render::PostProcessingStack stack{};

	while (!window->CloseRequested())
	{

		if (!renderTargetColor || renderTargetColor->GetSize() != glm::ivec2(window->GetWidth(), window->GetHeight()))
		{
			Render::ImageDescription colorDesc{};

			colorDesc.Width = window->GetWidth();
			colorDesc.Height = window->GetHeight();
			colorDesc.AllowFramebufferUsage = true;
			colorDesc.AllowComputeResourceUsage = true;
			colorDesc.ClearValue = glm::vec4(0.0f);
			colorDesc.Format = Render::ImageFormat::RGBA8_UNORM;

			Render::ImageDescription depthDesc{};

			depthDesc.Width = window->GetWidth();
			depthDesc.Height = window->GetHeight();
			depthDesc.AllowFramebufferUsage = true;
			depthDesc.ClearValue = glm::vec4(1.0f);
			depthDesc.Format = Render::ImageFormat::DEPTH16;

			renderTargetColor = CreateRef<Render::ImageResource>(colorDesc);
			renderTargetDepth = CreateRef<Render::ImageResource>(depthDesc);

			Render::FramebufferDesc fbDesc;
			fbDesc.Attachments.push_back({ renderTargetColor.get() });
			fbDesc.Attachments.push_back({ renderTargetDepth.get() });

			renderTarget = CreateRef<Render::Framebuffer>(fbDesc);

			stack.Init(renderTargetColor->GetSize());
		}

		frames++;

		if (timer.GetMillis() >= lastTime + 1000.0f)
		{
			uint32_t fps = frames - lastFrames;

			Z_INFO("FPS: {}", fps);

			lastFrames = frames; 
			lastTime = timer.GetMillis();
		}

		viewport.width = window->GetWidth();
		viewport.height = window->GetHeight();

		glm::mat4 shadowProj = glm::ortho(-16.0f, 16.0f, -16.0f, 16.0f, 0.1f, 100.0f);
		glm::mat4 shadowView = glm::lookAt(glm::vec3(3.0f, 15.0f, 5.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		pv[0] = glm::perspective(glm::radians(90.0f), viewport.width / (float)viewport.height, 0.01f, 100.0f);glm::perspective(glm::radians(90.0f), viewport.width / (float)viewport.height, 0.01f, 100.0f);
		pv[2] = shadowProj * shadowView;

		cmdBuffer.Begin();
		computeCmdBuffer.Begin();

		cmdBuffer.SetFramebuffer(shadowMapRt.get());
		cmdBuffer.ClearDepth(1.0f);
		cmdBuffer.SetPipeline(&shadowPipeline);

		viewport.width = shadowMapDesc.Width;
		viewport.height = shadowMapDesc.Height;

		cmdBuffer.SetViewport(&viewport);

		{
			glm::mat4 model = glm::identity<glm::mat4>();
			model = glm::translate(model, { 0.0f, 2.0f, 0.0f });
			model = glm::rotate(model, frames / 60.0f, glm::vec3(0.0f, 1.0f, 0.0f));

			cmdBuffer.SetVertexBuffer(Cube->GetVertexBuffer(), 0);
			cmdBuffer.SetIndexBuffer(Cube->GetIndexBuffer());

			model = shadowProj * shadowView * model;

			cmdBuffer.PushConstants(&model, 64);
			cmdBuffer.DrawIndexed(Cube->GetIndicesCount());
		}

		viewport.width = window->GetWidth();
		viewport.height = window->GetHeight();

		cmdBuffer.SetFramebuffer(renderTarget.get());
		cmdBuffer.ClearBuffer(0, glm::vec4(0.0f));
		cmdBuffer.ClearDepth(1.0f);
		cmdBuffer.SetPipeline(&pipeline);
		cmdBuffer.SetViewport(&viewport);

		cmdBuffer.SetConstantBuffer(&cb, 1);
		cmdBuffer.SetTextureResource(cubeImage.get(), 0);
		cmdBuffer.SetTextureResource(&shadowMap, 1);
		cmdBuffer.SetTextureSampler(&sampler, 0);
		cmdBuffer.UpdateConstantBuffer(&cb, pv);

		{
			glm::mat4 model = glm::identity<glm::mat4>();
			model = glm::translate(model, { 0.0f, 2.0f, 0.0f });
			model = glm::rotate(model, frames / 60.0f, glm::vec3(0.0f, 1.0f, 0.0f));

			cmdBuffer.SetVertexBuffer(Cube->GetVertexBuffer(), 0);
			cmdBuffer.SetIndexBuffer(Cube->GetIndexBuffer());

			cmdBuffer.PushConstants(&model, 64); 
			cmdBuffer.DrawIndexed(Cube->GetIndicesCount());
		}

		cmdBuffer.SetVertexBuffer(GroundPlane->GetVertexBuffer(), 0);
		cmdBuffer.SetIndexBuffer(GroundPlane->GetIndexBuffer());

		cmdBuffer.SetTextureResource(floorImage.get(), 0);

		for (int i = 0; i < 8; i++)
		{
			float x = (1.5f - (i % 4)) * 2;
			float y = (1.5f - (i / 4)) * 2;

			glm::mat4 model = glm::identity<glm::mat4>();
			model = glm::scale(model, glm::vec3(3.0f));
			model = glm::translate(model, { x, 0.0f, -1 + y });
			model = glm::scale(model, glm::vec3(1.0f));

			cmdBuffer.PushConstants(&model, 64);
			cmdBuffer.DrawIndexed(GroundPlane->GetIndicesCount());
		}

		computeCmdBuffer.SetComputePipeline(&computePipeline);
		computeCmdBuffer.SetTextureCompute(renderTargetColor.get(), 0);

		computeCmdBuffer.Dispatch(16, 16, 1);

		cmdBuffer.ClearVertexBuffers();
		cmdBuffer.SetIndexBuffer(NULL);

		/*
		cmdBuffer.ClearVertexBuffers();
		cmdBuffer.SetIndexBuffer(NULL);
		cmdBuffer.SetPipeline(&blitPipeline);
		cmdBuffer.SetFramebuffer(window->GetFramebuffer().get());
		cmdBuffer.SetTextureResource(renderTargetColor.get(), 0);
		*/

		cmdBuffer.Draw(3);

		Render::PostProcessingParameters params{};

		params.pOutputFramebuffer = window->GetFramebuffer().get();
		params.cCommandBuffer = &computeCmdBuffer;
		params.gCommandBuffer = &cmdBuffer;
		params.pBilinearTextureSampler = &sampler;
		params.pColorSampler = renderTargetColor.get();
		params.pDepthSampler = renderTargetDepth.get();
		params.OutputResolution = renderTargetColor->GetSize();
		params.Resolution = renderTargetColor->GetSize();

		stack.Render(params);

		cmdBuffer.End();
		cmdBuffer.Submit();

		window->Update();
	}

	//window->Destroy();
	
	return 0;
}