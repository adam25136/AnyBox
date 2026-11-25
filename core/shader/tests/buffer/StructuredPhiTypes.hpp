#pragma once

#include <IntermediateRepresentation/IrBuilder.hpp>

namespace BufferTests {
inline ShaderRecompiler::IrValue& PhiPayload(ShaderRecompiler::IrBuilder& builder, ShaderRecompiler::IrValue& value, ShaderRecompiler::IrType type) {
    using namespace ShaderRecompiler;
    switch (type) {
        case IrType::U32: return value;
        case IrType::Bool: return builder.INotEqual(builder.BitwiseAnd(value, builder.Constant(1)), builder.Constant(0));
        case IrType::U8: return builder.Emit(IrOpcode::ConvertU8U32, type, {&value});
        case IrType::U16: return builder.Emit(IrOpcode::ConvertU16U32, type, {&value});
        case IrType::F32: return builder.BitCastF32(value);
        case IrType::U64: return builder.ConstructU64(value, builder.IMul(value, builder.Constant(17)));
        default: throw std::logic_error("Structured PHI fixture type is unsupported");
    }
}

inline ShaderRecompiler::IrValue& PhiWord(ShaderRecompiler::IrBuilder& builder, ShaderRecompiler::IrValue& value) {
    using namespace ShaderRecompiler;
    switch (value.Type()) {
        case IrType::U32: return value;
        case IrType::Bool: return builder.Select(value, builder.Constant(1), builder.Constant(0));
        case IrType::U8: return builder.Emit(IrOpcode::ConvertU32U8, IrType::U32, {&value});
        case IrType::U16: return builder.Emit(IrOpcode::ConvertU32U16, IrType::U32, {&value});
        case IrType::F32: return builder.BitCastU32(value);
        case IrType::U64: return builder.CompositeExtract(value, 0);
        default: throw std::logic_error("Structured PHI result type is unsupported");
    }
}
}
