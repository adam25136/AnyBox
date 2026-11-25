#include "IntegerWaveKernel.hpp"
#include "Fixtures.hpp"
#include "../../src/dxil/flow/ActiveLane.hpp"

void CheckActiveLaneProof() {
    using namespace ShaderRecompiler;
    using AnyBox::Shader::Dxil::Flow::ReadsActiveLane;
    for (bool prefix : {false, true}) {
        for (unsigned lane = 0; lane < 32; ++lane) {
            for (unsigned mutation = 0; mutation < 8; ++mutation) {
                auto program = BufferTests::MakeIntegerWaveKernel(32, prefix, 0xA55A8420, 1, true);
                IrBuilder builder(program);
                auto* block = program.BlockOrder()[1];
                std::vector<IrValue*> reads;
                for (auto* value : block->Instructions()) if (value->Opcode() == IrOpcode::ReadLane) reads.push_back(value);
                Fixtures::Require(reads.size() == 32);
                auto* read = reads[lane];
                Fixtures::Require(ReadsActiveLane(*read, 32));
                Fixtures::Require(!ReadsActiveLane(*read, 64));
                auto* select = read->Argument(1);
                auto* present = select->Argument(0);
                auto* bit = present->Argument(0);
                auto* first = select->Argument(2);
                auto* low = bit->Argument(0);
                auto* ballot = low->Argument(0);
                if (mutation == 0) select->ReplaceArgument(1, &builder.Constant((lane + 1) % 32));
                if (mutation == 1) present->ReplaceArgument(1, &builder.Constant(1));
                if (mutation == 2) bit->ReplaceArgument(1, &builder.Constant(std::uint32_t{1} << ((lane + 1) % 32)));
                if (mutation == 3) select->ReplaceArgument(2, &builder.Constant(0));
                if (mutation == 4) low->ReplaceArgument(1, &builder.Constant(1));
                if (mutation == 5) ballot->ReplaceArgument(0, &builder.ConstantBool(false));
                if (mutation == 6) {
                    builder.SetInsertionPoint(*block);
                    auto& unrelated = builder.Emit(IrOpcode::Ballot, IrType::Vec4U32, {&builder.ConstantBool(true)});
                    auto& otherLow = builder.CompositeExtract(unrelated, 0);
                    first->ReplaceArgument(0, &otherLow);
                }
                if (mutation == 7) {
                    builder.SetInsertionPoint(program.EntryBlock());
                    auto& earlier = builder.Emit(IrOpcode::Ballot, IrType::Vec4U32, {&builder.ConstantBool(true)});
                    low->ReplaceArgument(0, &earlier);
                }
                Fixtures::Require(!ReadsActiveLane(*read, 32));
            }
        }
    }
}
