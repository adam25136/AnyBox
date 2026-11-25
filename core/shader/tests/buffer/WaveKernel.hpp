#pragma once

#include "FloatFixture.hpp"

namespace BufferTests {
inline ShaderRecompiler::IrProgram MakeWaveKernel(std::uint64_t mask, unsigned waveSize, unsigned operation = 0, bool dxilFlow = false) {
    using namespace ShaderRecompiler;
    ResourceTests::Fixture source;
    auto& context = source.Context();
    auto* word = LLVMBC::Type::getInt32Ty(context);
    auto* boolean = LLVMBC::Type::getInt1Ty(context);
    auto* floating = LLVMBC::Type::getFloatTy(context);
    bool counting = operation == 135;
    auto* byte = LLVMBC::Type::getInt8Ty(context);
    auto* argument = context.construct<LLVMBC::Argument>(floating, 0, false);
    auto integer = [&](LLVMBC::Type* type, unsigned value) { return LLVMBC::ConstantInt::get(type, value); };
    source.Module().add_value_name(110, counting ? "dx.op.waveAllOp" : operation == 0 ? "dx.op.waveIsFirstLane" : "dx.op.waveActiveOp.f32");
    auto parameters = operation == 0 ? LLVMBC::Vector<LLVMBC::Type*>{word} : LLVMBC::Vector<LLVMBC::Type*>{word, floating, byte, byte};
    auto arguments = operation == 0 ? LLVMBC::Vector<LLVMBC::Value*>{integer(word, 110)} : LLVMBC::Vector<LLVMBC::Value*>{integer(word, 119), argument, integer(byte, operation), integer(byte, 0)};
    if (counting) {
        parameters = {word, boolean};
        arguments = {integer(word, 135), integer(boolean, 1)};
    }
    auto* signature = context.construct<LLVMBC::FunctionType>(context, counting ? word : operation == 0 ? boolean : floating, std::move(parameters));
    auto* function = context.construct<LLVMBC::Function>(signature, 110, source.Module());
    auto* call = context.construct<LLVMBC::CallInst>(signature, function, std::move(arguments));
    IrProgram program;
    program.SetWaveSize(waveSize);
    auto& entry = program.CreateBlock();
    auto& body = program.CreateBlock();
    auto& exit = program.CreateBlock();
    program.SetEntryBlock(entry);
    program.BlockOrder() = {&entry, &body, &exit};
    if (!dxilFlow) {
        entry.AddBranch(&body);
        entry.AddBranch(&exit);
        body.AddBranch(&exit);
    }
    auto& metadata = program.Metadata().blockInfo;
    metadata.resize(3);
    for (unsigned index = 0; index < 3; ++index) metadata[index].id = index;
    metadata[0].terminator.kind = TerminatorKind::ConditionalBranch;
    metadata[0].terminator.condition = BranchCondition::ScalarInstruction;
    metadata[0].terminator.trueBlock = 1;
    metadata[0].terminator.falseBlock = metadata[0].terminator.mergeBlock = 2;
    metadata[1].terminator.kind = TerminatorKind::Branch;
    metadata[1].terminator.trueBlock = 2;
    IrBuilder builder(program);
    builder.SetInsertionPoint(entry);
    auto& lane = builder.Emit(IrOpcode::LaneId, IrType::U32, {});
    auto& low = builder.Constant(static_cast<std::uint32_t>(mask));
    auto& high = builder.Constant(static_cast<std::uint32_t>(mask >> 32));
    auto& selected = builder.Select(builder.ULessThan(lane, builder.Constant(32)), low, high);
    auto& shift = builder.BitwiseAnd(lane, builder.Constant(31));
    auto& active = builder.INotEqual(builder.BitwiseAnd(builder.ShiftRightLogical(selected, shift), builder.Constant(1)), builder.Constant(0));
    metadata[0].condition = &active;
    if (!dxilFlow) (void)builder.Emit(IrOpcode::Reference, IrType::Void, {&active});
    auto& global = builder.Emit(IrOpcode::GetBuiltin, IrType::U32, {&builder.Constant(static_cast<unsigned>(StageInputKind::GlobalInvocationId)), &builder.Constant(0)});
    if (dxilFlow) builder.BranchConditional(active, body, exit);
    builder.SetInsertionPoint(body);
    AnyBox::Shader::Dxil::ScalarLowering scalar(program, builder);
    if (operation != 0 && !counting) {
        auto& inputOffset = builder.ShiftLeftLogical(builder.BitwiseAnd(lane, builder.Constant(15)), builder.Constant(2));
        auto& inputAddress = builder.Emit(IrOpcode::GetAddressResource, IrType::AddressResource, {&inputOffset, &builder.Constant(1)});
        MemoryInfo inputMemory;
        inputMemory.kind = ResourceKind::Global;
        inputMemory.addressIsFull = true;
        program.Resources().memoryInfo.push_back(inputMemory);
        auto& loaded = builder.Emit(IrOpcode::LoadAddressU32, IrType::U32, {&inputAddress, &inputOffset, &builder.Constant(1), &active}, MemoryFlags{0, 0});
        scalar.Bind(*argument, builder.BitCastF32(loaded));
    }
    auto& value = scalar.Get(*call);
    auto& result = counting ? value : operation == 0 ? builder.Select(value, builder.Constant(1), builder.Constant(0)) : builder.BitCastU32(value);
    auto& offset = builder.ShiftLeftLogical(global, builder.Constant(2));
    auto& address = builder.Emit(IrOpcode::GetAddressResource, IrType::AddressResource, {&offset, &builder.Constant(2)});
    MemoryInfo memory;
    memory.kind = ResourceKind::Global;
    memory.addressIsFull = true;
    program.Resources().memoryInfo.push_back(memory);
    (void)builder.Emit(IrOpcode::StoreAddressU32, IrType::Void, {&address, &offset, &builder.Constant(2), &result, &active}, MemoryFlags{static_cast<std::uint32_t>(program.Resources().memoryInfo.size() - 1), 0});
    if (dxilFlow) {
        builder.Branch(exit);
        builder.SetInsertionPoint(exit);
        metadata[2].terminator.kind = TerminatorKind::Return;
        (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    }
    ValidateProgram(program, true);
    return program;
}
}
