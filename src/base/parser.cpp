#include "parser.hpp"
#include <iostream>

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
        } else {
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