#include "AtomicKernel.hpp"
#include "Fixtures.hpp"
#include <bit>
#include <cmath>

namespace {
ShaderRecompiler::IrProgram MakeKernel() {
    using namespace ShaderRecompiler;
    BufferTests::AtomicKernel fixture;
    auto& context = fixture.source.Context();
    auto& builder = fixture.builder;
    auto& index = fixture.GlobalIndex();
    auto* floating = LLVMBC::Type::getFloatTy(context);
    std::array<LLVMBC::Value*, 2> operands{};
    for (unsigned side = 0; side < 2; ++side) {
        auto& source = side == 0 ? builder.ShiftRightLogical(index, builder.Constant(4)) : index;
        auto& selected = builder.BitwiseAnd(source, builder.Constant(15));
        auto& offset = builder.ShiftLeftLogical(selected, builder.Constant(2));
        auto& value = builder.BitCastF32(fixture.Memory(IrOpcode::LoadAddressU32, offset, 1));
        auto* argument = context.construct<LLVMBC::Argument>(floating, side, false);
        fixture.scalar.Bind(*argument, value);
        operands[side] = argument;
    }
    auto* division = context.construct<LLVMBC::BinaryOperator>(operands[0], operands[1], LLVMBC::Instruction::FDiv);
    auto& value = fixture.scalar.Get(*division);
    auto& nan = builder.Emit(IrOpcode::FPIsNan32, IrType::Bool, {&value});
    auto& bits = builder.Select(nan, builder.Constant(0x7FC00000), builder.BitCastU32(value));
    fixture.Memory(IrOpcode::StoreAddressU32, builder.ShiftLeftLogical(index, builder.Constant(2)), 2, &bits);
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    return std::move(fixture.program);
}
}

void BufferTests::CheckFloatDivision(AnyBox::Vulkan::Context& context) {
    auto program = MakeKernel();
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = 64;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = info.hostSubgroupSize = 32;
    auto shader = AnyBox::Shader::CompileCompute(program, info, Target(), {0, 0, 0, 128});
    unsigned divisions = 0;
    for (std::size_t offset = 5; offset < shader.code.size();) {
        auto size = shader.code[offset] >> 16;
        Fixtures::Require(size != 0 && size <= shader.code.size() - offset);
        if ((shader.code[offset] & 65535) == spv::OpFDiv) ++divisions;
        offset += size;
    }
    Fixtures::Require(divisions == 1);

    std::array<std::uint32_t, 16> input{0, 0x80000000, 0x3F800000, 0xBF800000, 0x40000000, 0xC0000000, 0x3F000000, 0xBF000000, 0x4F800000, 0xCF800000, 0x2F800000, 0xAF800000, 0x7F800000, 0xFF800000, 0x7FC12345, 0x7F812345};
    std::vector<std::uint32_t> expected;
    for (auto numerator : input) {
        for (auto denominator : input) {
            auto result = std::bit_cast<float>(numerator) / std::bit_cast<float>(denominator);
            expected.push_back(std::isnan(result) ? 0x7FC00000U : std::bit_cast<std::uint32_t>(result));
        }
    }
    RunGpu(context, shader, expected, {4, 1, 1}, std::as_bytes(std::span(input)));
}
