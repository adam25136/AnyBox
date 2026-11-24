#include <anybox/buffer/AddressSpace.hpp>
#include <atomic>
#include <thread>
#include <stdexcept>

void CheckConcurrentMappings() {
    using namespace AnyBox::Graphics::Buffers;
    AddressSpace space;
    auto owner = std::make_shared<int>(1);
    std::atomic<bool> done = false;
    std::atomic<bool> failed = false;
    std::thread writer([&] {
        try {
            for (unsigned iteration = 0; iteration < 4000; ++iteration) {
                auto mapping = space.Register(0x1000, 64, 0x8000, owner);
                std::this_thread::yield();
            }
        } catch (...) { failed = true; }
        done = true;
    });
    while (!done.load()) {
        try {
            auto mapping = space.Resolve(0x1010, 16);
            if (mapping->Translate(0x1010, 16) != 0x8010) failed = true;
        } catch (const std::out_of_range&) {}
        auto snapshot = space.Capture();
        for (const auto& mapping : snapshot.mappings) {
            if (mapping->Translate(0x1000, 64) != 0x8000) failed = true;
        }
    }
    writer.join();
    if (failed || !space.Capture().mappings.empty()) throw std::runtime_error("Concurrent GPU buffer mapping lifetime failed");
}
