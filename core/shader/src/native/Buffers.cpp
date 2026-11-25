#include "Buffers.hpp"
#include <Recompiler.hpp>

void AnyBox::Shader::MarkNativeBufferDescriptors(ShaderRecompiler::IrProgram& program) {
    using namespace ShaderRecompiler;
    for (const auto& block : program.Blocks()) {
        for (auto* value : block->Instructions()) {
            if (BufferAccessOf(value->Opcode()) == BufferAccess::None) continue;
            auto& memory = program.Resources().memoryInfo.at(value->Flags<MemoryFlags>().index);
            if (memory.kind == ResourceKind::Buffer || memory.kind == ResourceKind::ScalarBuffer) memory.gpuDescriptor = true;
        }
    }
}
