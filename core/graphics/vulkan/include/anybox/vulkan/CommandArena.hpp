#pragma once

#include "QueueBridge.hpp"
#include <mutex>

namespace AnyBox::Vulkan {
struct CommandArenaState;
class CommandRecord;
class CommandReset;
class CommandSubmission;

class CommandArena {
public:
    explicit CommandArena(Context& context);
    CommandArena(const CommandArena&) = delete;
    CommandArena& operator=(const CommandArena&) = delete;
    CommandRecord Begin();
    CommandReset LockReset();
    CommandSubmission LockSubmission(const CommandRecord& record, const QueueBridge& queue);
    void Reset();

private:
    std::shared_ptr<CommandArenaState> state;
};

class CommandRecord {
public:
    ~CommandRecord();
    CommandRecord(CommandRecord&& other) noexcept;
    CommandRecord(const CommandRecord&) = delete;
    CommandRecord& operator=(const CommandRecord&) = delete;
    VkCommandBuffer GetCommands() const;
    VkCommandBuffer GetExecutable() const;
    void Close();
    void Retain(std::shared_ptr<void> resource);

private:
    friend class CommandArena;
    friend class CommandSubmission;
    CommandRecord(std::shared_ptr<CommandArenaState> state, VkCommandBuffer commands, std::uint64_t generation);
    std::shared_ptr<CommandArenaState> state;
    VkCommandBuffer commands;
    std::uint64_t generation;
    bool recording = true;
};

class CommandReset {
public:
    CommandReset(const CommandReset&) = delete;
    CommandReset& operator=(const CommandReset&) = delete;
    void Commit();

private:
    friend class CommandArena;
    explicit CommandReset(std::shared_ptr<CommandArenaState> state);
    std::shared_ptr<CommandArenaState> state;
    std::unique_lock<std::mutex> lock;
    bool committed = false;
};

class CommandSubmission {
public:
    ~CommandSubmission();
    CommandSubmission(const CommandSubmission&) = delete;
    CommandSubmission& operator=(const CommandSubmission&) = delete;
    VkCommandBuffer GetCommands() const noexcept { return commands; }

private:
    friend class CommandArena;
    CommandSubmission(std::shared_ptr<CommandArenaState> state, const CommandRecord& record, const QueueBridge& queue);
    std::shared_ptr<CommandArenaState> state;
    std::unique_lock<std::mutex> lock;
    const QueueBridge& queue;
    std::size_t slot;
    VkCommandBuffer commands;
};
}
