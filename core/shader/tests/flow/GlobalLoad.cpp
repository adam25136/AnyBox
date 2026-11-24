#include "Fixture.hpp"
#include "Fixtures.hpp"
#include <Optimization/ConstantFolder.hpp>

namespace {
void CheckGlobalLoad(unsigned words, bool explicitCache, bool scalar = false) {
    using namespace ShaderRecompiler;
    FlowTests::Fixture fixture;
    auto* entry = fixture.Block();
    auto& context = fixture.Context();
    LLVMBC::Module symbols(context);
    std::array<const char*, 4> names{"xbox.global_load_dword_with_cache", "xbox.global_load_dwordx2_with_cache", "xbox.global_load_dwordx3_with_cache", "xbox.global_load_dwordx4_with_cache"};
    symbols.add_value_name(0, scalar ? "xbox.s_load_dword_with_cache" : names.at(words - 1));
    auto* word = LLVMBC::Type::getInt32Ty(context);
    auto* address = LLVMBC::Type::getInt64Ty(context);
    LLVMBC::Type* result = word;
    if (words != 1) result = LLVMBC::StructType::get(context, LLVMBC::Vector<LLVMBC::Type*>(words, word), "");
    auto* signature = context.construct<LLVMBC::FunctionType>(context, result, LLVMBC::Vector<LLVMBC::Type*>{word, address, word, word});
    auto* declaration = context.construct<LLVMBC::Function>(signature, 0, symbols);
    const std::uint64_t guestAddress = scalar ? 0x7FFF1234567B : 0x7FFF12345678;
    auto* cache = explicitCache ? static_cast<LLVMBC::Value*>(LLVMBC::ConstantInt::get(word, 1)) : LLVMBC::UndefValue::get(word);
    auto* loaded = fixture.Emit<LLVMBC::CallInst>(entry, signature, declaration, LLVMBC::Vector<LLVMBC::Value*>{LLVMBC::ConstantInt::get(word, scalar ? 364 : 355 + words), LLVMBC::ConstantInt::get(address, guestAddress), cache, LLVMBC::UndefValue::get(word)});
    if (words != 1) {
        for (unsigned index = 0; index < words; ++index) fixture.Emit<LLVMBC::ExtractValueInst>(entry, word, loaded, LLVMBC::Vector<unsigned>{index});
    }
    fixture.Emit<LLVMBC::ReturnInst>(entry, nullptr);
    if (explicitCache) {
        Fixtures::Reject<std::runtime_error>([&] { (void)fixture.Lower(); });
        return;
    }
    auto output = fixture.Lower();
    ConstantFolder{}.Fold(output);
    ResolveControlFlowIdentities(output);
    ValidateProgram(output, true);
    Fixtures::Require(output.Resources().memoryInfo.size() == 1);
    const auto& memory = output.Resources().memoryInfo.front();
    Fixtures::Require(memory.kind == ResourceKind::Global && memory.addressIsFull && memory.dataDwords == words && memory.componentCount == words && memory.dataBits == 32);
    unsigned operations = 0;
    for (auto* instruction : output.EntryBlock().Instructions()) {
        if (AddressOpcodeInfoOf(instruction->Opcode()).access != AddressAccess::Read) continue;
        ++operations;
        Fixtures::Require(AddressOpcodeInfoOf(instruction->Opcode()).components == words);
        Fixtures::Require(instruction->Argument(0)->Opcode() == IrOpcode::GetAddressResource);
        Fixtures::Require(instruction->Argument(1)->Resolve()->ImmediateU32() == static_cast<std::uint32_t>(scalar ? guestAddress & ~std::uint64_t{3} : guestAddress));
        Fixtures::Require(instruction->Argument(2)->Resolve()->ImmediateU32() == guestAddress >> 32);
        Fixtures::Require(instruction->Argument(3)->Resolve()->ImmediateBool());
    }
    Fixtures::Require(operations == 1);
}
}

void FlowTests::CheckGlobalLoads() {
    CheckGlobalLoad(1, false, true);
    CheckGlobalLoad(1, true, true);
    for (unsigned words = 1; words <= 4; ++words) {
        CheckGlobalLoad(words, false);
        CheckGlobalLoad(words, true);
    }
}
