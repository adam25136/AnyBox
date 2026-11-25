#include "AtomicKernel.hpp"
#include "HalfPackReference.hpp"
#include "ComputeCompiler.hpp"
#include "Fixtures.hpp"

void CheckHalfPackInputs(AnyBox::Vulkan::Context& context);

namespace {
ShaderRecompiler::IrProgram Kernel() {
    using namespace ShaderRecompiler;
    BufferTests::AtomicKernel fixture;
    auto& builder = fixture.builder;
    auto& offset = builder.ShiftLeftLogical(fixture.GlobalIndex(), builder.Constant(2));
    auto& loaded = fixture.Memory(IrOpcode::LoadAddressU32, offset, 1);
    auto& context = fixture.source.Context();
    auto* word = LLVMBC::Type::getInt32Ty(context);
    auto* floating = LLVMBC::Type::getFloatTy(context);
    auto* argument = context.construct<LLVMBC::Argument>(floating, 0, false);
    fixture.scalar.Bind(*argument, builder.BitCastF32(loaded));
    fixture.source.Module().add_value_name(130, "dx.op.legacyF32ToF16");
    auto* type = context.construct<LLVMBC::FunctionType>(context, word, LLVMBC::Vector<LLVMBC::Type*>{word, floating});
    auto* function = context.construct<LLVMBC::Function>(type, 130, fixture.source.Module());
    auto* call = context.construct<LLVMBC::CallInst>(type, function, LLVMBC::Vector<LLVMBC::Value*>{LLVMBC::ConstantInt::get(word, 130), argument});
    auto& result = fixture.scalar.Get(*call);
    fixture.Memory(IrOpcode::StoreAddressU32, offset, 2, &result);
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    fixture.program.Metadata().blockInfo[0].terminator.kind = TerminatorKind::Return;
    return std::move(fixture.program);
}
}

void CheckHalfPackGpu(AnyBox::Vulkan::Context& context) {
    CheckHalfPackInputs(context);
    std::vector<std::uint32_t> input;
    for (unsigned half = 0; half < 65536; ++half) {
        auto bits = BufferTests::WidenHalfReference(half);
        input.push_back(bits);
        input.push_back(bits + 1);
        input.push_back(bits - 1);
    }
    for (unsigned exponent = 0; exponent < 256; ++exponent) {
        for (auto mantissa : {0u, 1u, 4095u, 4096u, 4097u, 8191u, 8192u, 8193u, 0x3FFFFFu, 0x400000u, 0x7FDFFFu, 0x7FE000u, 0x7FEFFFu, 0x7FF000u, 0x7FFFFEu, 0x7FFFFFu}) {
            for (auto sign : {0u, 0x80000000u}) input.push_back(sign | (exponent << 23) | mantissa);
        }
    }
    std::uint32_t random = 0x12345678;
    for (unsigned index = 0; index < 65536; ++index) {
        random ^= random << 13;
        random ^= random >> 17;
        random ^= random << 5;
        input.push_back(random);
    }
    std::vector<std::uint32_t> expected;
    for (auto bits : input) expected.push_back(BufferTests::HalfPackReference(bits));
    Fixtures::Require(input.size() % 64 == 0);
    auto program = Kernel();
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = 64;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = info.hostSubgroupSize = 32;
    auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128});
    for (std::size_t offset = 5; offset < shader.code.size(); offset += shader.code[offset] >> 16) {
        auto opcode = shader.code[offset] & 65535;
        if (opcode == spv::OpTypeFloat || opcode == spv::OpTypeInt) Fixtures::Require(shader.code[offset + 2] != 16);
    }
    BufferTests::RunGpu(context, shader, expected, {static_cast<std::uint32_t>(input.size() / 64), 1, 1}, std::as_bytes(std::span(input)));
}
