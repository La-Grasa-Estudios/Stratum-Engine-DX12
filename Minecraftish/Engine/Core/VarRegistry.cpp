#include "VarRegistry.h"

#include <regex>
#include <string>

#include "Logger.h"
#include "VFS/ZVFS.h"

using namespace ENGINE_NAMESPACE;

bool isNumber(std::string token)
{
    return std::regex_match(token, std::regex(("((\\+|-)?[[:digit:]]+)(\\.(([[:digit:]]+)?))?")));
}
bool isInteger(std::string token) {
    return token.find_first_not_of("-1234567890") == std::string::npos;
}

std::string formatConVar(ConsoleVar& var, std::string conVar) {
    std::string log;
    if (var.type == String) {
        log = std::string(conVar).append(" = ").append(var.str());
    }
    if (var.type == Bool) {
        log = std::string(conVar).append(" = ").append(var.asBool() ? "true" : "false");
    }
    if (var.type == Int) {
        log = std::string(conVar).append(" = ").append(std::to_string(var.asInt()));
    }
    if (var.type == Float) {
        log = std::string(conVar).append(" = ").append(std::to_string(var.asFloat()));
    }
    return log;
}

ConsoleVar* VarRegistry::RegisterConsoleVar(std::string module, std::string name, VarType type)
{

    Ref<ConsoleVar> var = CreateRef<ConsoleVar>();

    var->type = type;

    std::string cvar = module;
    cvar.append("_").append(name);


    if (module.empty()) {
        cvar = name;
    }

    s_GlobalVars[cvar] = var;

    return var.get();
}

ConsoleVar* VarRegistry::GetConsoleVar(std::string module, std::string name)
{
    std::string cvar = module;
    cvar.append("_").append(name);
    if (s_GlobalVars.contains(cvar)) {
        return s_GlobalVars[cvar].get();
    }
    return nullptr;
}

void VarRegistry::RunCfg(std::string path)
{

    if (!path.ends_with("cfg")) {
        Z_WARN("{} Is an invalid cfg file!", path);
    }

    if (!ZVFS::Exists(path.c_str())) {
        Z_WARN("Failed to open {}", path);
        return;
    }

    Z_INFO("Running {}", path);

    std::ifstream stream = std::ifstream(path.c_str());

    std::string line;
    while (std::getline(stream, line)) {

        if (line.find_first_of("#") != std::string::npos || line.empty()) {
            // Its a comment so ignore
            continue;
        }

        std::string log;

        ParseConsoleVar(line, log);

        if (!log.empty()) {
            Z_INFO(log);
        }

    }
}

void VarRegistry::Cleanup()
{
    s_GlobalVars.clear();
}

bool VarRegistry::ParseConsoleVar(std::string in, std::string& log)
{

    size_t pos = in.find_first_of(" ");
    size_t seekPos = in.size();

    if (pos != std::string::npos) {
        seekPos = pos;
    }

    std::string conVar;
    conVar.resize(seekPos);

    memcpy(conVar.data(), in.data(), seekPos);

    if (!s_GlobalVars.contains(conVar)) {
        log = std::string("Cannot find conVar ").append(conVar);
        return false;
    }
    ConsoleVar& var = *s_GlobalVars[conVar];

    if (var.type == Void) {
        if (var.func) {

            std::string param = in.substr(pos + 1);

            var.func(var, param);
        }
        return true;
    }

    if (pos == std::string::npos) {
        log = formatConVar(var, conVar);
        return true;
    }
    else {

        std::string param = in.substr(pos+1);

        if (var.type == String) {
            var.set(param);
        }
        else {
            if (isNumber(param)) {

                if (isInteger(param)) {

                    if (var.type == Int) {
                        var.set(std::atoi(param.data()));
                        return true;
                    }
                    if (var.type == Bool) {
                        var.set((bool)std::atoi(param.data()));
                        return true;
                    }

                    log = "Invalid type";

                }
                else {

                    double val = std::atof(param.data());

                    if (var.type == Float) {
                        var.set((float)val);
                        return true;
                    }

                    log = "Invalid type";

                }

            }
            else {
                if (var.type == Bool) {
                    var.set(param == "true");
                    return true;
                }
            }
        }

    }

    return true;
}

std::vector<std::string> VarRegistry::GetConVars(int maxConVars, std::string& filter)
{

    std::vector<std::string> conVars;

    if (filter.empty()) return conVars;

    size_t pos = filter.find_first_of(" ");
    size_t seekPos = filter.size();

    if (pos != std::string::npos) {
        seekPos = pos;
    }

    std::string conVar;
    conVar.resize(seekPos);

    memcpy(conVar.data(), filter.data(), seekPos);

    int nbVars = 0;

    for (auto& i : s_GlobalVars)
    {
        std::string name = i.first;
        if (name.find(conVar) != std::string::npos) {
            
            nbVars++;
            if (nbVars > maxConVars) {
                break;
            }
            ConsoleVar& var = *i.second;
            if (var.type == Void) {
                conVars.push_back(name);
            }
            else {
                conVars.push_back(formatConVar(var, name));
            }
        }
    }
    return conVars;
}

void VarRegistry::InitCVar(ConsoleVar& var)
{



}
