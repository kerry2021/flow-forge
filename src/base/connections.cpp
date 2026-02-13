#include "connections.hpp"

#include <sstream>
#include <stdexcept>

EndpointRef EndpointRef::parse(const std::string& s) {
    auto pos = s.find('.');
    if (pos == std::string::npos) {
        throw std::runtime_error("Invalid endpoint format: " + s);
    }

    EndpointRef ep;
    ep.instance = s.substr(0, pos);
    ep.port = s.substr(pos + 1);
    return ep;
}

std::ostream& operator<<(std::ostream& os, const EndpointRef& ep) {
    os << ep.instance << "." << ep.port;
    return os;
}

std::ostream& operator<<(std::ostream& os, const Connection& conn) {
    os << "Connection(name=" << conn.name
       << ", src=" << conn.src
       << ", dst=" << conn.dst
       << ")";
    return os;
}