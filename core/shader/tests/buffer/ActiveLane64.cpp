#include "IntegerWaveKernel.hpp"
#include "Fixtures.hpp"
#include "../../src/dxil/flow/ActiveLane.hpp"

void CheckActiveLane64Proof() {
    using namespace ShaderRecompiler;
    using AnyBox::Shader::Dxil::Flow::ReadsActiveLane;
    for (bool prefix : {false, true}) {
        for (unsigned lane = 0; lane < 64; ++lane) {
            for (unsigned mutation = 0; mutation < 15; ++mutation) {
                auto program = BufferTests::MakeIntegerWaveKernel(64, prefix, UINT32_MAX);
                IrBuilder builder(program);
                auto* block = &program.EntryBlock();
                std::vector<IrValue*> reads;
                for (auto* value : block->Instructions()) if (value->Opcode() == IrOpcode::ReadLane) reads.push_back(value);
                Fixtures::Require(reads.size() == 64);
                auto* read = reads[lane];
                Fixtures::Require(ReadsActiveLane(*read, 64));
                Fixtures::Require(!ReadsActiveLane(*read, 32));
                auto* select = read->Argument(1);
                auto* present = select->Argument(0);
                auto* bit = present->Argument(0);
                auto* first = select->Argument(2);
                auto* emptyLow = first->Argument(0);
                auto* upper = first->Argument(1);
                auto* lowFirst = first->Argument(2);
                auto* highFirst = upper->Argument(0);
                auto* word = bit->Argument(0);
                auto* ballot = word->Argument(0);
                if (mutation == 0) select->ReplaceArgument(1, &builder.Constant((lane + 1) % 64));
                if (mutation == 1) present->ReplaceArgument(1, &builder.Constant(1));
                if (mutation == 2) bit->ReplaceArgument(1, &builder.Constant(std::uint32_t{1} << ((lane + 1) % 32)));
                if (mutation == 3) select->ReplaceArgument(2, &builder.Constant(0));
                if (mutation == 4) word->ReplaceArgument(1, &builder.Constant(1 - lane / 32));
                if (mutation == 5) ballot->ReplaceArgument(0, &builder.ConstantBool(false));
                if (mutation == 6) {
                    builder.SetInsertionPoint(*block);
                    auto& unrelated = builder.Emit(IrOpcode::Ballot, IrType::Vec4U32, {&builder.ConstantBool(true)});
                    highFirst->ReplaceArgument(0, &builder.CompositeExtract(unrelated, 1));
                }
                if (mutation == 7) {
                    builder.SetInsertionPoint(program.CreateBlock());
                    auto& unrelated = builder.Emit(IrOpcode::Ballot, IrType::Vec4U32, {&builder.ConstantBool(true)});
                    word->ReplaceArgument(0, &unrelated);
                }
                if (mutation == 8) emptyLow->ReplaceArgument(1, &builder.Constant(1));
                if (mutation == 9) upper->ReplaceArgument(1, &builder.Constant(31));
                if (mutation == 10) lowFirst->ReplaceArgument(0, highFirst->Argument(0));
                if (mutation == 11) highFirst->ReplaceArgument(0, lowFirst->Argument(0));
                if (mutation == 12) {
                    first->ReplaceArgument(1, lowFirst);
                    first->ReplaceArgument(2, upper);
                }
                if (mutation == 13) emptyLow->ReplaceArgument(0, highFirst->Argument(0));
                if (mutation == 14) {
                    block->RemoveInstruction(read);
                    block->InsertInstructionBefore(ballot, read);
                }
                Fixtures::Require(!ReadsActiveLane(*read, 64));
            }
        }
    }
}
