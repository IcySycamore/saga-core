#pragma once
#include <functional>

namespace lCYC::callback {
template <typename Sig> class Connection;

/**
 * @brief 订阅器，构造时绑定到外部 Connection
 * @tparam Sig 函数签名 RT(Args...)，如 void(const Player&)、int(const Player&)
 * @exception 调用 opertor() 会抛 std::bad_function_call
 * @note Connection 生命周期必须长于 Subscriber ，总是在类外部声明一个connection
 */
template <typename Sig> class Subscriber {
private:
  using Func = std::function<Sig>;
  Func m_func;
  Connection<Sig> *m_connection = nullptr;

public:
  Subscriber() = default;
  Subscriber(Subscriber<Sig> &) = delete;
  Subscriber(Subscriber<Sig> &&) = delete;
  explicit Subscriber(Func func_, Connection<Sig> *connection_)
      : m_func(std::move(func_)), m_connection(connection_) {
    connection_->bind_to(this);
  }
  ~Subscriber() {
    if (m_connection && m_connection->getReceiver() == this)
      m_connection->bind_to(nullptr);
  }
  template <typename... Args> Func::result_type operator()(Args &&...args) {
    return m_func(std::forward<Args>(args)...);
  }
  bool valid() const { return static_cast<bool>(m_func); }
};
} // namespace lCYC::callback

/**
 * @brief 在类内声明一个槽成员并绑定到指定的 Connection
 * @param SlotName       本类的槽成员函数名，签名与 Signature 一致
 * @param ConnectionName Connection 对象名
 * @param Signature      函数签名 RT(Args...)，如 void(const Player&, int)
 * @note Connection 必须全局定义。Subscriber 析构时 connection 自动失效
 * @note SlotName 槽函数的声明必须先于宏声明,且不应该实现为重载函数
 */
#define SUBSCRIBER(SlotName, ConnectionName, Signature)                        \
  lCYC::callback::Subscriber<Signature> m_SlotName##_subscriber{               \
      [this](auto &&...args) {                                                 \
        return this->SlotName(std::forward<decltype(args)>(args)...);          \
      },                                                                       \
      &ConnectionName};