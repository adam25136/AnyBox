#include "AtomicKernel.hpp"
#include "ComputeCompiler.hpp"
#include "Fixtures.hpp"
#include "../../src/dxil/lowering/LaneReadMode.hpp"

namespace {
ShaderRecompiler::IrProgram Kernel(bool earlyReturn) {
    using namespace ShaderRecompiler;
    BufferTests::AtomicKernel fixture;
    auto& program = fixture.program;
    auto& builder = fixture.builder;
    auto& entry = program.EntryBlock();
    auto& active = program.CreateBlock();
    auto& inactive = program.CreateBlock();
    auto& merge = program.CreateBlock();
    program.BlockOrder() = {&entry, &active, &inactive, &merge};
    auto& metadata = program.Metadata().blockInfo;
    metadata.resize(4);
    for (unsigned block = 0; block < 4; ++block) metadata[block].id = block;
    auto& global = fixture.GlobalIndex();
    auto& payload = builder.IAdd(builder.IMul(global, builder.Constant(17)), builder.Constant(5));
    auto& condition = builder.INotEqual(builder.BitwiseAnd(global, builder.Constant(1)), builder.Constant(0));
    builder.BranchConditional(condition, active, inactive);
    metadata[0].condition = &condition;
    metadata[0].terminator.kind = TerminatorKind::ConditionalBranch;
    metadata[0].terminator.condition = BranchCondition::ScalarInstruction;
    metadata[0].terminator.trueBlock = active.Id();
    metadata[0].terminator.falseBlock = inactive.Id();
    for (auto* block : {&active, &inactive}) {
        builder.SetInsertionPoint(*block);
        fixture.Memory(IrOpcode::StoreAddressU32, builder.IMul(global, builder.Constant(8)), 2, &builder.Constant(block == &active ? 111 : 222));
        if (earlyReturn && block == &active) {
            (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
            metadata[block->Id()].terminator.kind = TerminatorKind::Return;
        } else {
            builder.Branch(merge);
            metadata[block->Id()].terminator.kind = TerminatorKind::Branch;
            metadata[block->Id()].terminator.trueBlock = merge.Id();
        }
    }
    builder.SetInsertionPoint(merge);
    auto& value = builder.Emit(IrOpcode::ReadLane, IrType::U32, {&payload, &builder.Constant(1)});
    fixture.Memory(IrOpcode::StoreAddressU32, builder.IAdd(builder.IMul(global, builder.Constant(8)), builder.Constant(4)), 2, &value);
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    metadata[merge.Id()].terminator.kind = TerminatorKind::Return;
    ValidateProgram(program, true);
    return std::move(program);
}
}

void CheckReadLaneReconvergence(AnyBox::Vulkan::Context& context) {
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = 128;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = info.hostSubgroupSize = 32;
    {
        auto program = Kernel(false);
        auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128});
        std::vector<std::uint32_t> expected;
        for (unsigned thread = 0; thread < 256; ++thread) {
            expected.push_back(thread & 1 ? 111 : 222);
            expected.push_back((thread / 32 * 32 + 1) * 17 + 5);
        }
        BufferTests::RunGpu(context, shader, expected, {2, 1, 1});
    }
    auto program = Kernel(true);
    bool rejected = false;
    try { (void)AnyBox::Shader::Dxil::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128}); }
    catch (const std::runtime_error& error) { rejected = std::string_view(error.what()) == "Xbox read lane from divergent control flow is not implemented"; }
    Fixtures::Require(rejected);
    auto activeOnly = Kernel(true);
    ShaderRecompiler::IrBuilder builder(activeOnly);
    for (auto* block : activeOnly.BlockOrder()) {
        for (auto* value : block->Instructions()) {
            if (value->Opcode() != ShaderRecompiler::IrOpcode::ReadLane) continue;
            value->SetFlags(AnyBox::Shader::Dxil::LaneReadMode::Active);
            value->ReplaceArgument(1, &builder.Constant(0));
        }
    }
    auto shader = AnyBox::Shader::Dxil::CompileCompute(activeOnly, info, BufferTests::Target(), {0, 0, 0, 128});
    std::vector<std::uint32_t> expected;
    for (unsigned thread = 0; thread < 256; ++thread) {
        expected.push_back(thread & 1 ? 111 : 222);
        expected.push_back(thread & 1 ? UINT32_MAX : (thread / 32 * 32) * 17 + 5);
    }
    BufferTests::RunGpu(context, shader, expected, {2, 1, 1});
    auto invalid = Kernel(false);
    for (auto* block : invalid.BlockOrder()) {
        for (auto* value : block->Instructions()) if (value->Opcode() == ShaderRecompiler::IrOpcode::ReadLane) value->SetFlags(std::uint64_t{2});
    }
    Fixtures::Reject<std::runtime_error>([&] { (void)AnyBox::Shader::Dxil::CompileCompute(invalid, info, BufferTests::Target(), {0, 0, 0, 128}); });
}
