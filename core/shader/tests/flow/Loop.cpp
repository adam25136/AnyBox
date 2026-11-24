#include "Fixture.hpp"
#include "Fixtures.hpp"
#include <anybox/ExceptionTest.hpp>

namespace {
void CheckLoop() {
    FlowTests::Fixture fixture;
    auto* entry = fixture.Block();
    auto* header = fixture.Block();
    auto* exit = fixture.Block();
    auto* body = fixture.Block();
    fixture.Emit<LLVMBC::BranchInst>(entry, header);
    auto* counter = fixture.Emit<LLVMBC::PHINode>(header, LLVMBC::Type::getInt16Ty(fixture.Context()), 2);
    auto* first = fixture.Emit<LLVMBC::PHINode>(header, LLVMBC::Type::getInt16Ty(fixture.Context()), 2);
    auto* second = fixture.Emit<LLVMBC::PHINode>(header, LLVMBC::Type::getInt16Ty(fixture.Context()), 2);
    auto* condition = fixture.Emit<LLVMBC::ICmpInst>(header, LLVMBC::Instruction::ICMP_ULT, counter, fixture.Constant(5));
    fixture.Emit<LLVMBC::BranchInst>(header, body, exit, condition);
    auto* increment = fixture.Emit<LLVMBC::BinaryOperator>(body, counter, fixture.Constant(1), LLVMBC::Instruction::Add);
    fixture.Emit<LLVMBC::BranchInst>(body, header);
    fixture.Emit<LLVMBC::ReturnInst>(exit, nullptr);
    counter->add_incoming(fixture.Constant(0), entry);
    counter->add_incoming(increment, body);
    first->add_incoming(fixture.Constant(17), entry);
    first->add_incoming(second, body);
    second->add_incoming(fixture.Constant(29), entry);
    second->add_incoming(first, body);
    auto program = fixture.Lower();
    Fixtures::Require(FlowTests::Evaluate(program) == std::vector<std::uint32_t>{5, 29, 17});
}

void CheckInvalid() {
    FlowTests::Fixture fixture;
    auto* entry = fixture.Block();
    auto* merge = fixture.Block();
    fixture.Emit<LLVMBC::BranchInst>(entry, merge);
    fixture.Emit<LLVMBC::PHINode>(merge, LLVMBC::Type::getInt16Ty(fixture.Context()), 0);
    fixture.Emit<LLVMBC::ReturnInst>(merge, nullptr);
    Fixtures::Reject<std::runtime_error>([&] { (void)fixture.Lower(); });
}
}

int main() {
    return AnyBox::Tests::Run([] {
        CheckLoop();
        CheckInvalid();
        FlowTests::CheckSwitches();
        FlowTests::CheckGlobalLoads();
        FlowTests::CheckBarriers();
        FlowTests::CheckAggregates();
    });
}
