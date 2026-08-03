#pragma once
#include "../helper/RandomEng.h"

enum class Dice20 : uint8_t
{
    one = 1,
    two,
    three,
    four,
    five,
    six,
    seven,
    eight,
    nine,
    ten,
    eleven,
    twelve,
    thirteen,
    fourteen,
    fifteen,
    sixteen,
    seventeen,
    nineteen,
    twenty
};

inline bool operator<(Dice20 a, Dice20 b)
{
    return static_cast<int>(a) < static_cast<int>(b);
}

inline bool operator>(Dice20 a, Dice20 b)
{
    return !(a < b);
}

inline bool operator==(Dice20 a, Dice20 b)
{
    return static_cast<int>(a) == static_cast<int>(b);
}

inline bool operator!=(Dice20 a, Dice20 b)
{
    return !(a == b);
}

inline bool operator<=(Dice20 a, Dice20 b)
{
    return !(a > b);
}

inline bool operator>=(Dice20 a, Dice20 b)
{
    return !(a < b);
}

namespace dice
{
    inline Dice20 roll_D20()
    {
        static std::uniform_int_distribution<int> dis(1, 20);
        return static_cast<Dice20>(dis(randomEng()));
    }
}