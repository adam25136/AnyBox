#include "Uniformity.hpp"

using namespace ShaderRecompiler;
using AnyBox::Shader::Control::Uniformity;

Uniformity::Uniformity(const IrProgram& program, const ShaderComputeInputInfo& inputInfo) : input(inputInfo) {
    std::vector<const IrValue*> controls;
    for (const auto& info : program.Metadata().blockInfo) {
        if (info.terminator.kind != TerminatorKind::ConditionalBranch) continue;
        if (info.terminator.condition != BranchCondition::ScalarInstruction || !info.condition) phiControlValid = false;
        else {
            controls.push_back(info.condition->Resolve());
            controlRoots.insert(info.condition->Resolve());
        }
    }
    controlsUniform = phiControlValid;
    extend(controls);
}

void Uniformity::extend(std::span<const IrValue* const> roots) {
    std::vector<const IrValue*> pending(roots.begin(), roots.end());
    std::vector<const IrValue*> added;
    while (!pending.empty()) {
        auto* value = pending.back()->Resolve();
        pending.pop_back();
        if (nodes.contains(value)) continue;
        auto dependencies = classify(*value);
        auto opcode = value->Opcode();
        bool needsControl = value->IsPhi() || opcode == IrOpcode::ReadFirstLane || opcode == IrOpcode::ReadLane || opcode == IrOpcode::Ballot;
        Node node{dependencies.has_value(), needsControl, {}, {}};
        if (dependencies) node.dependencies = std::move(*dependencies);
        for (auto* dependency : node.dependencies) pending.push_back(dependency);
        if (needsControl) controlUsers.push_back(value);
        nodes.emplace(value, std::move(node));
        added.push_back(value);
    }
    for (auto* value : added) {
        for (auto* dependency : nodes.at(value).dependencies) nodes.at(dependency).users.push_back(value);
    }
    pending.clear();
    for (auto* value : added) {
        auto& node = nodes.at(value);
        bool uniform = node.uniform && (!node.needsControl || controlsUniform);
        for (auto* dependency : node.dependencies) uniform &= nodes.at(dependency).uniform;
        if (!uniform) {
            node.uniform = false;
            pending.push_back(value);
        }
    }
    while (!pending.empty()) {
        auto* value = pending.back();
        pending.pop_back();
        if (controlsUniform && controlRoots.contains(value)) {
            controlsUniform = false;
            for (auto* user : controlUsers) {
                auto& node = nodes.at(user);
                if (!node.uniform) continue;
                node.uniform = false;
                pending.push_back(user);
            }
        }
        for (auto* user : nodes.at(value).users) {
            auto& node = nodes.at(user);
            if (!node.uniform) continue;
            node.uniform = false;
            pending.push_back(user);
        }
    }
}

bool Uniformity::Check(const IrValue& expression) {
    const auto* value = expression.Resolve();
    if (!nodes.contains(value)) extend(std::span(&value, 1));
    return nodes.at(value).uniform;
}
