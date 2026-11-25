#include "UniformKernel.hpp"
#include "FloatFixture.hpp"

void CheckIndexedLaneInvalid() {
    BufferTests::FloatFixture fixture;
    for (auto overload : {std::pair{1u, false}, {32u, false}, {64u, false}, {32u, true}}) {
        auto [bits, floating] = overload;
        auto* type = floating ? fixture.FloatType() : fixture.IntegerType(bits);
        auto* name = BufferTests::IndexedLaneName(bits, floating);
        auto input = [&]() -> LLVMBC::Value* { return floating ? static_cast<LLVMBC::Value*>(fixture.FloatConstant(0x80000000)) : fixture.Integer(bits, 1); };
        auto arguments = [&] { return LLVMBC::Vector<LLVMBC::Value*>{fixture.Integer(32, 117), input(), fixture.Integer(32, 0)}; };
        auto reject = [&](LLVMBC::Type* result, LLVMBC::Vector<LLVMBC::Value*> values) { fixture.Reject(*fixture.Call(name, result, std::move(values))); };
        reject(type, {});
        auto extra = arguments();
        extra.push_back(input());
        reject(type, std::move(extra));
        reject(fixture.IntegerType(16), arguments());
        for (unsigned index = 0; index < 3; ++index) {
            auto values = arguments();
            values[index] = fixture.Integer(16, 0);
            reject(type, std::move(values));
        }
        for (auto* opcode : std::array<LLVMBC::Value*, 2>{fixture.Integer(32, 118), fixture.Make<LLVMBC::Argument>(fixture.IntegerType(32), 0, false)}) {
            auto values = arguments();
            values[0] = opcode;
            reject(type, std::move(values));
        }
        for (auto index : {32u, 64u, UINT32_MAX}) {
            auto values = arguments();
            values[2] = fixture.Integer(32, index);
            reject(type, std::move(values));
        }
        if (bits == 32) {
            auto values = arguments();
            values[1] = floating ? static_cast<LLVMBC::Value*>(fixture.Integer(32, 0)) : fixture.FloatConstant(0);
            reject(type, std::move(values));
        }
    }
}
