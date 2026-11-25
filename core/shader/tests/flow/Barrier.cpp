#include "Fixture.hpp"
#include "Fixtures.hpp"
#include "../buffer/Target.hpp"

namespace {
void CheckBarrier(std::uint32_t opcode, std::uint32_t mode, unsigned wave = 32) {
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
    if (opcode != 80 || mode == 0 || mode > 15) {
        Fixtures::Reject<std::runtime_error>([&] { (void)fixture.Lower(); });
        return;
    }
    auto program = fixture.Lower();
    ShaderComputeInputInfo input;
    input.threadsNum[0] = 64;
    input.threadsNum[1] = input.threadsNum[2] = 1;
    input.waveSize = wave;
    input.hostSubgroupSize = 32;
    auto shader = AnyBox::Shader::CompileCompute(program, input, BufferTests::Target(), {0, 0, 0, 128});
    std::unordered_map<std::uint32_t, std::uint32_t> constants;
    unsigned barriers = 0;
    for (std::size_t offset = 5; offset < shader.code.size();) {
        auto words = shader.code[offset] >> 16;
        auto operation = shader.code[offset] & 65535;
        Fixtures::Require(words != 0 && words <= shader.code.size() - offset);
        if (operation == spv::OpConstant && words == 4) constants.emplace(shader.code[offset + 2], shader.code[offset + 3]);
        if (operation == spv::OpControlBarrier || operation == spv::OpMemoryBarrier) {
            bool sync = (mode & 1) != 0;
            Fixtures::Require(operation == static_cast<std::uint32_t>(sync ? spv::OpControlBarrier : spv::OpMemoryBarrier));
            Fixtures::Require(words == (sync ? 4u : 3u));
            if (sync) Fixtures::Require(constants.at(shader.code[offset + 1]) == spv::ScopeWorkgroup);
            auto scope = constants.at(shader.code[offset + (sync ? 2 : 1)]);
            Fixtures::Require(scope == static_cast<std::uint32_t>(mode & 2 ? spv::ScopeDevice : spv::ScopeWorkgroup));
            std::uint32_t semantics = 0;
            if (mode & 6) semantics |= spv::MemorySemanticsUniformMemoryMask | spv::MemorySemanticsImageMemoryMask;
            if (mode & 8) semantics |= spv::MemorySemanticsWorkgroupMemoryMask;
            if (semantics) semantics |= spv::MemorySemanticsAcquireReleaseMask;
            Fixtures::Require(constants.at(shader.code[offset + (sync ? 3 : 2)]) == semantics);
            ++barriers;
        }
        offset += words;
    }
    Fixtures::Require(barriers == 1);
}
}

void FlowTests::CheckBarriers() {
    for (unsigned mode = 0; mode <= 16; ++mode) CheckBarrier(80, mode);
    for (unsigned mode = 1; mode <= 15; ++mode) CheckBarrier(80, mode, 64);
    CheckBarrier(80, UINT32_MAX);
    CheckBarrier(79, 9);
}
