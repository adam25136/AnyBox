#include "AtomicKernel.hpp"
#include "Fixtures.hpp"

namespace {
ShaderRecompiler::IrProgram MakeKernel(unsigned words, bool scalar, unsigned displacement, bool varying = false, unsigned baseOffset = 0) {
    using namespace ShaderRecompiler;
    BufferTests::AtomicKernel fixture;
    auto& context = fixture.source.Context();
    auto& builder = fixture.builder;
    auto& index = fixture.GlobalIndex();
    auto& group = builder.Emit(IrOpcode::GetBuiltin, IrType::U32, {&builder.Constant(static_cast<std::uint32_t>(StageInputKind::WorkgroupId)), &builder.Constant(0)});
    auto& selected = scalar && !varying ? group : builder.BitwiseAnd(index, builder.Constant(3));
    auto& offset = builder.IAdd(builder.ShiftLeftLogical(selected, builder.Constant(2)), builder.Constant(baseOffset + displacement));
    auto* word = LLVMBC::Type::getInt32Ty(context);
    auto* wide = LLVMBC::Type::getInt64Ty(context);
    auto* address = context.construct<LLVMBC::Argument>(wide, 0, false);
    fixture.scalar.Bind(*address, builder.ConstructU64(offset, builder.Constant(1)));
    LLVMBC::Type* result = words == 1 ? word : LLVMBC::StructType::get(context, LLVMBC::Vector<LLVMBC::Type*>(words, word), "");
    constexpr std::array names{"xbox.global_load_dword_with_cache", "xbox.global_load_dwordx2_with_cache", "xbox.global_load_dwordx3_with_cache", "xbox.global_load_dwordx4_with_cache"};
    fixture.source.Module().add_value_name(356, scalar ? "xbox.s_load_dword_with_cache" : names.at(words - 1));
    auto* signature = context.construct<LLVMBC::FunctionType>(context, result, LLVMBC::Vector<LLVMBC::Type*>{word, wide, word, word});
    auto* function = context.construct<LLVMBC::Function>(signature, 356, fixture.source.Module());
    auto* call = context.construct<LLVMBC::CallInst>(signature, function, LLVMBC::Vector<LLVMBC::Value*>{LLVMBC::ConstantInt::get(word, scalar ? 364 : 355 + words), address, LLVMBC::UndefValue::get(word), LLVMBC::UndefValue::get(word)});
    auto& loaded = fixture.scalar.Get(*call);
    auto& base = builder.IMul(index, builder.Constant(words * 4));
    for (unsigned part = 0; part < words; ++part) {
        auto& value = words == 1 ? loaded : builder.CompositeExtract(loaded, part);
        fixture.Memory(IrOpcode::StoreAddressU32, builder.IAdd(base, builder.Constant(part * 4)), 2, &value);
    }
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    return std::move(fixture.program);
}

void CheckGpu(AnyBox::Vulkan::Context& context, unsigned words, bool scalar, unsigned displacement, unsigned baseOffset = 0) {
    auto program = MakeKernel(words, scalar, displacement, false, baseOffset);
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = 64;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = info.hostSubgroupSize = 32;
    auto shader = AnyBox::Shader::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128});
    std::array<std::uint32_t, 16> input{};
    for (unsigned index = 0; index < input.size(); ++index) input[index] = 0xA1234500U + index;
    std::vector<std::uint32_t> expected;
    for (unsigned thread = 0; thread < 128; ++thread) {
        auto selected = baseOffset / 4 + (scalar ? thread / 64 : thread & 3);
        for (unsigned part = 0; part < words; ++part) expected.push_back(input[selected + part]);
    }
    BufferTests::RunGpu(context, shader, expected, {2, 1, 1}, std::as_bytes(std::span(input)));
}
}

namespace BufferTests {
void CheckAddressLoads(AnyBox::Vulkan::Context& context) {
    for (unsigned words = 1; words <= 4; ++words) CheckGpu(context, words, false, 0);
    for (unsigned displacement = 0; displacement < 4; ++displacement) {
        CheckGpu(context, 1, true, displacement);
        CheckGpu(context, 1, true, displacement, 56);
    }
    Fixtures::Reject<std::runtime_error>([&] { (void)MakeKernel(1, true, 0, true); });
}
}
