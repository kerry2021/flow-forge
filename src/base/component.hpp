#pragma once

#include <string>
#include <unordered_map>

// Represents an instantiated component in the system
struct Component {
    std::string name;       // instance name
    std::string spec_path;  // path to component spec JSON
    std::string src_path;   // path to SystemVerilog source

    // Parameter overrides at instantiation
    // Keep values as strings to allow symbolic / numeric
    std::unordered_map<std::string, int> parameters;

    // ports defined by the component's specification
    // filled in when the spec file is parsed
    std::unordered_map<std::string, std::unique_ptr<Port>> ports;
};