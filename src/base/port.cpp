#include "port.hpp"

PortMode parse_mode(const std::string& s) {
    if (s == "input")  return PortMode::Input;
    if (s == "output") return PortMode::Output;
    if (s == "master") return PortMode::Master;
    if (s == "slave")  return PortMode::Slave;
    throw std::runtime_error("Unknown port mode: " + s);
}

std::string to_string(PortType type) {
    switch (type) {
        case PortType::Wire: return "wire";
        case PortType::Interface: return "interface";
    }
    return "unknown";
}

std::string to_string(PortMode mode) {
    switch (mode) {
        case PortMode::Input:  return "input";
        case PortMode::Output: return "output";
        case PortMode::Master: return "master";
        case PortMode::Slave:  return "slave";
    }
    return "unknown";
}