#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <unordered_map>
#include "third_party/json.hpp"
using json = nlohmann::json;

enum class PortType {
    Wire,
    Interface
};

enum class PortMode {
    Input,
    Output,
    Master,
    Slave
};

struct Port {
    std::string name;
    PortType type;
    PortMode mode;

    virtual ~Port() = default;
};

struct WirePort : public Port {
    unsigned width = 1;

    WirePort() {
        type = PortType::Wire;
    }
};

std::string to_string(PortType k) {
    switch (k) {
        case PortType::Wire: return "wire";
        case PortType::Interface: return "interface";
    }
    return "unknown";
}

struct InterfacePort : public Port {
    std::string protocol;   // "axi_stream"

    // Parameters may be numeric or symbolic ("DATA_WIDTH")
    std::unordered_map<std::string, std::string> parameters;

    // Logical signal -> top-level signal name
    std::unordered_map<std::string, std::string> signal_map;

    InterfacePort() {
        type = PortType::Interface;
    }
};

struct SystemIR {
    std::unordered_map<std::string, std::unique_ptr<Port>> ports;
};

PortMode parse_mode(const std::string& s) {
    if (s == "input")  return PortMode::Input;
    if (s == "output") return PortMode::Output;
    if (s == "master") return PortMode::Master;
    if (s == "slave")  return PortMode::Slave;
    throw std::runtime_error("Unknown port mode: " + s);
}

std::string to_string(PortMode m) {
    switch (m) {
        case PortMode::Input:  return "input";
        case PortMode::Output: return "output";
        case PortMode::Master: return "master";
        case PortMode::Slave:  return "slave";
    }
    return "unknown";
}

void parse_interface_ports(const json& j, SystemIR& sys) {
    for (const auto& p : j.at("interface_ports")) {

        const std::string name = p.at("name");
        const std::string type = p.at("type");
        const std::string mode_str = p.at("mode");

        if (type == "wire") {
            auto port = std::make_unique<WirePort>();
            port->name = name;
            port->mode = parse_mode(mode_str);
            port->width = p.value("width", 1u);

            sys.ports[name] = std::move(port);
        }
        else {
            auto port = std::make_unique<InterfacePort>();
            port->name = name;
            port->mode = parse_mode(mode_str);
            port->protocol = type;

            if (p.contains("parameters")) {
                for (auto& [k, v] : p["parameters"].items()) {
                    port->parameters[k] = v.dump();
                }
            }

            if (p.contains("port_maps")) {
                for (auto& [k, v] : p["port_maps"].items()) {
                    port->signal_map[k] = v.get<std::string>();
                }
            }

            sys.ports[name] = std::move(port);
        }
    }
}


/* -------------------- Printing -------------------- */

void print_system_ports(const SystemIR& sys) {
    std::cout << "System Interface Ports:\n";
    std::cout << "-----------------------\n";

    for (const auto& [name, port_ptr] : sys.ports) {
        const Port* base = port_ptr.get();

        std::cout << "Port: " << base->name << "\n";
        std::cout << "  type: " << to_string(base->type) << "\n";
        std::cout << "  mode: " << to_string(base->mode) << "\n";

        if (base->type == PortType::Wire) {
            const auto* wp = static_cast<const WirePort*>(base);
            std::cout << "  width: " << wp->width << "\n";
        } else {
            const auto* ip = static_cast<const InterfacePort*>(base);
            std::cout << "  protocol: " << ip->protocol << "\n";

            std::cout << "  parameters:\n";
            for (const auto& [k, v] : ip->parameters) {
                std::cout << "    " << k << " = " << v << "\n";
            }

            std::cout << "  signal map:\n";
            for (const auto& [k, v] : ip->signal_map) {
                std::cout << "    " << k << " -> " << v << "\n";
            }
        }

        std::cout << "\n";
    }
}

/* -------------------- main -------------------- */

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <system.json>\n";
        return 1;
    }

    std::ifstream in(argv[1]);
    if (!in) {
        std::cerr << "Error: cannot open file " << argv[1] << "\n";
        return 1;
    }

    json j;
    in >> j;

    SystemIR system;
    parse_interface_ports(j, system);
    print_system_ports(system);

    return 0;
}