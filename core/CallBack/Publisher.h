#pragma once
#include "Connection.h"

/**
 * @brief 在类内，创建一个目标为 ConnectionName 的名为 SignalName 的成员函数
 * @param SignalName     信号函数名
 * @param ConnectionName 目标 Connection 对象名
 * @return void
 * @details
 * 在类作用域内，创建一个指定名称的成员函数触发目标 Connection 的
 * 转发调用
 * @note 持有由 PUBLISHER 定义的方法的类称为信号源对象
 * 由 PUBLISHER 定义的信号必须由明确的信号源对象调用。
 * Connection 的生命周期总应该长于信号源对象
 * @par usage:
 * @code
 * // 多个PUBLISHER被绑定到此连接
 * Connection<void(const Player&)> player_changed_connection;
 *
 * class LoginService {
 * public:
 *   SUBSCRIBER(onPlayerChanged, player_changed_connection, void(const Player&));
 *   PUBLISHER(PlayerLogin,  player_changed_connection);  // public 信号
 *   PUBLISHER(PlayerLogout, player_changed_connection);
 * private:
 *   PUBLISHER(onPlayerInternal, player_changed_connection); // private 信号
 * };
 * @endcode
 */
#define PUBLISHER(SignalName, ConnectionName)                                  \
  template <typename... Args> inline void SignalName(Args &&...args) {         \
    ConnectionName.emit(std::forward<Args>(args)...);                          \
  }
