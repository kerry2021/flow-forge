#pragma once

#include <string>
#include <unordered_map>

// Represents an instantiated component in the system
struct Component {
    std::string name;       // instance name
    std::string spec_path; // path to component spec JSON
    std::string src_path;  // path to SystemVerilog source

    // Parameter overrides at instantiation
    // Keep values as strings to allow symbolic / numeric
    std::unordered_map<std::string, std::string> parameters;
};