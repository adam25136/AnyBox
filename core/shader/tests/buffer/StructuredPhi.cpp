#include "AtomicKernel.hpp"
#include "ComputeCompiler.hpp"
#include "StructuredPhiTypes.hpp"
#include "Fixtures.hpp"
#include "flow/Builder.hpp"

namespace {
constexpr std::array<std::array<unsigned, 2>, 18> Edges{{{2, 1}, {2, 17}, {3, 12}, {4, 5}, {17, 17}, {6, 10}, {7, 7}, {9, 8}, {11, 11}, {7, 7}, {11, 11}, {17, 17}, {13, 14}, {17, 17}, {15, 16}, {17, 17}, {17, 17}, {17, 17}}};

ShaderRecompiler::IrProgram MakeKernel(ShaderRecompiler::IrType type) {
    using namespace ShaderRecompiler;
    BufferTests::AtomicKernel fixture;
    auto& program = fixture.program;
    auto& builder = fixture.builder;
    auto& index = fixture.GlobalIndex();
    std::vector<IrBlock*> blocks{&program.EntryBlock()};
    while (blocks.size() < Edges.size()) blocks.push_back(&program.CreateBlock());
    program.BlockOrder() = blocks;
    auto& metadata = program.Metadata().blockInfo;
    metadata.resize(blocks.size());
    builder.SetInsertionPoint(*blocks[7]);
    auto& counter = builder.Emit(IrOpcode::Phi, IrType::U32, {});
    builder.SetInsertionPoint(*blocks[9]);
    auto& next = builder.IAdd(counter, builder.Constant(1));
    counter.AddPhiOperand(blocks[6], &builder.Constant(0));
    counter.AddPhiOperand(blocks[9], &next);
    builder.SetInsertionPoint(*blocks[17]);
    auto& result = builder.Emit(IrOpcode::Phi, type, {});

    for (unsigned block = 0; block < blocks.size(); ++block) {
        metadata[block].id = block;
        builder.SetInsertionPoint(*blocks[block]);
        if (block == 17) {
            auto& offset = builder.ShiftLeftLogical(index, builder.Constant(3));
            auto& low = BufferTests::PhiWord(builder, result);
            auto& high = type == IrType::U64 ? builder.CompositeExtract(result, 1) : builder.Constant(0);
            fixture.Memory(IrOpcode::StoreAddressU32, offset, 2, &low);
            fixture.Memory(IrOpcode::StoreAddressU32, builder.IAdd(offset, builder.Constant(4)), 2, &high);
            (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
            metadata[block].terminator.kind = TerminatorKind::Return;
            continue;
        }
        auto& payload = builder.IAdd(index, builder.Constant(block * 5));
        auto [first, second] = Edges[block];
        if (first == 17 || second == 17) result.AddPhiOperand(blocks[block], &BufferTests::PhiPayload(builder, payload, type));
        auto& term = metadata[block].terminator;
        term.trueBlock = first;
        if (first == second) {
            builder.Branch(*blocks[first]);
            term.kind = TerminatorKind::Branch;
        } else {
            auto& condition = block == 7 ? builder.ULessThan(counter, builder.Constant(3)) : builder.INotEqual(builder.BitwiseAnd(index, builder.Constant(1u << (block % 8))), builder.Constant(0));
            builder.BranchConditional(condition, *blocks[first], *blocks[second]);
            metadata[block].condition = &condition;
            term.kind = TerminatorKind::ConditionalBranch;
            term.condition = BranchCondition::ScalarInstruction;
            term.falseBlock = second;
        }
    }
    ValidateProgram(program, true);
    return std::move(program);
}

std::uint32_t Expected(unsigned index) {
    unsigned block = 0;
    unsigned counter = 0;
    for (unsigned step = 0; step < 64; ++step) {
        if (block == 9) ++counter;
        bool condition = block == 7 ? counter < 3 : (index & (1u << (block % 8))) != 0;
        auto next = Edges[block][condition ? 0 : 1];
        if (next == 17) return index + block * 5;
        block = next;
    }
    throw std::logic_error("Structured PHI reference did not terminate");
}
}

namespace BufferTests {
void CheckStructuredPhi(AnyBox::Vulkan::Context& context) {
    using ShaderRecompiler::IrType;
    auto invalid = MakeKernel(IrType::U32);
    invalid.Blocks()[17]->Instructions().front()->ReplaceArgument(0, invalid.Blocks()[14]->Instructions().front());
    Fixtures::Reject<std::runtime_error>([&] { (void)AnyBox::Shader::Dxil::Flow::Builder(invalid).Run(); });
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = 64;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = info.hostSubgroupSize = 32;
    for (auto type : {IrType::Bool, IrType::U8, IrType::U16, IrType::U32, IrType::U64, IrType::F32}) {
        auto program = MakeKernel(type);
        auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, Target(), {0, 0, 0, 128});
        std::vector<std::uint32_t> expected;
        for (unsigned index = 0; index < 256; ++index) {
            auto value = Expected(index);
            expected.push_back(type == IrType::Bool ? value & 1 : type == IrType::U8 ? value & 255 : value);
            expected.push_back(type == IrType::U64 ? value * 17 : 0);
        }
        RunGpu(context, shader, expected, {4, 1, 1});
    }
}
}
