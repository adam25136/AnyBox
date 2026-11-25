#include "Tests.hpp"
#include "DxgiReference.hpp"
#include <system_error>
#include <vector>

void AnyBox::Graphics::Tests::CheckFrameConfiguration(Device* device) {
    auto configure = Method<decltype(&Device::SetFrameIntervalX)>(device, 78);
    Require(device->GetFrameConfiguration() == FrameConfiguration{}, "New graphics device has an unexpected frame configuration");
    for (auto interval : {16667u, 16683u, 33333u, 33367u, 50000u, 50049u, 66667u, 66734u}) {
        for (auto periodicity : {0u, 1u, 3u, 18u}) {
            Require(configure(device, nullptr, interval, periodicity, 0) == S_FALSE, "Xbox frame interval did not report its deferred configuration");
            Require(device->GetFrameConfiguration() == FrameConfiguration{interval, periodicity}, "Xbox frame interval configuration was not retained");
        }
    }

    auto preserved = device->GetFrameConfiguration();
    for (auto interval : {0u, 1u, 8332u, 8343u, 16666u, 16684u, 33332u, 33368u, 100000u, 0xFFFFFFFFu}) Reject<std::invalid_argument>([&] { configure(device, nullptr, interval, 3, 0); });
    Reject<std::runtime_error>([&] { configure(device, device, 16667, 3, 0); });
    Reject<std::runtime_error>([&] { configure(device, nullptr, 16667, 3, 1); });
    Require(device->GetFrameConfiguration() == preserved, "Rejected frame configuration changed the pending frame interval");

    auto luid = Method<decltype(&Device::GetAdapterLuid)>(device, 43)(device);
    auto output = GetReferenceOutput({luid.LowPart, luid.HighPart});
    bool highRefresh = false;
    if (output) {
        for (auto format : {DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_B8G8R8X8_UNORM, DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_R16G16B16A16_FLOAT}) {
            UINT count = 0;
            auto result = output->GetDisplayModeList1(format, 0, &count, nullptr);
            if (result == DXGI_ERROR_NOT_FOUND || (result == S_OK && count == 0)) continue;
            Require(result == S_OK, "Native frame refresh mode count failed");
            std::vector<DXGI_MODE_DESC1> modes(count);
            Require(output->GetDisplayModeList1(format, 0, &count, modes.data()) == S_OK, "Native frame refresh mode enumeration failed");
            for (const auto& mode : modes) highRefresh |= mode.RefreshRate.Numerator == 120ull * mode.RefreshRate.Denominator;
        }
    }
    for (auto interval : {8333u, 8342u, 25000u, 25025u, 41667u, 41708u}) {
        if (!output) {
            bool missingOutput = false;
            Reject<std::system_error>([&] {
                try { configure(device, nullptr, interval, 3, 0); }
                catch (const std::system_error& error) {
                    missingOutput = error.code().value() == static_cast<int>(DXGI_ERROR_NOT_FOUND);
                    throw;
                }
            });
            Require(missingOutput, "Headless adapter reported an unexpected high refresh error");
            Require(device->GetFrameConfiguration() == preserved, "Headless adapter did not retain its frame configuration");
        } else if (highRefresh) {
            Require(configure(device, nullptr, interval, 3, 0) == S_FALSE, "120 Hz display rejected a high refresh frame interval");
            Require(device->GetFrameConfiguration() == FrameConfiguration{interval, 3}, "High refresh frame interval was not retained");
        } else {
            Reject<std::invalid_argument>([&] { configure(device, nullptr, interval, 3, 0); });
            Require(device->GetFrameConfiguration() == preserved, "Unsupported high refresh interval changed the frame configuration");
        }
    }
}
