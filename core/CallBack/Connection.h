#pragma once
#include <utility>

namespace lCYC::callback {
template <typename Sig> class Subscriber;
/**
 * @brief 绑定一个 subscriber
 * 的连接对象。总应该在全局作用域定义或维持尽可能长的生命周期
 * @details
 * 若信号源对象析构，自然不会再触发信号；如果接受者Subscriber析构，会自动无效化
 * Connection 调用
 * @par usage:
 * @code lCYC::callback::Connection<void(const Player&)> web_connection;
 * @endcode
 */
template <typename Sig> class Connection {
private:
  Subscriber<Sig> *m_receiver = nullptr;

public:
  Connection() = default;
  Connection(Connection<Sig> &) = delete;
  Connection(Connection<Sig> &&) = delete;
  void bind_to(Subscriber<Sig> *receiver_) { m_receiver = receiver_; }
  template <typename... Args> void emit(Args &&...args) {
    if (m_receiver && m_receiver->valid())
      m_receiver->operator()(std::forward<Args>(args)...);
  }
};
} // namespace lCYC::callback
