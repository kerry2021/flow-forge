#pragma once

#include <string>
#include <vector>
#include <ostream>

struct EndpointRef {
    std::string instance;  // "this", "test_master_0", etc.
    std::string port;      // "clk", "s_axis", etc.

    static EndpointRef parse(const std::string& s);
};

struct Connection {
    std::string name;
    EndpointRef src;
    EndpointRef dst;
};

std::ostream& operator<<(std::ostream& os, const EndpointRef& ep);
std::ostream& operator<<(std::ostream& os, const Connection& conn);