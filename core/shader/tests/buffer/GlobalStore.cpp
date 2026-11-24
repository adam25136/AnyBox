#include "AtomicKernel.hpp"

namespace BufferTests { void CheckGlobalStoreInvalid(); }

namespace {
void CheckGpu(AnyBox::Vulkan::Context& context, unsigned words, unsigned mode) {
    using namespace ShaderRecompiler;
    BufferTests::AtomicKernel fixture;
    auto& llvmContext = fixture.source.Context();
    auto& symbols = fixture.source.Module();
    auto& builder = fixture.builder;
    auto* word = LLVMBC::Type::getInt32Ty(llvmContext);
    auto* wide = LLVMBC::Type::getInt64Ty(llvmContext);
    auto* empty = LLVMBC::Type::getVoidTy(llvmContext);
    auto* aggregate = words == 1 ? word : LLVMBC::StructType::get(llvmContext, LLVMBC::Vector<LLVMBC::Type*>(words, word), "");
    auto& index = fixture.GlobalIndex();
    auto* dynamic = llvmContext.construct<LLVMBC::Argument>(word, 0, false);
    fixture.scalar.Bind(*dynamic, index);
    LLVMBC::Vector<LLVMBC::Value*> values;
    for (unsigned part = 0; part < words; ++part) values.push_back(mode == 0 && part == 0 ? static_cast<LLVMBC::Value*>(dynamic) : LLVMBC::ConstantInt::get(word, 100 + part));
    LLVMBC::Value* data = words == 1 ? values.front() : mode == 1 ? static_cast<LLVMBC::Value*>(llvmContext.construct<LLVMBC::ConstantDataArray>(aggregate, values)) : llvmContext.construct<LLVMBC::CompositeConstructInst>(aggregate, values);
    if (mode == 4 && words != 1) data = llvmContext.construct<LLVMBC::ConstantAggregate>(aggregate, values);
    if (mode == 2) data = words == 1 ? static_cast<LLVMBC::Value*>(LLVMBC::ConstantInt::get(word, 0)) : llvmContext.construct<LLVMBC::ConstantAggregateZero>(aggregate);
    if (mode == 3) data = LLVMBC::UndefValue::get(aggregate);
    auto* address = llvmContext.construct<LLVMBC::Argument>(wide, 1, false);
    fixture.scalar.Bind(*address, builder.ConstructU64(builder.ShiftLeftLogical(index, builder.Constant(4)), builder.Constant(2)));

    constexpr std::array names{"xbox.global_store_dword_with_cache", "xbox.global_store_dwordx2_with_cache", "xbox.global_store_dwordx3_with_cache", "xbox.global_store_dwordx4_with_cache"};
    symbols.add_value_name(360, names.at(words - 1));
    auto* signature = llvmContext.construct<LLVMBC::FunctionType>(llvmContext, empty, LLVMBC::Vector<LLVMBC::Type*>{word, wide, aggregate, word, word});
    auto* function = llvmContext.construct<LLVMBC::Function>(signature, 360, symbols);
    auto* call = llvmContext.construct<LLVMBC::CallInst>(signature, function, LLVMBC::Vector<LLVMBC::Value*>{LLVMBC::ConstantInt::get(word, 359 + words), address, data, LLVMBC::UndefValue::get(word), LLVMBC::UndefValue::get(word)});
    fixture.scalar.Emit(*call);
    auto shader = fixture.Compile();
    std::vector<std::uint32_t> expected(128 * 4, UINT32_MAX);
    for (unsigned thread = 0; thread < 128; ++thread) {
        for (unsigned part = 0; part < words; ++part) expected[thread * 4 + part] = mode == 2 || mode == 3 ? 0 : mode == 0 && part == 0 ? thread : 100 + part;
    }
    BufferTests::RunGpu(context, shader, expected, {2, 1, 1});
}
}

void BufferTests::CheckGlobalStoreGpu(AnyBox::Vulkan::Context& context) {
    CheckGlobalStoreInvalid();
    for (unsigned words = 1; words <= 4; ++words) {
        for (unsigned mode = 0; mode < 5; ++mode) CheckGpu(context, words, mode);
    }
}
