#include <iostream>
#include <fstream>
#include "base/port.hpp"
#include "base/parser.hpp"
#include "third_party/json.hpp"
using json = nlohmann::json;


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