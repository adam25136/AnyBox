#include "Fixture.hpp"
#include "Fixtures.hpp"
#include "../buffer/Target.hpp"

namespace {
void CheckBarrier(std::uint32_t opcode, std::uint32_t mode) {
    using namespace ShaderRecompiler;
    FlowTests::Fixture fixture;
    auto* block = fixture.Block();
    auto& context = fixture.Context();
    LLVMBC::Module symbols(context);
    symbols.add_value_name(0, "dx.op.barrier");
    auto* word = LLVMBC::Type::getInt32Ty(context);
    auto* signature = context.construct<LLVMBC::FunctionType>(context, LLVMBC::Type::getVoidTy(context), LLVMBC::Vector<LLVMBC::Type*>{word, word});
    auto* declaration = context.construct<LLVMBC::Function>(signature, 0, symbols);
    fixture.Emit<LLVMBC::CallInst>(block, signature, declaration, LLVMBC::Vector<LLVMBC::Value*>{LLVMBC::ConstantInt::get(word, opcode), LLVMBC::ConstantInt::get(word, mode)});
    fixture.Emit<LLVMBC::ReturnInst>(block, nullptr);
    if (opcode != 80 || (mode != 1 && mode != 9)) {
        Fixtures::Reject<std::runtime_error>([&] { (void)fixture.Lower(); });
        return;
    }
    auto program = fixture.Lower();
    ShaderComputeInputInfo input;
    input.threadsNum[0] = 64;
    input.threadsNum[1] = input.threadsNum[2] = 1;
    input.waveSize = input.hostSubgroupSize = 32;
    auto shader = AnyBox::Shader::CompileCompute(program, input, BufferTests::Target(), {0, 0, 0, 128});
    std::unordered_map<std::uint32_t, std::uint32_t> constants;
    unsigned barriers = 0;
    for (std::size_t offset = 5; offset < shader.code.size();) {
        auto words = shader.code[offset] >> 16;
        auto operation = shader.code[offset] & 65535;
        Fixtures::Require(words != 0 && words <= shader.code.size() - offset);
        if (operation == spv::OpConstant && words == 4) constants.emplace(shader.code[offset + 2], shader.code[offset + 3]);
        if (operation == spv::OpControlBarrier) {
            Fixtures::Require(words == 4);
            Fixtures::Require(constants.at(shader.code[offset + 1]) == spv::ScopeWorkgroup);
            Fixtures::Require(constants.at(shader.code[offset + 2]) == spv::ScopeWorkgroup);
            auto semantics = constants.at(shader.code[offset + 3]);
            Fixtures::Require((semantics & spv::MemorySemanticsAcquireReleaseMask) != 0);
            ++barriers;
        }
        offset += words;
    }
    Fixtures::Require(barriers == 1);
}
}

void FlowTests::CheckBarriers() {
    for (unsigned mode : {0u, 1u, 2u, 4u, 8u, 9u, 15u, UINT32_MAX}) CheckBarrier(80, mode);
    CheckBarrier(79, 9);
}
