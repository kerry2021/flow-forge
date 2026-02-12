#pragma once

#include <unordered_map>
#include <memory>
#include "port.hpp"
#include "component.hpp"

struct SystemIR {
    std::unordered_map<std::string, std::unique_ptr<Port>> ports;
    std::unordered_map<std::string, Component> components;
};