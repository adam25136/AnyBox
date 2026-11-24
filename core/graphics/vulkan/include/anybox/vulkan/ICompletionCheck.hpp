#pragma once

namespace AnyBox::Vulkan {
class ICompletionCheck {
public:
    virtual ~ICompletionCheck() = default;
    virtual void CheckCompletion() = 0;
};
}
