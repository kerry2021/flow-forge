#pragma once
#include "port.hpp"
#include "../third_party/json.hpp"
using json = nlohmann::json;

void parse_interface_ports(const json& j, SystemIR& sys);