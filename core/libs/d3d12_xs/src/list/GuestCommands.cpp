#include "List.hpp"
#include "Compute.hpp"
#include "../pipeline/State.hpp"
#include <anybox/graphics/commands/Inline.hpp>
#include <stdexcept>

namespace {
class PipelineReferences {
public:
    explicit PipelineReferences(std::span<AnyBox::Graphics::PipelineState*> storage) : storage(storage) {}
    PipelineReferences(const PipelineReferences&) = delete;
    PipelineReferences& operator=(const PipelineReferences&) = delete;
    ~PipelineReferences() {
        for (std::size_t index = 0; index < count; ++index) AnyBox::Graphics::PipelineState::Release(storage[index]);
    }
    void Retain(AnyBox::Graphics::PipelineState* pipeline) {
        if (count == storage.size()) throw std::length_error("Inline pipeline reference capacity is exceeded");
        AnyBox::Graphics::PipelineState::AddRef(pipeline);
        storage[count++] = pipeline;
    }

private:
    std::span<AnyBox::Graphics::PipelineState*> storage;
    std::size_t count = 0;
};
}

void AnyBox::Graphics::CommandList::resetGuestCommands() noexcept {
    guest.commandWrite = inlineCommands.data();
    guest.dispatchEnd = inlineCommands.data() + inlineCommands.size() - commandSlack;
    guest.commandEnd = guest.dispatchEnd;
    std::uint32_t clean = 0;
    std::memcpy(guest.driverState.data() + 320 - 56, &clean, sizeof(clean));
    updateRootHeaders();
    cachePipeline();
}

void AnyBox::Graphics::CommandList::validateGuestCommands() const {
    auto begin = reinterpret_cast<std::uintptr_t>(inlineCommands.data());
    auto end = begin + sizeof(inlineCommands);
    auto visibleEnd = end - commandSlack * sizeof(std::uint32_t);
    auto write = reinterpret_cast<std::uintptr_t>(guest.commandWrite);
    if (write < begin || write > end || (write - begin) % sizeof(std::uint32_t) != 0 || reinterpret_cast<std::uintptr_t>(guest.dispatchEnd) != visibleEnd || reinterpret_cast<std::uintptr_t>(guest.commandEnd) != visibleEnd) throw std::logic_error("Xbox command stream bounds are corrupted");
}

void AnyBox::Graphics::CommandList::updateRootHeaders() noexcept {
    auto count = compute ? compute->GetRoot()->GetLayout().parameters.size() : 0;
    for (std::uint32_t index = 0; index < 64; ++index) {
        auto header = index < count ? Commands::RootHeader(index) : UINT32_MAX;
        std::memcpy(guest.driverState.data() + 0x344 - 56 + index * 4, &header, 4);
    }
}

void AnyBox::Graphics::CommandList::flushGuestCommands() {
    validateGuestCommands();
    if (guest.commandWrite == inlineCommands.data()) return;
    auto words = std::span(inlineCommands.data(), static_cast<std::size_t>(guest.commandWrite - inlineCommands.data()));
    auto* arguments = compute ? &compute->GetArguments() : nullptr;
    std::array<PipelineState*, std::tuple_size_v<decltype(inlineCommands)> / Commands::PipelineWordCount> retained;
    PipelineReferences references(retained);
    Commands::InlineReader validation(words);
    while (!validation.Empty()) {
        auto command = validation.Next(arguments);
        if (std::holds_alternative<Commands::Dispatch>(command)) throw std::runtime_error("Xbox inline compute dispatch execution is not implemented");
        if (auto* binding = std::get_if<Commands::PipelineBind>(&command)) {
            auto* selected = PipelineState::From(reinterpret_cast<void*>(static_cast<std::uintptr_t>(binding->address)), device);
            if (guest.type != D3D12_COMMAND_LIST_TYPE_DIRECT && guest.type != D3D12_COMMAND_LIST_TYPE_COMPUTE) throw std::runtime_error("Inline pipeline binding is not implemented for this command list type");
            references.Retain(selected);
        }
    }
    Commands::InlineReader reader(words);
    while (!reader.Empty()) {
        auto command = reader.Next(arguments);
        if (auto* update = std::get_if<Commands::RootWrite>(&command)) arguments->Write(update->parameter, update->offset, update->words);
        else if (auto* binding = std::get_if<Commands::PipelineBind>(&command)) bindPipeline(PipelineState::From(reinterpret_cast<void*>(static_cast<std::uintptr_t>(binding->address)), device));
    }
    guest.commandWrite = inlineCommands.data();
}
