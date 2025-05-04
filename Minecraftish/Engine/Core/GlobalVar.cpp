#include "GlobalVar.h"

using namespace ENGINE_NAMESPACE;

int32_t ConsoleVar::asInt() {
	if (!data) return 0;
	return *(int32_t*)data;
}

float_t ConsoleVar::asFloat() {
	if (!data) return 0;
	return *(float_t*)data;
}

bool ConsoleVar::asBool() {
	if (!data) return 0;
	return *(bool*)data;
}

std::string ConsoleVar::str()
{
	if (!data) return "";
    std::string str((const char*)data);
    return str;
}

void ConsoleVar::setOnModifyCallback(std::function<void(ConsoleVar&)> callback)
{
	hasCallBack = true;
	this->callback = callback;
}

ConsoleVar* ConsoleVar::set(int32_t i)
{
	if (dataSize < sizeof(int32_t)) {
		if (data) {
			delete[] data;
		}
		data = new char[sizeof(int32_t)];
		dataSize = sizeof(int32_t);
	}
	memcpy(data, &i, sizeof(int32_t));
	if (hasCallBack) {
		callback(*this);
	}
	return this;
}

ConsoleVar* ConsoleVar::set(float_t i)
{
	if (dataSize < sizeof(float_t)) {
		if (data) {
			delete[] data;
		}
		data = new char[sizeof(float_t)];
		dataSize = sizeof(float_t);
	}
	memcpy(data, &i, sizeof(float_t));
	if (hasCallBack) {
		callback(*this);
	}
	return this;
}

ConsoleVar* ConsoleVar::set(bool i)
{
	if (dataSize < sizeof(bool)) {
		if (data) {
			delete[] data;
		}
		data = new char[sizeof(bool)];
		dataSize = sizeof(bool);
	}
	memcpy(data, &i, sizeof(bool));
	if (hasCallBack) {
		callback(*this);
	}
	return this;
}

ConsoleVar* ConsoleVar::set(std::string_view i)
{
	if (dataSize < i.size()) {
		if (data) {
			delete[] data;
		}
		data = new char[i.size() + 16];
		dataSize = i.size() + 16;
	}
	memset(data, 0, dataSize);
	memcpy(data, i.data(), i.size());
	if (hasCallBack) {
		callback(*this);
	}
	return this;
}

ConsoleVar* ConsoleVar::set(const char* i)
{
	return set(std::string(i));
}

ConsoleVar::operator bool()
{
	return asBool();
}

ConsoleVar::operator int32_t()
{
	return asInt();
}

ConsoleVar::operator float_t()
{
	return asFloat();
}

ConsoleVar::operator std::string()
{
	return str();
}
