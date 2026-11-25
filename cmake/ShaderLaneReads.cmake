function(anybox_adapt_shader_lane_reads target root)
    set(original "${root}/SpirvBackend/src/SpirvAnalysis.cpp")
    file(READ "${original}" content)
    anybox_replace_shader_text(content
        "bool UniformSource(const IrValue& value) {\n    switch (value.Opcode()) {"
        "bool UniformSource(const IrValue& value) {\n    switch (value.Opcode()) {\n    case IrOpcode::ReadLane: return value.Flags<std::uint64_t>() == 0u;"
    )
    anybox_replace_shader_text(content
        "    case IrOpcode::ReadLane:\n    case IrOpcode::GetUserData:"
        "    case IrOpcode::GetUserData:"
    )
    anybox_replace_shader_text(content
        "for (std::size_t index = 0; !lanes && !UniformSource(*inst) && index < inst->ArgumentCount(); index++)"
        "for (std::size_t index = inst->Opcode() == IrOpcode::ReadLane ? 1u : 0u; !lanes && !UniformSource(*inst) && index < inst->ArgumentCount(); index++)"
    )
    anybox_overlay_shader_source(${target} "${original}" "${content}")
endfunction()
