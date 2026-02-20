#include "sv_emitter.hpp"
#include "../base/port.hpp"

#include <iostream>
#include <sstream>
#include <fstream>

std::string normalize(std::string s) {
    s.erase(0, s.find_first_not_of(" \t\n\r"));
    s.erase(s.find_last_not_of(" \t\n\r") + 1);
    return s;
}

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

std::string emit_module_instance_sv(
    const Component& comp,    
    const std::unordered_map<std::string, std::string> signal_map
) {
    std::ostringstream ss;

    //find the module name from verilog source path
    std::string module_name;
    {
        std::ifstream in(comp.src_path);
        if (!in) {
            throw std::runtime_error("Unable to open component source " + comp.src_path);
        }
        std::string line;
        while (std::getline(in, line)) {
            std::istringstream iss(line);
            std::string token;
            if (!(iss >> token)) { continue; }
            if (token == "module") {
                if (!(iss >> module_name)) {
                    throw std::runtime_error("Malformed module declaration in " + comp.src_path);
                }
                break;
            }
        }
        if (module_name.empty()) {
            throw std::runtime_error("No module declaration found in " + comp.src_path);
        }
    }

    if(signal_map.size() != 0){
        ss << "\n" << module_name << " (\n";

        bool first = true;
        for (const auto& [port_name, signal_name] : signal_map) {
            if (!first) ss << ",\n";
            first = false;
            ss << "        ." << port_name << "(" << signal_name << ")";
        }

        ss << "\n )" << comp.name << ";\n";
    }

    return ss.str();
}

void populate_conn_map(
    const std::string& comp_instance,
    const Port* comp_port,
    const Port* external_port,
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& comp2sigmap
) {
    if(comp_port->type != external_port->type) {
        throw std::runtime_error("Port type mismatch in connection for instance " + comp_instance);
    }

    if (comp_port->type == PortType::Wire) {
        comp2sigmap[comp_instance][comp_port->name] = external_port->name;
    } else if (comp_port->type == PortType::Interface) {
        const auto& comp_ip = static_cast<const InterfacePort&>(*comp_port);
        const auto& ext_ip = static_cast<const InterfacePort&>(*external_port);

        if (comp_ip.protocol != ext_ip.protocol) {
            throw std::runtime_error("Interface protocol mismatch in connection for instance " + comp_instance);
        }

        for (const auto& [port_name, signal_name] : comp_ip.port_maps) {
            auto ext_name = ext_ip.port_maps.find(port_name);
            if (ext_name == ext_ip.port_maps.end()) {
                throw std::runtime_error("Missing port map entry for " + port_name + " in external interface of instance " + comp_instance);
            }
            auto comp_port_it = comp_ip.port_maps.find(port_name);
            if (comp_port_it == comp_ip.port_maps.end()) {
                throw std::runtime_error("Missing port map entry for " + port_name + " in component interface of instance " + comp_instance);
            }
            comp2sigmap[comp_instance][comp_port_it->second] = ext_name->second;
        }
    } else {
        throw std::runtime_error("Unknown port type in connection for instance " + comp_instance);
    }
}

Port* create_intermediate_port(const Port* src_port, const Port* dst_port, std::vector<std::pair<std::string, int>>* interconnect_signals, const std::unordered_map<std::string, int>* comp_parameters = nullptr) {
    if (src_port->type != dst_port->type) {
        throw std::runtime_error("Port type mismatch in intermediate connection");
    }

    if (src_port->type == PortType::Wire) {
        std::string signal_name = "interconnect_" + std::to_string(interconnect_signals->size());
        int width = 1;
        const auto* wp = static_cast<const WirePort*>(src_port);
        width = wp->width;
        interconnect_signals->emplace_back(signal_name, width);
        WirePort* new_port = (WirePort*)malloc(sizeof(WirePort));
        new_port->name = signal_name;
        new_port->mode = wp->mode;
        new_port->width = width;
        
        return new_port;
        
    } else if (src_port->type == PortType::Interface) {
        const auto* ip = static_cast<const InterfacePort*>(src_port);
        if (ip->protocol == "axi_stream") {
            //iterate through the port maps and create intermediate signals for each
            std::string base_signal_name = "interconnect_" + std::to_string(interconnect_signals->size());
            for (const auto& [port_name, signal_name] : ip->port_maps) {
                std::cout << "Processing port map: " << port_name << " -> " << signal_name << std::endl;
                std::string interconnect_signal_name = base_signal_name + "_" + port_name;
                int width = 1;
                std::string width_str;
                if (port_name == "tdata") {
                    //the tdata width itself might be a parameter, so we find out what parameter it is and look it up in the component parameters if it exists
                    if (ip->parameters.find("tdata_width") != ip->parameters.end()) {
                        width_str = normalize(ip->parameters.at("tdata_width"));
                    } else {
                        throw std::runtime_error("tdata_width parameter not found in interface port");
                    }
                    std::cout << "comp_parameters: " << std::endl;
                    for (const auto& [k, v] : *comp_parameters) {
                        std::cout << "  " << k << " = " << v << " key length: "<< k.length() << std::endl;
                    }
                    std::cout << "width_str: " << "[" << width_str << "]" << "width_str length: " << width_str.length() << std::endl;
                    auto it = comp_parameters->find(width_str);
                    if(it != comp_parameters->end()) {
                        width = it->second;
                    } else {
                        width = std::stoi(width_str);
                    }
                    std::cout << "Determined width for tdata: " << width << std::endl;
                    
                } else if (port_name == "tdest") {
                    //the tdest width itself might be a parameter, so we find out what parameter it is and look it up in the component parameters if it exists                           
                    std::string width_str;
                    if (ip->parameters.find("tdest_width") != ip->parameters.end()) {
                        width_str = normalize(ip->parameters.at("tdest_width"));
                    } else {
                        throw std::runtime_error("tdest_width parameter not found in interface port");
                    }
                    auto it = comp_parameters->find(width_str);
                    if(it != comp_parameters->end()) {
                        width = it->second;
                    } else {
                        width = std::stoi(width_str);
                    }
                    std::cout << "Determined width for tdest: " << width << std::endl;
                }
                interconnect_signals->emplace_back(interconnect_signal_name, width);
            }            
            InterfacePort* new_port = new InterfacePort();
            new_port->name = base_signal_name;
            new_port->mode = ip->mode;
            new_port->protocol = ip->protocol;
            new_port->parameters = ip->parameters;            
            //give new names to the port maps based on the interconnect signal names
            for (const auto& [port_name, signal_name] : ip->port_maps) {
                std::string interconnect_signal_name = base_signal_name + "_" + port_name;
                std::cout << "Mapping port " << port_name << " to intermediate signal " << interconnect_signal_name << std::endl;
                new_port->port_maps[port_name] = interconnect_signal_name;
            }
            std::cout << "Created intermediate interface port: " << new_port->name << " with protocol " << new_port->protocol << std::endl;
            return new_port;
        } else {
            throw std::runtime_error("Unsupported interface protocol in intermediate connection");
        }
    } else {
        throw std::runtime_error("Unknown port type in intermediate connection");
        return nullptr;
    }
    
    
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
    ss << "\n);\n\n";

    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> comp2sigmap;
    std::vector<std::pair<std::string, int>> interconnect_signals; // (signal_name, width)

    // Populate signal maps for each component instance
    for (const auto& conn : sys.connections) {
        // src and dst are already resolved to actual Port* in parse_connections
        const Port* src_port = conn.src.port_ptr;
        const Port* dst_port = conn.dst.port_ptr;
        if (src_port == nullptr || dst_port == nullptr) {
            throw std::runtime_error("Unresolved endpoint in connection: " + conn.name);
        }
        // Determine which is the component port and which is the top-level port
        if (conn.src.instance == "this") {
            // src is top-level, dst is component port
            populate_conn_map(conn.dst.instance, conn.dst.port_ptr, conn.src.port_ptr, comp2sigmap);
        } else if (conn.dst.instance == "this") {
            // dst is top-level, src is component port
            populate_conn_map(conn.src.instance, conn.src.port_ptr, conn.dst.port_ptr, comp2sigmap);
        } else {
            Port* intermediate_port = create_intermediate_port(src_port, dst_port, &interconnect_signals, &(sys.components.at(conn.src.instance).parameters));
            populate_conn_map(conn.src.instance, conn.src.port_ptr, intermediate_port, comp2sigmap);
            populate_conn_map(conn.dst.instance, conn.dst.port_ptr, intermediate_port, comp2sigmap);
            std::cout << "here";
        }
    }
    
    // Emit intermediate signals for connections between component ports
    bool first_signal = true;
    for (const auto& [signal_name, width] : interconnect_signals) {
        if (!first_signal) ss << ",\n";
        first_signal = false;
        ss << "    logic";
        if (width > 1) {
            ss << " [" << width << "-1:0]";
        }
        ss << " " << signal_name;
    }

    // Emit module instances
    for (const auto& [comp_name, comp] : sys.components) {
        const auto& sig_map = comp2sigmap[comp_name];
        ss << emit_module_instance_sv(comp, sig_map);
    }

    ss << "\nendmodule\n";

    return ss.str();
}