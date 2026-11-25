#pragma once

#include <IntermediateRepresentation/IrProgram.hpp>

namespace AnyBox::Shader {
void MarkNativeBufferDescriptors(ShaderRecompiler::IrProgram& program);
}
