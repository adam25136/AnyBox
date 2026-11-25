#pragma once

namespace LLVMBC { class CallInst; }
namespace ShaderRecompiler { class IrValue; class IrBuilder; }

namespace AnyBox::Shader::Dxil {
class ScalarLowering;
ShaderRecompiler::IrValue& HalfExtremumBits(ShaderRecompiler::IrBuilder& builder, ShaderRecompiler::IrValue& left, ShaderRecompiler::IrValue& right, bool maximum);
ShaderRecompiler::IrValue& LowerHalfExtremum(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, ShaderRecompiler::IrBuilder& builder);
ShaderRecompiler::IrValue& LowerPackedHalfExtremum(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, ShaderRecompiler::IrBuilder& builder, bool maximum);
}
