#pragma once

#include "../helper/RandomEng.h"
enum class Dice : uint8_t
{
    one = 1,
    two,
    three,
    four,
    five,
    six
};
inline bool operator<(Dice a, Dice b)
{
    return static_cast<int>(a) < static_cast<int>(b);
}

inline bool operator>(Dice a, Dice b)
{
    return !(a < b);
}

inline bool operator==(Dice a, Dice b)
{
    return static_cast<int>(a) == static_cast<int>(b);
}

inline bool operator!=(Dice a, Dice b)
{
    return !(a == b);
}

inline bool operator<=(Dice a, Dice b)
{
    return !(a > b);
}

inline bool operator>=(Dice a, Dice b)
{
    return !(a < b);
}
namespace dice
{
    inline Dice roll_dice()
    {
        static std::uniform_int_distribution<int> dis(1, 6);
        return static_cast<Dice>(dis(randomEng()));
    }
}