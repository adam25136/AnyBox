#include "AddressLoad.hpp"
#include <IntermediateRepresentation/IrProgram.hpp>
#include <unordered_map>

namespace {
using namespace ShaderRecompiler;
class Uniformity {
public:
    explicit Uniformity(const IrProgram& program) : program(program) {}
    bool Check(const IrValue& input) {
        const auto& value = *input.Resolve();
        if (value.HasImmediate()) return true;
        if (auto found = known.find(&value); found != known.end()) return found->second;
        known.emplace(&value, false);
        auto opcode = value.Opcode();
        if (opcode == IrOpcode::GetUserData || opcode == IrOpcode::GetScalarRegister || opcode == IrOpcode::ReadFirstLane) return known[&value] = true;
        if (opcode == IrOpcode::GetBuiltin) {
            bool uniform = value.ArgumentCount() == 2 && value.Argument(0)->HasImmediate() && value.Argument(0)->ImmediateU32() == static_cast<std::uint32_t>(StageInputKind::WorkgroupId) && value.Argument(1)->HasImmediate() && value.Argument(1)->ImmediateU32() < 3;
            return known[&value] = uniform;
        }
        if (value.IsPhi() || opcode == IrOpcode::GetVectorRegister || value.ArgumentCount() == 0) return false;
        auto access = AddressOpcodeInfoOf(opcode).access;
        if (access == AddressAccess::Read) {
            auto flags = value.Flags<MemoryFlags>();
            const auto& memory = program.Resources().memoryInfo.at(flags.index);
            if (memory.kind != ResourceKind::Global && memory.kind != ResourceKind::ScalarAddress) return false;
        } else if (IrOpcodeHasSideEffects(opcode)) return false;
        for (auto* argument : value.Arguments()) if (!Check(*argument)) return false;
        return known[&value] = true;
    }

private:
    const IrProgram& program;
    std::unordered_map<const IrValue*, bool> known;
};
}

bool AnyBox::Shader::Dxil::IsUniformAddress(const ShaderRecompiler::IrValue& value, const ShaderRecompiler::IrProgram& program) {
    return Uniformity(program).Check(value);
}
