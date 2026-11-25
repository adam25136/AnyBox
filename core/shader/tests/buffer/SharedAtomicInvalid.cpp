#include "SharedAtomicFixture.hpp"
#include "Fixtures.hpp"

void CheckSharedAtomicInvalid() {
    using namespace BufferTests;
    namespace Bitcode = AnyBox::Shader::Dxil::Bitcode;
    {
        SharedAtomicFixture source;
        auto* instruction = source.Instruction(source.Pointer(source.Integer(0)), source.Integer(1), SharedOperation::Add);
        Fixtures::Reject<std::runtime_error>([&] { (void)source.fixture.scalar.Get(*instruction); });
        for (auto values : {std::array<std::uint64_t, 3>{2, 6, 1}, {0, 0, 1}, {0, 1, 1}, {0, 7, 1}, {0, 6, UINT64_MAX}}) Fixtures::Require(!Bitcode::StoreAtomicAttributes(*instruction, values));
        std::array<std::uint64_t, 4> oversized{0, 6, 1, 0};
        Fixtures::Require(!Bitcode::StoreAtomicAttributes(*instruction, oversized));
        Fixtures::Require(!Bitcode::StoreAtomicAttributes(*instruction, std::span(oversized).first(2)));
        Fixtures::Require(Bitcode::StoreAtomicAttributes(*instruction, std::span(oversized).first(3)));
        const auto& attributes = Bitcode::GetAtomicAttributes(*instruction);
        Fixtures::Require(!attributes.volatileAccess && attributes.ordering == 6 && attributes.scope == 1);
        Fixtures::Require(!Bitcode::StoreAtomicAttributes(*instruction, std::span(oversized).first(3)));
    }
    for (auto values : {std::array<std::uint64_t, 3>{1, 6, 1}, {0, 3, 1}, {0, 4, 1}, {0, 5, 1}, {0, 6, 0}, {0, 6, 2}}) {
        SharedAtomicFixture source;
        auto* instruction = source.Instruction(source.Pointer(source.Integer(0)), source.Integer(1), SharedOperation::Add);
        Fixtures::Require(Bitcode::StoreAtomicAttributes(*instruction, values));
        Fixtures::Reject<std::runtime_error>([&] { (void)source.fixture.scalar.Get(*instruction); });
    }
    for (auto operation : {SharedOperation::Nand, SharedOperation::FAdd, SharedOperation::FSub}) {
        SharedAtomicFixture source;
        auto* instruction = source.Instruction(source.Pointer(source.Integer(0)), source.Integer(1), operation);
        Fixtures::Require(Bitcode::StoreAtomicAttributes(*instruction, std::array<std::uint64_t, 3>{0, 6, 1}));
        Fixtures::Reject<std::runtime_error>([&] { (void)source.fixture.scalar.Get(*instruction); });
    }
    for (auto space : {0u, 1u, 3u}) {
        SharedAtomicFixture source(space, space == 3);
        auto* instruction = source.Instruction(source.Pointer(source.Integer(0)), source.Integer(1), SharedOperation::Add);
        Fixtures::Require(Bitcode::StoreAtomicAttributes(*instruction, std::array<std::uint64_t, 3>{0, 6, 1}));
        Fixtures::Reject<std::runtime_error>([&] { (void)source.fixture.scalar.Get(*instruction); });
    }
    for (auto index : {256u, UINT32_MAX}) {
        SharedAtomicFixture source;
        auto* instruction = source.Instruction(source.Pointer(source.Integer(index)), source.Integer(1), SharedOperation::Add);
        Fixtures::Require(Bitcode::StoreAtomicAttributes(*instruction, std::array<std::uint64_t, 3>{0, 6, 1}));
        Fixtures::Reject<std::out_of_range>([&] { (void)source.fixture.scalar.Get(*instruction); });
    }
}
