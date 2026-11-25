#include "AtomicKernel.hpp"
#include "FloatFixture.hpp"
#include "ComputeCompiler.hpp"
#include <bit>

void CheckWideUnaryInvalid();

namespace {
std::uint64_t Expected(unsigned opcode, std::uint64_t value) {
    if (opcode == 30) {
        std::uint64_t result = 0;
        for (unsigned bit = 0; bit < 64; ++bit) result |= ((value >> bit) & 1) << (63 - bit);
        return result;
    }
    if (opcode == 31) return static_cast<std::uint32_t>(std::popcount(value));
    if (opcode == 32) return value == 0 ? UINT32_MAX : static_cast<std::uint32_t>(std::countr_zero(value));
    if (opcode == 34 && (value >> 63) != 0) value = ~value;
    return value == 0 ? UINT32_MAX : static_cast<std::uint32_t>(std::countl_zero(value));
}

ShaderRecompiler::IrProgram Kernel() {
    using namespace ShaderRecompiler;
    BufferTests::AtomicKernel fixture;
    auto& builder = fixture.builder;
    auto& index = fixture.GlobalIndex();
    auto& offset = builder.ShiftLeftLogical(index, builder.Constant(3));
    auto& low = fixture.Memory(IrOpcode::LoadAddressU32, offset, 1);
    auto& high = fixture.Memory(IrOpcode::LoadAddressU32, builder.IAdd(offset, builder.Constant(4)), 1);
    auto& context = fixture.source.Context();
    auto* word = LLVMBC::Type::getInt32Ty(context);
    auto* wide = LLVMBC::Type::getInt64Ty(context);
    auto* argument = context.construct<LLVMBC::Argument>(wide, 0, false);
    fixture.scalar.Bind(*argument, builder.ConstructU64(low, high));
    for (unsigned opcode = 30; opcode <= 34; ++opcode) {
        auto* type = context.construct<LLVMBC::FunctionType>(context, opcode == 30 ? wide : word, LLVMBC::Vector<LLVMBC::Type*>{word, wide});
        fixture.source.Module().add_value_name(opcode, opcode == 30 ? "dx.op.unary.i64" : "dx.op.unaryBits.i64");
        auto* function = context.construct<LLVMBC::Function>(type, opcode, fixture.source.Module());
        auto* call = context.construct<LLVMBC::CallInst>(type, function, LLVMBC::Vector<LLVMBC::Value*>{LLVMBC::ConstantInt::get(word, opcode), argument});
        auto& value = fixture.scalar.Get(*call);
        auto& destination = builder.ShiftLeftLogical(builder.IAdd(builder.IMul(index, builder.Constant(6)), builder.Constant(opcode == 30 ? 0 : opcode - 29)), builder.Constant(2));
        auto& lower = opcode == 30 ? builder.CompositeExtract(value, 0) : value;
        fixture.Memory(IrOpcode::StoreAddressU32, destination, 2, &lower);
        if (opcode == 30) fixture.Memory(IrOpcode::StoreAddressU32, builder.IAdd(destination, builder.Constant(4)), 2, &builder.CompositeExtract(value, 1));
    }
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    fixture.program.Metadata().blockInfo[0].terminator.kind = TerminatorKind::Return;
    return std::move(fixture.program);
}

void CheckConstants(AnyBox::Vulkan::Context& context) {
    BufferTests::FloatFixture fixture;
    std::array<std::uint64_t, 8> input{0, UINT64_MAX, 1, 0x8000000000000000, 0x7FFFFFFFFFFFFFFF, 0xFFFFFFFF00000000, 0x80000000, 0x0123456789ABCDEF};
    auto* word = fixture.IntegerType(32);
    auto* wide = fixture.IntegerType(64);
    for (unsigned index = 0; index < input.size(); ++index) {
        auto* low = fixture.Make<LLVMBC::CastInst>(word, fixture.Argument(index * 2), LLVMBC::Instruction::BitCast);
        auto* high = fixture.Make<LLVMBC::CastInst>(word, fixture.Argument(index * 2 + 1), LLVMBC::Instruction::BitCast);
        auto* lower = fixture.Make<LLVMBC::CastInst>(wide, low, LLVMBC::Instruction::ZExt);
        auto* upper = fixture.Make<LLVMBC::CastInst>(wide, high, LLVMBC::Instruction::ZExt);
        auto* shifted = fixture.Make<LLVMBC::BinaryOperator>(upper, fixture.Integer(64, 32), LLVMBC::Instruction::Shl);
        auto* dynamic = fixture.Make<LLVMBC::BinaryOperator>(lower, shifted, LLVMBC::Instruction::Or);
        for (unsigned opcode = 30; opcode <= 34; ++opcode) {
            for (auto* value : {static_cast<LLVMBC::Value*>(dynamic), static_cast<LLVMBC::Value*>(fixture.Integer(64, input[index]))}) {
                auto* call = fixture.Call(opcode == 30 ? "dx.op.unary.i64" : "dx.op.unaryBits.i64", opcode == 30 ? wide : word, {fixture.Integer(32, opcode), value});
                auto expected = Expected(opcode, input[index]);
                if (opcode != 30) fixture.Expect(*call, static_cast<std::uint32_t>(expected));
                else {
                    fixture.Expect(*fixture.Make<LLVMBC::CastInst>(word, call, LLVMBC::Instruction::Trunc), static_cast<std::uint32_t>(expected));
                    auto* upperResult = fixture.Make<LLVMBC::BinaryOperator>(call, fixture.Integer(64, 32), LLVMBC::Instruction::LShr);
                    fixture.Expect(*fixture.Make<LLVMBC::CastInst>(word, upperResult, LLVMBC::Instruction::Trunc), static_cast<std::uint32_t>(expected >> 32));
                }
            }
        }
    }
    std::array<std::uint32_t, 16> words;
    for (unsigned index = 0; index < input.size(); ++index) {
        words[index * 2] = static_cast<std::uint32_t>(input[index]);
        words[index * 2 + 1] = static_cast<std::uint32_t>(input[index] >> 32);
    }
    fixture.Run(context, words);
}
}

void CheckWideUnaryGpu(AnyBox::Vulkan::Context& context) {
    CheckWideUnaryInvalid();
    CheckConstants(context);
    std::vector<std::uint64_t> input{0, UINT64_MAX, 1, 0x8000000000000000, 0x7FFFFFFFFFFFFFFF, 0xFFFFFFFF00000000, 0x80000000, 0x0123456789ABCDEF};
    for (unsigned bit = 0; bit < 64; ++bit) {
        input.push_back(std::uint64_t{1} << bit);
        input.push_back(~(std::uint64_t{1} << bit));
    }
    std::uint64_t random = 0x123456789ABCDEF0;
    while (input.size() < 256) {
        random ^= random << 13;
        random ^= random >> 7;
        random ^= random << 17;
        input.push_back(random);
    }
    std::vector<std::uint32_t> expected;
    for (auto value : input) {
        for (unsigned opcode = 30; opcode <= 34; ++opcode) {
            auto result = Expected(opcode, value);
            expected.push_back(static_cast<std::uint32_t>(result));
            if (opcode == 30) expected.push_back(static_cast<std::uint32_t>(result >> 32));
        }
    }
    auto program = Kernel();
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = 64;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = info.hostSubgroupSize = 32;
    auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128});
    BufferTests::RunGpu(context, shader, expected, {4, 1, 1}, std::as_bytes(std::span(input)));
}
