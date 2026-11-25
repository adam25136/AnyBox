#include "WaveControlKernel.hpp"
#include "../../src/backend/control/Uniformity.hpp"
#include "Fixtures.hpp"

void CheckUniformLoopProof() {
    using namespace ShaderRecompiler;
    auto program = BufferTests::WaveControlKernel(4);
    IrBuilder builder(program);
    auto& merge = *program.BlockOrder().back();
    auto& phi = program.CreateValue(IrOpcode::Phi, IrType::U32);
    phi.AddPhiOperand(merge.Predecessors().at(0), &builder.Constant(5));
    phi.AddPhiOperand(merge.Predecessors().at(1), &builder.Constant(7));
    merge.InsertInstructionBefore(merge.Instructions().front(), &phi);
    ShaderComputeInputInfo info;
    info.threadsNum[0] = 128;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = 64;
    info.hostSubgroupSize = 32;
    ValidateProgram(program, true);
    AnyBox::Shader::Control::Uniformity uniform(program, info);
    Fixtures::Require(!uniform.Check(phi));
    Fixtures::Require(uniform.Check(builder.Constant(5)));
    Fixtures::Require(!uniform.Check(phi));
}
