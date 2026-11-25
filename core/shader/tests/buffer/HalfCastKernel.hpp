#pragma once

#include "AtomicKernel.hpp"

namespace BufferTests {
inline ShaderRecompiler::IrProgram MakeHalfCastKernel(bool narrow) {
    using namespace ShaderRecompiler;
    AtomicKernel fixture;
    auto& context = fixture.source.Context();
    auto& builder = fixture.builder;
    auto& index = fixture.GlobalIndex();
    auto& offset = builder.ShiftLeftLogical(index, builder.Constant(2));
    auto& loaded = fixture.Memory(IrOpcode::LoadAddressU32, offset, 1);
    auto* half = LLVMBC::Type::getHalfTy(context);
    auto* floating = LLVMBC::Type::getFloatTy(context);
    auto* integer = LLVMBC::Type::getInt16Ty(context);
    auto* argument = context.construct<LLVMBC::Argument>(narrow ? floating : integer, 0, false);
    fixture.scalar.Bind(*argument, narrow ? builder.BitCastF32(loaded) : loaded);
    auto* converted = context.construct<LLVMBC::CastInst>(half, argument, narrow ? LLVMBC::Instruction::FPTrunc : LLVMBC::Instruction::BitCast);
    auto* widened = context.construct<LLVMBC::CastInst>(floating, converted, LLVMBC::Instruction::FPExt);
    auto* bits = context.construct<LLVMBC::CastInst>(integer, converted, LLVMBC::Instruction::BitCast);
    auto* negated = context.construct<LLVMBC::UnaryOperator>(LLVMBC::UnaryOperator::UnaryOps::FNeg, converted);
    auto* condition = context.construct<LLVMBC::Argument>(LLVMBC::Type::getInt1Ty(context), 1, false);
    fixture.scalar.Bind(*condition, builder.INotEqual(builder.BitwiseAnd(index, builder.Constant(1)), builder.Constant(0)));
    auto* selected = context.construct<LLVMBC::SelectInst>(negated, converted, condition);
    auto* selectedBits = context.construct<LLVMBC::CastInst>(integer, selected, LLVMBC::Instruction::BitCast);
    auto canonical = [&](IrValue& value) -> IrValue& {
        auto& magnitude = builder.BitwiseAnd(value, builder.Constant(0x7FFF));
        return builder.Select(builder.UGreaterThan(magnitude, builder.Constant(0x7C00)), builder.Constant(0x7E00), value);
    };
    auto& raw = fixture.scalar.Get(*bits);
    auto& wide = fixture.scalar.Get(*widened);
    auto& chosen = fixture.scalar.Get(*selectedBits);
    auto& nan = builder.Emit(IrOpcode::FPIsNan32, IrType::Bool, {&wide});
    auto& base = builder.IMul(index, builder.Constant(12));
    fixture.Memory(IrOpcode::StoreAddressU32, base, 2, narrow ? &canonical(raw) : &raw);
    auto& wideBits = builder.Select(nan, builder.Constant(0x7FC00000), builder.BitCastU32(wide));
    fixture.Memory(IrOpcode::StoreAddressU32, builder.IAdd(base, builder.Constant(4)), 2, &wideBits);
    fixture.Memory(IrOpcode::StoreAddressU32, builder.IAdd(base, builder.Constant(8)), 2, narrow ? &canonical(chosen) : &chosen);
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    fixture.program.Metadata().blockInfo[0].terminator.kind = TerminatorKind::Return;
    return std::move(fixture.program);
}
}
