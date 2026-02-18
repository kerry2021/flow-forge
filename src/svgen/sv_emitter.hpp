#pragma once

#include <string>
#include "../base/system_ir.hpp"

// Emit the SV for the top module.  The implementation handles
//   * top-level port declarations
//   * intermediate wires needed for connections between components
//   * instantiation of each component with parameter overrides and
//     expanded port bindings
std::string emit_top_module_sv(
    const SystemIR& sys,
    const std::string& module_name
);
