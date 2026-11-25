#pragma once

#include "AtomicKernel.hpp"

namespace BufferTests {
inline ShaderRecompiler::IrProgram PredicationKernel(unsigned conditionMode, unsigned branchMode, bool load = false, unsigned diagnostic = 0, unsigned phiKind = 0, bool bitCount = false) {
    using namespace ShaderRecompiler;
    AtomicKernel fixture;
    auto& program = fixture.program;
    auto& builder = fixture.builder;
    program.SetWaveSize(64);
    auto& entry = program.EntryBlock();
    auto* positive = branchMode == 1 ? nullptr : &program.CreateBlock();
    auto* negative = branchMode == 0 ? nullptr : &program.CreateBlock();
    auto& merge = program.CreateBlock();
    program.BlockOrder() = {&entry};
    if (positive) program.BlockOrder().push_back(positive);
    if (negative) program.BlockOrder().push_back(negative);
    program.BlockOrder().push_back(&merge);
    auto& metadata = program.Metadata().blockInfo;
    metadata.resize(program.Blocks().size());
    for (auto* block : program.BlockOrder()) metadata[block->Id()].id = block->Id();
    auto& global = fixture.GlobalIndex();
    auto& local = builder.Emit(IrOpcode::GetBuiltin, IrType::U32, {&builder.Constant(static_cast<unsigned>(StageInputKind::LocalInvocationIndex)), &builder.Constant(0)});
    auto& lane = builder.BitwiseAnd(local, builder.Constant(63));
    IrValue* condition;
    if (conditionMode == 0) condition = &builder.INotEqual(builder.BitwiseAnd(global, builder.Constant(1)), builder.Constant(0));
    else if (conditionMode == 1) condition = &builder.IEqual(lane, builder.Constant(63));
    else if (conditionMode == 2) condition = &builder.ULessThan(lane, builder.Constant(32));
    else {
        auto& group = builder.Emit(IrOpcode::GetBuiltin, IrType::U32, {&builder.Constant(static_cast<unsigned>(StageInputKind::WorkgroupId)), &builder.Constant(0)});
        condition = &builder.INotEqual(builder.BitwiseAnd(group, builder.Constant(1)), builder.Constant(0));
    }
    metadata[0].condition = condition;
    metadata[0].terminator.kind = TerminatorKind::ConditionalBranch;
    metadata[0].terminator.condition = BranchCondition::ScalarInstruction;
    metadata[0].terminator.trueBlock = positive ? positive->Id() : merge.Id();
    metadata[0].terminator.falseBlock = negative ? negative->Id() : merge.Id();
    auto convert = [&](IrValue& value) -> IrValue& {
        if (phiKind == 1) return builder.INotEqual(builder.BitwiseAnd(value, builder.Constant(1)), builder.Constant(0));
        return phiKind == 2 ? builder.BitCastF32(value) : value;
    };
    auto* fallback = &convert(builder.Constant(0xDEADBEEF));
    builder.BranchConditional(*condition, positive ? *positive : merge, negative ? *negative : merge);
    IrValue* positiveValue = fallback;
    IrValue* negativeValue = fallback;
    for (auto* block : {positive, negative}) {
        if (!block) continue;
        builder.SetInsertionPoint(*block);
        auto* value = &builder.IAdd(builder.IMul(global, builder.Constant(17)), builder.Constant(5));
        if (load) {
            auto& valid = builder.IMul(builder.BitwiseAnd(global, builder.Constant(15)), builder.Constant(4));
            auto& offset = block == positive ? builder.Select(*condition, valid, builder.Constant(0xFFFFFFFC)) : builder.Select(*condition, builder.Constant(0xFFFFFFFC), valid);
            value = &builder.IAdd(fixture.Memory(IrOpcode::LoadAddressU32, offset, 1), global);
        }
        if (block == negative) value = &builder.BitwiseXor(*value, builder.Constant(0xF0C33CAB));
        if (bitCount) {
            auto& bodyLane = builder.Emit(IrOpcode::LaneId, IrType::U32, {});
            auto& wide = builder.ConstructU64(*value, bodyLane);
            value = &builder.Emit(IrOpcode::BitCount64, IrType::U32, {&wide});
        }
        if (diagnostic) {
            auto& inactive = diagnostic == 2 ? builder.ConstantBool(true) : block == positive ? builder.LogicalNot(*condition) : *condition;
            (void)builder.Emit(IrOpcode::DescriptorFault, IrType::Void, {&inactive, &builder.Constant(0), &builder.Constant(0), &builder.Constant(0)});
            program.Info().usesFaultBuffer = true;
        }
        fixture.Memory(IrOpcode::StoreAddressU32, builder.IMul(global, builder.Constant(8)), 2, value);
        auto& converted = convert(*value);
        if (block == positive) positiveValue = &converted;
        else negativeValue = &converted;
        builder.Branch(merge);
        metadata[block->Id()].terminator.kind = TerminatorKind::Branch;
        metadata[block->Id()].terminator.trueBlock = merge.Id();
    }
    builder.SetInsertionPoint(merge);
    auto& phi = builder.Emit(IrOpcode::Phi, phiKind == 1 ? IrType::Bool : phiKind == 2 ? IrType::F32 : IrType::U32, {});
    phi.AddPhiOperand(positive ? positive : &entry, positiveValue);
    phi.AddPhiOperand(negative ? negative : &entry, negativeValue);
    auto& result = phiKind == 1 ? builder.Select(phi, builder.Constant(1), builder.Constant(0)) : phiKind == 2 ? builder.BitCastU32(phi) : phi;
    fixture.Memory(IrOpcode::StoreAddressU32, builder.IAdd(builder.IMul(global, builder.Constant(8)), builder.Constant(4)), 2, &result);
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    metadata[merge.Id()].terminator.kind = TerminatorKind::Return;
    return std::move(program);
}
}
