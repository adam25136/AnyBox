#include "FloatBits.hpp"
#include <IntermediateRepresentation/IrBuilder.hpp>

using namespace ShaderRecompiler;

IrValue& AnyBox::Shader::Dxil::ReciprocalSqrt32(IrBuilder& builder, IrValue& value) {
    auto& bits = builder.BitCastU32(value);
    auto& magnitude = builder.BitwiseAnd(bits, builder.Constant(0x7FFFFFFF));
    auto& sign = builder.BitwiseAnd(bits, builder.Constant(0x80000000U));
    auto& zero = builder.IEqual(magnitude, builder.Constant(0));
    auto& positive = builder.ULessThan(bits, builder.Constant(0x7F800000));
    auto& valid = builder.LogicalAnd(positive, builder.LogicalNot(zero));
    auto& one = builder.BitCastF32(builder.Constant(0x3F800000));
    auto& operand = builder.Select(valid, value, one);
    auto& result = builder.Emit(IrOpcode::FPRecipSqrt32, IrType::F32, {&operand});
    auto& infinity = builder.BitCastF32(builder.BitwiseOr(sign, builder.Constant(0x7F800000)));
    auto& positiveInfinity = builder.IEqual(bits, builder.Constant(0x7F800000));
    auto& nan = builder.BitCastF32(builder.Constant(0x7FC00000));
    auto& special = builder.Select(positiveInfinity, builder.BitCastF32(builder.Constant(0)), nan);
    return builder.Select(zero, infinity, builder.Select(valid, result, special));
}
