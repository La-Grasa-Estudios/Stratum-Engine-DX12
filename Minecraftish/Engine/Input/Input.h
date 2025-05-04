#pragma once

#define MAX_KEYS 1000
#define MAX_BUTTONS 64

#include "znmsp.h"
#include <glm/glm.hpp>

#include "KeyCode.h"

struct SDL_Window;

BEGIN_ENGINE

enum class MouseInputMode {
	Normal,
	Hidden,
	Disabled,
};

class Input {

	DLLEXPORT static inline bool m_LastKeys[MAX_KEYS];
	DLLEXPORT static inline bool m_Keys[MAX_KEYS];
	DLLEXPORT static inline bool m_NextKeys[MAX_KEYS];

	DLLEXPORT static inline bool m_GamePadButtons[MAX_BUTTONS];
	DLLEXPORT static inline bool m_LastGamePadButtons[MAX_BUTTONS];

	DLLEXPORT static inline int16_t m_GamepadAxis[MAX_BUTTONS];
	
	DLLEXPORT static inline bool m_LastMouse[32];
	DLLEXPORT static inline bool m_Mouse[32];
	DLLEXPORT static inline bool m_NextMouse[32];

	DLLEXPORT static inline bool m_AnyKeyDown;
	DLLEXPORT static inline bool m_AnyGamepadDown;

	DLLEXPORT static inline uint8_t m_GamepadCount = 0;

	DLLEXPORT static inline glm::vec2 m_Pos;

	DLLEXPORT static inline SDL_Window* m_Window;

public:

	inline static glm::vec2 s_MousePosition = {};
	inline static glm::vec2 s_MouseSpeed = {};
	inline static glm::vec2 s_ThreadedMousePos = {};

	static void Init(SDL_Window* window);

	static void SetKey(int key, bool press);
	static void SetMouse(int click, bool press);
	static void SetMousePos(glm::vec2 pos);
	static void SetGamepad(int button, bool press);
	static void SetGamepadAxis(int axis, int16_t value);

	DLLEXPORT static void SetInputMode(MouseInputMode mode);

	DLLEXPORT static bool GetKeyDown(KeyCode keyCode);
	DLLEXPORT static bool GetKey(KeyCode keyCode);

	DLLEXPORT static bool GetMouseButton(int button);
	DLLEXPORT static bool GetMouseButttonDown(int button);

	DLLEXPORT static bool GetGamepadButton(int button);
	DLLEXPORT static bool GetGamepadButtonDown(int button);

	DLLEXPORT static float GetGamepadAxis(int axis);

	DLLEXPORT static bool AnyKeyDown();
	DLLEXPORT static bool AnyGamepadDown();
	DLLEXPORT static bool HasGamepadConnected();

	DLLEXPORT static glm::vec2 GetMousePosition();
	DLLEXPORT static glm::vec2 GetMouseSpeed();

	static void Update();

};

END_ENGINE
