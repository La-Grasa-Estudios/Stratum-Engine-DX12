#include "Window.h"

#include "Renderer/RendererContext.h"
#include "Input/Input.h"
#include "VarRegistry.h"
#include "Logger.h"
#include "Event/EventHandler.h"

#include <vector>

#include <iostream>

using namespace ENGINE_NAMESPACE;

using namespace Internal;

std::unordered_map<SDL_JoystickID, SDL_Gamepad*> g_Gamepads;

Window::Window(Render::RendererContext* pContext, const char* name)
{
	m_Window = NULL;
	this->m_Name = name;
	this->m_Context = pContext;
	this->m_Vsync = false;
	
}

Internal::Window::~Window()
{
	if (!m_Window) return;
}

void Window::Create(int width, int height)
{
	VarRegistry::RegisterConsoleVar("cl", "width", VarType::Int)->set(width);
	VarRegistry::RegisterConsoleVar("cl", "height", VarType::Int)->set(height);

	if (SDL_InitSubSystem(SDL_INIT_VIDEO)) {
		// TO DO: Set up a proper warning
		Z_ERROR("Could not initialize SDL! SDL_Error: {}", SDL_GetError());
		return;
	}

	if (SDL_InitSubSystem(SDL_INIT_GAMEPAD))
	{
		Z_ERROR("Could not initialize SDL Gamepad! SDL_Error: {}", SDL_GetError());
		return;
	}

	s_instance = this;

	uint32_t flags = SDL_WINDOW_RESIZABLE;
	if (this->StartMaximized) {
		flags |= SDL_WINDOW_MAXIMIZED;
	}
	if (this->FullScreen) {
		flags |= SDL_WINDOW_FULLSCREEN;
	}

	m_Window = SDL_CreateWindow(this->m_Name, width, height, flags);
	if (!m_Window)
	{
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return;
	}

	//glfwSetWindowSizeCallback(m_Window, window_size_callback);
	//glfwSetKeyCallback(m_Window, key_callback);
	//glfwSetMouseButtonCallback(m_Window, mouse_callback);

	m_Context->initialize(this);

	Render::FramebufferDesc framebufferDesc;
	framebufferDesc.IsWindowSurfaceFb = true;

	m_Framebuffer = CreateRef<Render::Framebuffer>(framebufferDesc);
	
	/*
	IMGUI_CHECKVERSION();

	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGuiIO& io = ImGui::GetIO();

	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	m_Context->ImGuiInit(this);

	while (1)
	{
		Clear();

		Update();
	}
	*/

}

void Internal::Window::Destroy()
{
	m_Context->terminate();
	SDL_DestroyWindow(m_Window);
	SDL_Quit();
	m_Window = NULL;
}

int Window::GetWidth()
{
	int width, h;
	SDL_GetWindowSize(m_Window, &width, &h);
	return width;
}

int Window::GetHeight()
{
	int height, w;
	SDL_GetWindowSize(m_Window, &w, &height);
	return height;
}

SDL_Window* Window::GetHandle()
{
	return m_Window;
}

Ref<Render::Framebuffer> Internal::Window::GetFramebuffer()
{
	return m_Framebuffer;
}

bool Window::CloseRequested()
{
	return m_ShouldClose;
}

bool Internal::Window::IsWindowActive()
{
	return m_IsWindowFocused;
}

void ENGINE_NAMESPACE::Internal::Window::Clear()
{
	m_Context->clear_front_buffer((int)Render::ClearBits::COLOR_BIT | (int)Render::ClearBits::DEPTH_BIT);
}

void Window::Update()
{
	m_Framebuffer->m_Width = GetWidth();
	m_Framebuffer->m_Height = GetHeight();

	m_Context->swap_buffers(this);
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		switch (e.type)
		{
		case SDL_EVENT_QUIT:
				m_ShouldClose = true;
				break;
		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			    m_ShouldClose = true;
			break;
		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP:
			Input::SetKey(e.key.keysym.scancode, e.key.state);
			break;
		case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		case SDL_EVENT_GAMEPAD_BUTTON_UP:
			Input::SetGamepad(e.gbutton.button, e.gbutton.state);
			break;
		case SDL_EVENT_GAMEPAD_AXIS_MOTION:
			Input::SetGamepadAxis(e.gaxis.axis, e.gaxis.value);
			break;
		case SDL_EVENT_GAMEPAD_ADDED:
			g_Gamepads[e.gdevice.which] = SDL_OpenGamepad(e.gdevice.which);
			Z_INFO("Gamepad found! id: {}-{}", e.gdevice.which, SDL_GetGamepadName(g_Gamepads[e.gdevice.which]))
			EventHandler::InvokeEvent(EventHandler::GetEventID("gamepad_connect"), this, { g_Gamepads[e.gdevice.which] }, 1);
			break;
		case SDL_EVENT_GAMEPAD_REMOVED:
			EventHandler::InvokeEvent(EventHandler::GetEventID("gamepad_remove"), this, { g_Gamepads[e.gdevice.which] }, 1);
			SDL_CloseGamepad(g_Gamepads[e.gdevice.which]);
			g_Gamepads.erase(e.gdevice.which);
			Z_INFO("Gamepad removed! id: {}", e.gdevice.which)
				break;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_UP:

			Input::SetMouse(e.button.button, e.button.state);
			//Input::SetMousePos(glm::vec2((float)xpos, (float)ypos));
			break;
		case SDL_EVENT_MOUSE_MOTION:
			Input::s_ThreadedMousePos = glm::vec2(e.motion.x, e.motion.y);
			break;
		case SDL_EVENT_WINDOW_RESIZED:
		{
			int width = e.window.data1;
			int height = e.window.data2;
			ConsoleVar* pWidthVar = VarRegistry::GetConsoleVar("cl", "width");
			ConsoleVar* pHeightVar = VarRegistry::GetConsoleVar("cl", "height");
			pWidthVar->set(width);
			pHeightVar->set(height);
			std::string log;
			VarRegistry::ParseConsoleVar("r_video_reset", log);
		}
			break;
		default:
			break;
		}
	}
	
	m_IsWindowFocused = (SDL_GetWindowFlags(m_Window) & SDL_WINDOW_INPUT_FOCUS);
}

void Internal::Window::ResetViewport()
{
	m_Context->set_viewport(this->GetWidth(), this->GetHeight());
}

void ENGINE_NAMESPACE::Internal::Window::SetName(const char* name)
{
	SDL_SetWindowTitle(m_Window, name);
}

void ENGINE_NAMESPACE::Internal::Window::SetIcon(int count, WindowIcon* icons)
{
	
}

void Internal::Window::SetVSync(bool vsync)
{
	m_Vsync = vsync;
	m_Context->set_vsync(vsync);
}

void Internal::Window::SetInfo(WindowEnum param, bool val)
{
	switch (param)
	{
	case ENGINE_NAMESPACE::Internal::WindowEnum::WINDOW_NULL:
		break;
	case ENGINE_NAMESPACE::Internal::WindowEnum::WINDOW_START_MAXIMIZED:
		StartMaximized = val;
		break;
	case ENGINE_NAMESPACE::Internal::WindowEnum::WINDOW_FULLSCREEN:
		FullScreen = val;
		break;
	case ENGINE_NAMESPACE::Internal::WindowEnum::WINDOW_IMGUI:
		m_IsImGuiEnabled = val;
		break;
	default:
		break;
	}
}
