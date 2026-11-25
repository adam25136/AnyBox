#pragma once

#include <array>

namespace LLVMBC { class Value; }
namespace ShaderRecompiler { class IrBuilder; class IrValue; }

namespace AnyBox::Shader::Dxil {
class ScalarLowering;
std::array<ShaderRecompiler::IrValue*, 2> ReadWordPair(const LLVMBC::Value& source, ScalarLowering& scalar, ShaderRecompiler::IrBuilder& builder);
}
