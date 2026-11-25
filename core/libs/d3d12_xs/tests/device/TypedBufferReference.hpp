#pragma once

#include <array>
#include <cstdint>

namespace AnyBox::Graphics::Tests {
struct TypedBufferReference {
    std::uint32_t format;
    std::uint32_t size;
    std::uint32_t control;
};
inline constexpr std::array TypedBufferReferences{
    TypedBufferReference{2, 16, 0x1004DFAC},
    TypedBufferReference{3, 16, 0x1004BFAC},
    TypedBufferReference{4, 16, 0x1004CFAC},
    TypedBufferReference{6, 12, 0x1004A3AC},
    TypedBufferReference{7, 12, 0x100483AC},
    TypedBufferReference{8, 12, 0x100493AC},
    TypedBufferReference{10, 8, 0x10047FAC},
    TypedBufferReference{11, 8, 0x10041FAC},
    TypedBufferReference{12, 8, 0x10045FAC},
    TypedBufferReference{13, 8, 0x10042FAC},
    TypedBufferReference{14, 8, 0x10046FAC},
    TypedBufferReference{16, 8, 0x1004022C},
    TypedBufferReference{17, 8, 0x1003E22C},
    TypedBufferReference{18, 8, 0x1003F22C},
    TypedBufferReference{24, 4, 0x10032FAC},
    TypedBufferReference{25, 4, 0x10036FAC},
    TypedBufferReference{26, 4, 0x100243AC},
    TypedBufferReference{28, 4, 0x10038FAC},
    TypedBufferReference{30, 4, 0x1003CFAC},
    TypedBufferReference{31, 4, 0x10039FAC},
    TypedBufferReference{32, 4, 0x1003DFAC},
    TypedBufferReference{34, 4, 0x1001D22C},
    TypedBufferReference{35, 4, 0x1001722C},
    TypedBufferReference{36, 4, 0x1001B22C},
    TypedBufferReference{37, 4, 0x1001822C},
    TypedBufferReference{38, 4, 0x1001C22C},
    TypedBufferReference{41, 4, 0x10016204},
    TypedBufferReference{42, 4, 0x10014204},
    TypedBufferReference{43, 4, 0x10015204},
    TypedBufferReference{49, 2, 0x1000E22C},
    TypedBufferReference{50, 2, 0x1001222C},
    TypedBufferReference{51, 2, 0x1000F22C},
    TypedBufferReference{52, 2, 0x1001322C},
    TypedBufferReference{54, 2, 0x1000D204},
    TypedBufferReference{56, 2, 0x10007204},
    TypedBufferReference{57, 2, 0x1000B204},
    TypedBufferReference{58, 2, 0x10008204},
    TypedBufferReference{59, 2, 0x1000C204},
    TypedBufferReference{61, 1, 0x10001204},
    TypedBufferReference{62, 1, 0x10005204},
    TypedBufferReference{63, 1, 0x10002204},
    TypedBufferReference{64, 1, 0x10006204}
};
}
