#pragma once
#include "../helper/UuidGen.h"
#include <boost/uuid.hpp>
enum class Statue
{
    dead,
    live,
    crazy,
    burning,
    hurted,
    GM
};

/// @brief 所有可参与检定的角色/怪物/NPC 的基础类型
class Entity
{
protected:
    // 理智	Sanity
    unsigned int m_san;
    // 体力	Strength
    unsigned int m_str;
    // 魅力	Charisma
    unsigned int m_cha;
    // 敏捷	Dexterity
    unsigned int m_edx;
    // 感知	Perception
    unsigned int m_per;
    Statue m_statue;
    boost::uuids::uuid m_id;

public:
    Entity(unsigned int san, unsigned int str, unsigned int cha,
           unsigned int edx, unsigned int per, Statue statue = Statue::live)
        : m_san(san), m_str(str), m_cha(cha), m_edx(edx),
          m_per(per), m_statue(statue),
          m_id(uuidGen())
    {
    }
    virtual ~Entity() = default;

    virtual unsigned getSan() const { return m_san; }
    virtual unsigned getStr() const { return m_str; }
    virtual unsigned getCha() const { return m_cha; }
    virtual unsigned getEdx() const { return m_edx; }
    virtual unsigned getPer() const { return m_per; }
    virtual Statue getStatue() const { return m_statue; }
    virtual unsigned getAttribute(unsigned selsection) const
    {
        switch (selsection)
        {
        case 0:
            return m_san;
        case 1:
            return m_str;
        case 2:
            return m_cha;
        case 3:
            return m_edx;
        case 4:
            return m_per;
        default:
            throw std::out_of_range("Invalid attribute selection");
        }
    }

    virtual void setSan(unsigned san) { m_san = san; }
    virtual void setStr(unsigned str) { m_str = str; }
    virtual void setCha(unsigned cha) { m_cha = cha; }
    virtual void setEdx(unsigned edx) { m_edx = edx; }
    virtual void setPer(unsigned per) { m_per = per; }
    virtual void setStatue(Statue statue) { m_statue = statue; }

    bool operator==(const Entity &other) const
    {
        return m_san == other.m_san && m_str == other.m_str && m_cha == other.m_cha && m_edx == other.m_edx && m_per == other.m_per && m_statue == other.m_statue;
    }
    bool operator!=(const Entity &other) const { return !(*this == other); }
};
