#pragma once
#include "Entity.h"

class Player : public Entity
{
private:
public:
    Player(unsigned int san, unsigned int str, unsigned int cha,
           unsigned int edx, unsigned int per, Statue statue)
        : Entity(san, str, cha, edx, per, statue)
    {
    }
};
