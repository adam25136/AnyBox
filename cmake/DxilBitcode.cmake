function(anybox_adapt_dxil_bitcode)
    set(original "${PROJECT_SOURCE_DIR}/3rdparty/dxil-spirv/bc/module.cpp")
    file(READ "${original}" content)
    set(needle "\tcase FunctionRecord::INST_EXTRACTVAL:")
    set(insertion [=[
    case FunctionRecord::INST_INSERTVAL: {
        unsigned index = 0;
        auto aggregate = get_value_and_type(entry.ops, index);
        auto inserted = get_value_and_type(entry.ops, index);
        if (!aggregate.first || !inserted.first || index >= entry.ops.size()) return false;
        auto indices = std::span<const std::uint64_t>(entry.ops.data(), entry.ops.size()).subspan(index);
        auto* value = AnyBox::Shader::Dxil::Bitcode::InsertValue(*context, *aggregate.first, *inserted.first, indices);
        auto* instruction = dyn_cast<CompositeConstructInst>(value);
        if (!instruction || !add_instruction(instruction)) return false;
        break;
    }
]=])
    anybox_replace_shader_text(content "#include \"module.hpp\"" "#include \"module.hpp\"\n#include \"AnyBoxInsertValue.hpp\"")
    anybox_replace_shader_text(content "${needle}" "${insertion}${needle}")
    get_target_property(sources llvm-bc SOURCES)
    if(NOT "module.cpp" IN_LIST sources)
        message(FATAL_ERROR "The pinned DXIL parser does not expose module.cpp")
    endif()
    list(REMOVE_ITEM sources module.cpp)
    set_property(TARGET llvm-bc PROPERTY SOURCES "${sources}")
    set(output "${CMAKE_BINARY_DIR}/generated/bitcode/module.cpp")
    file(GENERATE OUTPUT "${output}" CONTENT "${content}")
    set_source_files_properties("${output}" TARGET_DIRECTORY llvm-bc PROPERTIES GENERATED TRUE)
    target_sources(llvm-bc PRIVATE "${output}")
    target_compile_features(llvm-bc PRIVATE cxx_std_20)
    target_include_directories(llvm-bc PRIVATE "${PROJECT_SOURCE_DIR}/core/shader/src/dxil/bitcode")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${original}")
endfunction()
