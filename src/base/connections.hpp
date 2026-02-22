#pragma once

#include <string>
#include <vector>
#include <ostream>
#include "port.hpp"

struct EndpointRef {
    std::string instance;  // "this", "test_master_0", etc.
    std::string port;      // "clk", "s_axis", etc.

    // pointer to the actual Port object; populated during connection
    // resolution.  nullptr for unresolved or top-level port until resolved.
    Port* port_ptr = nullptr;

    static EndpointRef parse(const std::string& s);
};

struct Connection {
    std::string name;
    EndpointRef src;
    std::vector<EndpointRef> dsts;
};


std::ostream& operator<<(std::ostream& os, const EndpointRef& ep);
std::ostream& operator<<(std::ostream& os, const Connection& conn);