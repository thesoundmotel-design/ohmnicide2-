#pragma once
namespace Models
{
    constexpr int count = 8;
    inline const char* name (int i)
    {
        static const char* n[] = { "Soft Clip", "Hard Clip", "Tube", "Fold", "Rectify", "Bit Crush", "Sine", "Fuzz" };
        return n[i < 0 ? 0 : (i >= count ? count - 1 : i)];
    }
}
