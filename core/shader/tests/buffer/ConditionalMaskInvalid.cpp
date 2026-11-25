#include "FloatFixture.hpp"
#include "Fixtures.hpp"
#include "lowering/vector/Words.hpp"

void CheckConditionalMaskInvalid() {
    using namespace BufferTests;
    FloatFixture fixture;
    auto* word = fixture.IntegerType(32);
    auto* pair = LLVMBC::VectorType::get(2, word);
    auto* values = fixture.Make<LLVMBC::ConstantDataVector>(pair, LLVMBC::Vector<LLVMBC::Value*>{fixture.Integer(32, 0), fixture.Integer(32, 1)});
    auto arguments = [&] { return LLVMBC::Vector<LLVMBC::Value*>{fixture.Integer(32, 170), values, values}; };
    auto reject = [&](LLVMBC::Type* type, LLVMBC::Vector<LLVMBC::Value*> args) { fixture.Reject(*fixture.Call("xbox.V_CNDMASK_B32", type, std::move(args))); };
    reject(word, {});
    reject(fixture.FloatType(), arguments());
    auto extra = arguments();
    extra.push_back(values);
    reject(word, std::move(extra));
    for (auto* opcode : std::array<LLVMBC::Value*, 3>{fixture.Integer(32, 169), fixture.Integer(16, 170), fixture.Make<LLVMBC::Argument>(word, 0, false)}) {
        auto args = arguments();
        args[0] = opcode;
        reject(word, std::move(args));
    }
    for (unsigned position : {1u, 2u}) {
        for (auto* invalid : std::array<LLVMBC::Value*, 3>{fixture.Integer(32, 0), fixture.Make<LLVMBC::ConstantAggregateZero>(LLVMBC::VectorType::get(3, word)), fixture.Make<LLVMBC::ConstantAggregateZero>(LLVMBC::VectorType::get(2, fixture.IntegerType(16)))}) {
            auto args = arguments();
            args[position] = invalid;
            reject(word, std::move(args));
        }
        auto args = arguments();
        args[position] = fixture.Make<LLVMBC::ConstantDataVector>(pair, LLVMBC::Vector<LLVMBC::Value*>{fixture.Integer(32, 0)});
        reject(word, std::move(args));
        args = arguments();
        args[position] = fixture.Make<LLVMBC::ConstantDataVector>(pair, LLVMBC::Vector<LLVMBC::Value*>{fixture.Integer(16, 0), fixture.Integer(32, 1)});
        reject(word, std::move(args));
        args = arguments();
        args[position] = fixture.Make<LLVMBC::Argument>(pair, 0, false);
        reject(word, std::move(args));
    }
}
