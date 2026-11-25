#pragma once

#include "AtomicKernel.hpp"

namespace BufferTests {
inline ShaderRecompiler::IrProgram MakeHalfExtremaKernel(bool maximum) {
    using namespace ShaderRecompiler;
    AtomicKernel fixture;
    auto& context = fixture.source.Context();
    auto& builder = fixture.builder;
    auto* half = LLVMBC::Type::getHalfTy(context);
    auto* word = LLVMBC::Type::getInt32Ty(context);
    auto* shortType = LLVMBC::Type::getInt16Ty(context);
    auto* pair = LLVMBC::StructType::get(context, {half, half}, "halves");
    auto& index = fixture.GlobalIndex();
    auto& inputOffset = builder.ShiftLeftLogical(index, builder.Constant(3));
    std::array<LLVMBC::Value*, 4> values{};
    for (unsigned operand = 0; operand < 2; ++operand) {
        auto& loaded = fixture.Memory(IrOpcode::LoadAddressU32, builder.IAdd(inputOffset, builder.Constant(operand * 4)), 1);
        for (unsigned component = 0; component < 2; ++component) {
            auto id = operand * 2 + component;
            auto* value = context.construct<LLVMBC::Argument>(half, id, false);
            fixture.scalar.Bind(*value, builder.BitCastF16(builder.ShiftRightLogical(loaded, builder.Constant(component * 16))));
            values[id] = value;
        }
    }
    auto call = [&](std::string_view name, unsigned opcode, LLVMBC::Type* result, LLVMBC::Value* left, LLVMBC::Value* right) {
        auto id = fixture.nextId++;
        fixture.source.Module().add_value_name(id, LLVMBC::String(name.begin(), name.end()));
        auto* type = context.construct<LLVMBC::FunctionType>(context, result, LLVMBC::Vector<LLVMBC::Type*>{word, left->getType(), right->getType()});
        auto* function = context.construct<LLVMBC::Function>(type, id, fixture.source.Module());
        return context.construct<LLVMBC::CallInst>(type, function, LLVMBC::Vector<LLVMBC::Value*>{LLVMBC::ConstantInt::get(word, opcode), left, right});
    };
    auto* left = context.construct<LLVMBC::CompositeConstructInst>(pair, LLVMBC::Vector<LLVMBC::Value*>{values[0], values[1]});
    auto* right = context.construct<LLVMBC::CompositeConstructInst>(pair, LLVMBC::Vector<LLVMBC::Value*>{values[2], values[3]});
    auto* scalar = call("dx.op.binary.f16", maximum ? 35 : 36, half, values[0], values[2]);
    auto* packed = call(maximum ? "xbox.V_PK_MAX_F16" : "xbox.V_PK_MIN_F16", maximum ? 224 : 223, pair, left, right);
    auto* chain = call(maximum ? "xbox.V_PK_MIN_F16" : "xbox.V_PK_MAX_F16", maximum ? 223 : 224, pair, packed, left);
    std::array<LLVMBC::Value*, 5> output{scalar};
    for (unsigned component = 0; component < 2; ++component) {
        output[1 + component] = context.construct<LLVMBC::ExtractValueInst>(half, packed, LLVMBC::Vector<unsigned>{component});
        output[3 + component] = context.construct<LLVMBC::ExtractValueInst>(half, chain, LLVMBC::Vector<unsigned>{component});
    }
    for (unsigned component = 0; component < output.size(); ++component) {
        auto* integer = context.construct<LLVMBC::CastInst>(shortType, output[component], LLVMBC::Instruction::BitCast);
        auto& value = fixture.scalar.Get(*integer);
        auto& offset = builder.IAdd(builder.IMul(index, builder.Constant(20)), builder.Constant(component * 4));
        fixture.Memory(IrOpcode::StoreAddressU32, offset, 2, &value);
    }
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    fixture.program.Metadata().blockInfo[0].terminator.kind = TerminatorKind::Return;
    return std::move(fixture.program);
}
}
