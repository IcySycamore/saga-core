#pragma once
#include "../Dice/Dice.h"
#include "../Dice/Dice20.h"
#include "../Entity/EntityType.h"
#include "../Event/Event.h"
#include <climits>
#include <stdexcept>
#include <type_traits>
#include <utility>

enum class CheckType : uint8_t {
  noCheck = 0,  // 无意义，成功
  SFcheck,      // 成功等级（是否大成功），是否成功，检定目标属性与骰子大小
  opposedCheck, // 成功等级（是否大成功），是否成功，检定两者属性大小
  timedCheck,   // 无意义，是否成功，检定GM时间与骰子大小
  traitCheck,   // 无意义，是否成功，检定目标属性与固定值大小
  gradedCheck   // 成功等级，是否成功，检定目标属性与骰子大小
};

template <typename T> class Check : public Event {
private:
  EntityInstance *m_source;
  EntityInstance *m_target;
  CheckType m_check_type;
  unsigned
      m_numinfo; // 选定的属性(0-4)，当是时间检定时为系统时间，当是特性检定时是固定值
  T m_dice;

public:
  Check(EntityInstance *source, EntityInstance *target, CheckType ct = CheckType::noCheck,
        unsigned selectedAttribute = UINT_MAX);
  std::pair<unsigned, bool> getResult() const;
};

template <typename T> using multiCheck = Check<T> *[3];

template <typename T>
Check<T>::Check(EntityInstance *source, EntityInstance *target, CheckType ct, unsigned numinfo)
    : Event(), m_source(source), m_target(target), m_check_type(ct),
      m_numinfo(numinfo) {
  if (source->getStatue() == Statue::GM) {
    if (m_check_type == CheckType::opposedCheck) {
      throw std::runtime_error(
          "Initialize a opposedCheck for GM and another EntityInstance");
    }
    if constexpr (std::is_same_v<T, Dice20>)
      m_dice = dice::roll_D20();
    else
      m_dice = dice::roll_dice();
  }
}
/**
    @brief 从check对象返回结果
    @return std::pair<unsigned, bool> <成功等级, 是否成功>
    @details
   第一个返回值可能的取值：当检定定类型为成败与对抗时为大成功6，大失败1，无意义-1
             当检定类型为等级时为
                                1-2	完美成功	Perfect Success
                                3-4	部分成功	Partial Success
                                5-6	代价成功	Costly Success
            其他检定类型时始终为 -1
*/
template <typename T>
inline std::pair<unsigned, bool> Check<T>::getResult() const {
  auto dv = static_cast<unsigned>(m_dice);
  switch (m_check_type) {
  case CheckType::noCheck:
    return {-1, true}; // 总是成功
  case CheckType::SFcheck:
    return {(dv == 6 ? 6 : (dv == 0 ? 0 : -1)),
            m_target->getAttribute(m_numinfo) >= dv};
  case CheckType::opposedCheck:
    return {(dv == 6 ? 6 : (dv == 0 ? 0 : -1)),
            m_source->getAttribute(m_numinfo) >
                m_target->getAttribute(m_numinfo)};
  case CheckType::timedCheck:
    // TODO: 取系统时间与骰子比较
    return {-1, false};
  case CheckType::gradedCheck:
    return {dv < 3 ? 1 : (dv < 5 ? 2 : 3),
            m_target->getAttribute(m_numinfo) >= dv}; // 1-2;3-4;5-6
  default:
    return {-1, false};
  }
}
