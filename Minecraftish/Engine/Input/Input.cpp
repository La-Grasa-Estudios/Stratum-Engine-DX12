#include "Input.h"
#include "Core/Logger.h"
#include "Event/EventHandler.h"

#include <vcruntime_string.h>

#include <SDL3/SDL.h>

bool g_IsMouseGrabbed = false;

void ENGINE_NAMESPACE::Input::Init(SDL_Window* window)
{
	m_Window = window;
	s_MousePosition = GetMousePosition();
	memset(m_Keys, 0, sizeof(m_Keys));
	memset(m_LastKeys, 0, sizeof(m_LastKeys));
	memset(m_GamePadButtons, 0, sizeof(m_GamePadButtons));
	memset(m_GamepadAxis, 0, sizeof(m_GamepadAxis));

	EventHandler::RegisterListener([&](void*, void**, uint32_t) {

		m_GamepadCount += 1;

		}, EventHandler::GetEventID("gamepad_connect"));
	EventHandler::RegisterListener([&](void*, void**, uint32_t) {

		m_GamepadCount -= 1;

		}, EventHandler::GetEventID("gamepad_remove"));
}

void ENGINE_NAMESPACE::Input::SetKey(int key, bool press)
{
	m_NextKeys[key] = press;
}

void ENGINE_NAMESPACE::Input::SetMouse(int click, bool press)
{
	int button = click;
	if (button == 1) button = 0;
	if (button == 3) button = 1;
	m_NextMouse[button] = press;
}

void ENGINE_NAMESPACE::Input::SetMousePos(glm::vec2 pos)
{
	SDL_WarpMouseInWindow(m_Window, pos.x, pos.y);
}

void ENGINE_NAMESPACE::Input::SetGamepad(int button, bool press)
{
	m_GamePadButtons[button] = press;
}

void ENGINE_NAMESPACE::Input::SetGamepadAxis(int axis, int16_t value)
{
	//Z_INFO("Axis! {}, Value: {}", axis, value);
	m_GamepadAxis[axis] = value;
}

void ENGINE_NAMESPACE::Input::SetInputMode(MouseInputMode mode)
{
	int input = 0;
	switch (mode)
	{
	case ENGINE_NAMESPACE::MouseInputMode::Normal:
		input = 0;
		g_IsMouseGrabbed = false;
		break;
	case ENGINE_NAMESPACE::MouseInputMode::Hidden:
		input = 1;
		g_IsMouseGrabbed = false;
		break;
	case ENGINE_NAMESPACE::MouseInputMode::Disabled:
		input = 1;
		g_IsMouseGrabbed = true;
		break;
	default:
		break;
	}

	SDL_SetRelativeMouseMode((SDL_bool)input);
}

bool ENGINE_NAMESPACE::Input::GetKeyDown(KeyCode keyCode)
{
	return m_Keys[(int)keyCode] && !m_LastKeys[(int)keyCode];
}

bool ENGINE_NAMESPACE::Input::GetKey(KeyCode keyCode)
{
	return m_Keys[(int)keyCode];
}

bool ENGINE_NAMESPACE::Input::GetMouseButton(int button)
{
	return m_Mouse[button];
}

bool ENGINE_NAMESPACE::Input::GetMouseButttonDown(int button)
{
	return m_Mouse[button] && !m_LastMouse[button];;
}

bool ENGINE_NAMESPACE::Input::GetGamepadButton(int button)
{
	return m_GamePadButtons[button];
}

bool ENGINE_NAMESPACE::Input::GetGamepadButtonDown(int button)
{
	return m_GamePadButtons[button] && !m_LastGamePadButtons[button];
}

float ENGINE_NAMESPACE::Input::GetGamepadAxis(int axis)
{
	float a = m_GamepadAxis[axis] / (float)(INT16_MAX);
	bool s = a < 0.0f;
	a = glm::max(glm::abs(a) - 0.1f, 0.0f) / 0.9f;
	return a * (s ? -1.0f : 1.0f);
}

bool ENGINE_NAMESPACE::Input::AnyKeyDown()
{
	return m_AnyKeyDown;
}

bool ENGINE_NAMESPACE::Input::AnyGamepadDown()
{
	return m_AnyGamepadDown;
}

bool ENGINE_NAMESPACE::Input::HasGamepadConnected()
{
	return m_GamepadCount > 0;
}

glm::vec2 ENGINE_NAMESPACE::Input::GetMousePosition()
{
	return s_ThreadedMousePos;
}

glm::vec2 ENGINE_NAMESPACE::Input::GetMouseSpeed()
{
	return s_MouseSpeed;
}

void ENGINE_NAMESPACE::Input::Update()
{
	memcpy(m_LastKeys, m_Keys, sizeof(m_Keys));
	memcpy(m_Keys, m_NextKeys, sizeof(m_Keys));

	memcpy(m_LastMouse, m_Mouse, sizeof(m_Mouse));
	memcpy(m_Mouse, m_NextMouse, sizeof(m_Mouse));

	memcpy(m_LastGamePadButtons, m_GamePadButtons, sizeof(m_GamePadButtons));

	m_AnyKeyDown = false;

	for (int i = 0; i < 1000 && !m_AnyKeyDown; i++)
	{
		m_AnyKeyDown |= m_Keys[i];
		m_AnyKeyDown |= m_LastKeys[i];
		
	}

	for (int i = 0; i < MAX_BUTTONS && !m_AnyKeyDown; i++)
	{
		m_AnyKeyDown |= m_GamePadButtons[i];
	}

	glm::vec2 mousePos = GetMousePosition();

	s_MouseSpeed = mousePos - s_MousePosition;

	s_MousePosition = mousePos;

	if (g_IsMouseGrabbed) {
		int w, h;

		SDL_GetWindowSizeInPixels(m_Window, &w, &h);

		SetMousePos(glm::vec2(w / 2, h / 2));
		s_MousePosition = glm::vec2(w / 2, h / 2);
	}

	float x = 0.0f;
	float y = 0.0f;
	SDL_GetMouseState(&x, &y);
	s_ThreadedMousePos = glm::vec2(x, y);

	SDL_UpdateGamepads();

}
