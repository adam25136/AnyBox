#pragma once

#include <thread_local_allocator.hpp>
#include <stdexcept>
#include "bitcode/AtomicAttributes.hpp"

namespace AnyBox::Shader {
class DxilSession {
public:
    DxilSession() {
        if (active) throw std::logic_error("DXIL compiler session cannot be reentered on the same thread");
        Dxil::Bitcode::ResetAtomicAttributes();
        dxil_spv::begin_thread_allocator_context();
        active = true;
    }
    ~DxilSession() {
        Dxil::Bitcode::ResetAtomicAttributes();
        dxil_spv::end_thread_allocator_context();
        active = false;
    }
    DxilSession(const DxilSession&) = delete;
    DxilSession& operator=(const DxilSession&) = delete;

private:
    inline static thread_local bool active = false;
};
}
