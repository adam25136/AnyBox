#pragma once

namespace LLVMBC { class CallInst; }
namespace ShaderRecompiler { class IrBuilder; class IrValue; }

namespace AnyBox::Shader::Dxil {
class ScalarLowering;
ShaderRecompiler::IrValue& LowerBitCount64(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, ShaderRecompiler::IrBuilder& builder);
}
