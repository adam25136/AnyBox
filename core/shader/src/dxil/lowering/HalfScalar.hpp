#pragma once

namespace LLVMBC { class Value; class CastInst; }
namespace ShaderRecompiler { class IrBuilder; class IrValue; }

namespace AnyBox::Shader::Dxil {
class ScalarLowering;
bool IsHalf(const LLVMBC::Value& value);
ShaderRecompiler::IrValue& LowerHalfCast(const LLVMBC::CastInst& instruction, ScalarLowering& scalar, ShaderRecompiler::IrBuilder& builder);
}
