#include "connections.hpp"
#include "component.hpp"
#include "parser.hpp"
#include <iostream>
#include <fstream>

// charge an arbitrary port map from JSON.  The caller supplies the
// destination container so the same logic may be used for the top-level
// system ports and for component specifications.
static void populate_port_map(const json& j,
                              std::unordered_map<std::string, std::unique_ptr<Port>>& dest) {
    if (!j.contains("interface_ports"))
        return;

    for (const auto& p : j.at("interface_ports")) {
        const std::string name = p.at("name").get<std::string>();
        const std::string type = p.at("type").get<std::string>();
        const std::string mode_str = p.at("mode").get<std::string>();

        if (type == "wire") {
            auto port = std::make_unique<WirePort>();
            port->name = name;
            port->mode = parse_mode(mode_str);
            port->width = p.value("width", 1u);
            dest[name] = std::move(port);
        } else {
            auto port = std::make_unique<InterfacePort>();
            port->name = name;
            port->mode = parse_mode(mode_str);
            port->protocol = type;

            if (p.contains("parameters")) {
                for (auto& [k, v] : p["parameters"].items()) {
                    port->parameters[k] = v.is_number() ? std::to_string(v.get<int>()) : v.get<std::string>();
                }
            }

            if (p.contains("port_maps")) {
                for (auto& [k, v] : p["port_maps"].items()) {
                    port->port_maps[k] = v.get<std::string>();
                }
            }

            dest[name] = std::move(port);
        }
    }
}

void parse_interface_ports(const json& j, SystemIR& sys) {
    populate_port_map(j, sys.ports);
}

void parse_components(const json& j, SystemIR& sys) {
    if (!j.contains("components"))
        return;

    for (const auto& c : j.at("components")) {
        Component comp;

        comp.name      = c.at("name").get<std::string>();
        comp.spec_path = c.at("spec_path").get<std::string>();
        comp.src_path  = c.at("src_path").get<std::string>();

        if (c.contains("parameters")) {
            for (const auto& [k, v] : c["parameters"].items()) {
                comp.parameters[k] = std::stoi(v.dump());
            }
        }

        // load the spec file itself so we know what ports this instance has
        {
            std::ifstream in(comp.spec_path);
            if (!in) {
                throw std::runtime_error("Unable to open component spec " + comp.spec_path);
            }
            json spec;
            in >> spec;
            populate_port_map(spec, comp.ports);
        }

        // Sanity: no duplicate instance names
        if (sys.components.count(comp.name)) {
            throw std::runtime_error(
                "Duplicate component name: " + comp.name
            );
        }

        sys.components.emplace(comp.name, std::move(comp));
    }
}

// helper used after all components/ports are in place to resolve endpoints
static Port* resolve_endpoint(const SystemIR& sys, EndpointRef& ep) {
    if (ep.instance == "this") {
        auto it = sys.ports.find(ep.port);
        if (it == sys.ports.end()) {
            throw std::runtime_error("No such top-level port: " + ep.port);
        }
        return it->second.get();
    }

    auto cit = sys.components.find(ep.instance);
    if (cit == sys.components.end()) {
        throw std::runtime_error("Unknown component instance: " + ep.instance);
    }
    auto pit = cit->second.ports.find(ep.port);
    if (pit == cit->second.ports.end()) {
        throw std::runtime_error("Component '" + ep.instance + "' has no port '" + ep.port + "'");
    }
    return pit->second.get();
}

void parse_connections(const json& j, SystemIR& sys) {
    if (!j.contains("connections")) {
        return;
    }

    for (const auto& jc : j.at("connections")) {
        Connection c;
        c.name = jc.at("name").get<std::string>();

        const auto& destinations = jc.at("dsts");

        c.src = EndpointRef::parse(jc.at("src").get<std::string>());
        c.dsts = std::vector<EndpointRef>{};
        for (const auto& dst_str : destinations) {
            c.dsts.push_back(EndpointRef::parse(dst_str.get<std::string>()));
        }

        // resolve the port pointers immediately
        c.src.port_ptr = resolve_endpoint(sys, c.src);
        for (auto& dst : c.dsts) {
            dst.port_ptr = resolve_endpoint(sys, dst);
        }

        sys.connections.push_back(std::move(c));
    }
}
