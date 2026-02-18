#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <stdexcept>

// -------------------- Types --------------------
enum class PortType { Wire, Interface };
enum class PortMode { Input, Output, Master, Slave };

struct Port {
    std::string name;
    PortType type;
    PortMode mode;
    virtual ~Port() = default;
};

struct WirePort : public Port {
    unsigned width = 1;
    WirePort() { type = PortType::Wire; }
};

struct InterfacePort : public Port {
    std::string protocol;   
    std::unordered_map<std::string, std::string> parameters;
    std::unordered_map<std::string, std::string> port_maps;
    InterfacePort() { type = PortType::Interface; }
};

// -------------------- Helpers --------------------
PortMode parse_mode(const std::string& s);
std::string to_string(PortType type);
std::string to_string(PortMode mode);