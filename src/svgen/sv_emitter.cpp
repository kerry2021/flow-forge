#include "sv_emitter.hpp"
#include "../base/port.hpp"

#include <iostream>
#include <sstream>
#include <fstream>

static std::string emit_wire_port(const WirePort& p) {
    std::ostringstream ss;
    ss << "    " << to_string(p.mode) << " logic " << p.name;
    return ss.str();
}

static std::string emit_axi_stream_port(const InterfacePort& p) {
    std::ostringstream ss;
    const bool is_slave = (p.mode == PortMode::Slave);

    auto dir = [&](const std::string& sig) {
        if (sig == "tready")
            return is_slave ? "output" : "input";
        return is_slave ? "input" : "output";
    };

    bool first = true;
    for (const auto& [axi_sig, sv_name] : p.port_maps) {
        if (!first) ss << ",\n";
        first = false;

        ss << "    " << dir(axi_sig) << " logic";

        if (axi_sig == "tdata") {
            ss << " [" << p.parameters.at("tdata_width") << "-1:0]";
        } else if (axi_sig == "tdest") {
            ss << " [" << p.parameters.at("tdest_width") << "-1:0]";
        }

        ss << " " << sv_name;
    }

    return ss.str();
}

std::string emit_top_module_sv(
    const SystemIR& sys,
    const std::string& module_name
) {
    std::ostringstream ss;

    ss << "module " << module_name << " (\n";

    bool first = true;

    for (const auto& kv : sys.ports) {
        if (!first) ss << ",\n";
        first = false;

        const Port* p = kv.second.get(); // assuming sys.ports stores unique_ptr<Port>

        if (auto wp = dynamic_cast<const WirePort*>(p)) {
            ss << emit_wire_port(*wp);       // safe: wp is really a WirePort
        } 
        else if (auto ip = dynamic_cast<const InterfacePort*>(p)) {
            ss << emit_axi_stream_port(*ip); // safe: ip is really an InterfacePort
        } 
        else {
            // optional: unknown type
            throw std::runtime_error("Unknown Port subclass");
        }
    }

    ss << "\n);\n\nendmodule\n";

    return ss.str();
}