#pragma once

#include "GlobalVar.h"
#include "Ref.h"

#include <unordered_map>

BEGIN_ENGINE

class VarRegistry {

public:

	DLLEXPORT static ConsoleVar* RegisterConsoleVar(std::string module, std::string name, VarType type);
	DLLEXPORT static ConsoleVar* GetConsoleVar(std::string module, std::string name);
	DLLEXPORT static void RunCfg(std::string file);
	DLLEXPORT static void Cleanup();
	DLLEXPORT static bool ParseConsoleVar(std::string in, std::string& log);
	DLLEXPORT static std::vector<std::string> GetConVars(int maxConVars, std::string& filter);

private:

	static void InitCVar(ConsoleVar& var);
	inline static std::unordered_map<std::string, Ref<ConsoleVar>> s_GlobalVars;

};

END_ENGINE