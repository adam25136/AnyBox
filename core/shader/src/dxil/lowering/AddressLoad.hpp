#pragma once

namespace LLVMBC { class CallInst; }
namespace ShaderRecompiler { class IrBuilder; class IrValue; class IrProgram; }

namespace AnyBox::Shader::Dxil {
class ScalarLowering;
bool IsUniformAddress(const ShaderRecompiler::IrValue& value, const ShaderRecompiler::IrProgram& program);
ShaderRecompiler::IrValue& LowerAddressLoad(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, ShaderRecompiler::IrBuilder& builder, ShaderRecompiler::IrProgram& program, unsigned words, unsigned expectedOpcode);
}
