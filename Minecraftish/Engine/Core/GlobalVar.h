#pragma once

#include "znmsp.h"

#include <functional>

BEGIN_ENGINE

enum VarType {
	Int,
	Float,
	Bool,
	String,
	Void,
};

class ConsoleVar {

public:

	VarType type;
	char* data = 0;

	DLLEXPORT inline int32_t asInt();
	DLLEXPORT inline float_t asFloat();
	DLLEXPORT inline bool asBool();
	DLLEXPORT inline std::string str();

	DLLEXPORT void setOnModifyCallback(std::function<void(ConsoleVar&)> callback);

	DLLEXPORT ConsoleVar* set(int32_t i);
	DLLEXPORT ConsoleVar* set(float_t i);
	DLLEXPORT ConsoleVar* set(bool i);
	DLLEXPORT ConsoleVar* set(std::string_view i);
	DLLEXPORT ConsoleVar* set(const char* i);

	DLLEXPORT operator bool();
	DLLEXPORT operator int32_t();
	DLLEXPORT operator float_t();
	DLLEXPORT operator std::string();

	std::function<void(ConsoleVar&, std::string& args)> func;

private:

	bool hasCallBack = false;
	std::function<void(ConsoleVar&)> callback;
	size_t dataSize = 0;

};

END_ENGINE