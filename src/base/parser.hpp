#pragma once

#include "system_ir.hpp"
#include "../third_party/json.hpp"
using json = nlohmann::json;

void parse_interface_ports(const json& j, SystemIR& sys);
void parse_components(const json& j, SystemIR& sys);