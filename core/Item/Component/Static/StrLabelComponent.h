#pragma once
#include "StaticComponent.h"
#include <string>

struct StrLabelComponent : public StaticComponent {
  std::string m_str_label;
};