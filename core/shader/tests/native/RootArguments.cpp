#include "RootFixture.hpp"

void CheckNativeRootArguments() {
    using namespace AnyBox::Shader;
    using namespace Fixtures;
    for (unsigned wave : {32u, 64u}) {
        for (unsigned slot : {0u, 2u, 16u, 55u, 255u}) {
            NativeTests::RootFixture fixture(wave, slot);
            RootArgumentPlan plan(fixture.buffers, fixture.root);
            auto translation = fixture.Translate();
            BindNativeRootArguments(translation, fixture.roots, plan);
            Reject<std::logic_error>([&] { BindNativeRootArguments(translation, fixture.roots, plan); });
            Require(translation.program.Resources().userDataCount == 1);
            auto compiled = CompileCompute(translation.program, translation.input, BufferTests::Target(), {0, 0, 0, 128});
            Require(compiled.bindings.layout.userDataRegisters == std::vector<std::uint32_t>{0});
        }
    }
    NativeTests::RootFixture fixture(32, 16);
    RootArgumentPlan plan(fixture.buffers, fixture.root);
    auto reject = [&](auto roots) {
        auto translation = fixture.Translate();
        Reject<std::exception>([&] { BindNativeRootArguments(translation, roots, plan); });
    };
    auto roots = fixture.roots;
    roots[0].firstUserRegister = 17;
    reject(roots);
    roots = fixture.roots;
    roots[0].firstUserRegister = 4;
    reject(roots);
    roots = fixture.roots;
    roots[0].registerCount = 2;
    reject(roots);
    roots = fixture.roots;
    roots[0].shaderRegister = 1;
    reject(roots);
    roots = fixture.roots;
    roots[0].kind = AnyBox::Graphics::Container::NativeRootKind::Table;
    reject(roots);
    roots = fixture.roots;
    roots[0].tableOffset = 1;
    reject(roots);
    std::array duplicate{fixture.roots[0], fixture.roots[0]};
    reject(duplicate);
    roots = fixture.roots;
    roots[0].firstUserRegister = 255;
    roots[0].registerCount = 2;
    reject(roots);
}
