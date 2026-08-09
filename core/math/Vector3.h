#pragma once
#include <cassert>
#include <cmath>
#include <string>

namespace lCYC::math {

/**
 * @brief 三维向量连续空间
 * @note  内存布局: 3 × float = 12 字节
 * @namespace lCYC::math
 */
struct Vector3 {
  float x;
  float y;
  float z;

  // ============================ 构造 ============================

  constexpr Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
  constexpr Vector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
  explicit constexpr Vector3(float scalar) : x(scalar), y(scalar), z(scalar) {}

  // ============================ 静态常量 ============================

  static constexpr Vector3 zero() { return Vector3(0.0f, 0.0f, 0.0f); }
  /// 全 1 向量
  static constexpr Vector3 one() { return Vector3(1.0f, 1.0f, 1.0f); }
  static constexpr Vector3 up() { return Vector3(0.0f, 1.0f, 0.0f); }
  static constexpr Vector3 down() { return Vector3(0.0f, -1.0f, 0.0f); }
  static constexpr Vector3 left() { return Vector3(-1.0f, 0.0f, 0.0f); }
  static constexpr Vector3 right() { return Vector3(1.0f, 0.0f, 0.0f); }
  static constexpr Vector3 forward() { return Vector3(0.0f, 0.0f, 1.0f); }
  static constexpr Vector3 back() { return Vector3(0.0f, 0.0f, -1.0f); }

  // ============================ 索引访问 ============================

  constexpr float &operator[](std::size_t index) {
    assert(index < 3 && "Vector3: index out of range");
    return (&x)[index];
  }
  constexpr const float &operator[](std::size_t index) const {
    assert(index < 3 && "Vector3: index out of range");
    return (&x)[index];
  }

  // ============================ 运算符 ============================

  constexpr Vector3 operator+(const Vector3 &rhs) const {
    return Vector3(x + rhs.x, y + rhs.y, z + rhs.z);
  }
  constexpr Vector3 operator-(const Vector3 &rhs) const {
    return Vector3(x - rhs.x, y - rhs.y, z - rhs.z);
  }
  constexpr Vector3 operator*(float scalar) const {
    return Vector3(x * scalar, y * scalar, z * scalar);
  }
  constexpr Vector3 operator*(const Vector3 &rhs) const {
    return Vector3(x * rhs.x, y * rhs.y, z * rhs.z);
  }
  constexpr Vector3 operator/(float scalar) const {
    assert(scalar != 0.0f && "Vector3: division by zero");
    const float inv = 1.0f / scalar;
    return Vector3(x * inv, y * inv, z * inv);
  }
  constexpr Vector3 operator-() const { return Vector3(-x, -y, -z); }

  constexpr Vector3 &operator+=(const Vector3 &rhs) {
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    return *this;
  }
  constexpr Vector3 &operator-=(const Vector3 &rhs) {
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    return *this;
  }
  constexpr Vector3 &operator*=(float scalar) {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
  }
  constexpr Vector3 &operator/=(float scalar) {
    assert(scalar != 0.0f && "Vector3: division by zero");
    const float inv = 1.0f / scalar;
    x *= inv;
    y *= inv;
    z *= inv;
    return *this;
  }

  constexpr bool operator==(const Vector3 &rhs) const {
    return x == rhs.x && y == rhs.y && z == rhs.z;
  }
  constexpr bool operator!=(const Vector3 &rhs) const {
    return !(*this == rhs);
  }

  // ============================ 调试 ============================

  std::string toString() const {
    return "( " + std::to_string(x) + ", " + std::to_string(y) + ", " +
           std::to_string(z) + " )";
  }
};

// ============================ 自由函数 ============================

/// 点积
constexpr float dot(const Vector3 &a, const Vector3 &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

/// 叉积
constexpr Vector3 cross(const Vector3 &a, const Vector3 &b) {
  return Vector3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                 a.x * b.y - a.y * b.x);
}

/// 长度平方
constexpr float lengthSquared(const Vector3 &v) { return dot(v, v); }

/// 长度
constexpr float length(const Vector3 &v) { return std::sqrt(lengthSquared(v)); }

/// 两点距离
constexpr float distance(const Vector3 &a, const Vector3 &b) {
  return length(a - b);
}

/// 归一化副本。零向量：返回零向量
constexpr Vector3 normalized(const Vector3 &v) {
  const float lenSq = lengthSquared(v);
  if (lenSq == 0.0f || lenSq == 1.0f) {
    return v;
  }
  return v * (1.0f / std::sqrt(lenSq));
}

/// 就地归一化。零向量：归零
inline void normalize(Vector3 &v) {
  const float lenSq = lengthSquared(v);
  if (lenSq == 0.0f || lenSq == 1.0f) {
    return;
  }
  v *= 1.0f / std::sqrt(lenSq);
}

/**
 * @brief 钳制向量长度到 maxLen，零向量安全
 * @param v 输入向量
 * @param maxLen 最大长度（< 0 按 0 处理）
 * @return 钳制后的向量
 */
constexpr Vector3 clampMagnitude(const Vector3 &v, float maxLen) {
  assert(maxLen >= 0.0f && "clampMagnitude: maxLen must be non-negative");
  const float clampedMax = maxLen < 0.0f ? 0.0f : maxLen;
  const float lenSq = lengthSquared(v);
  if (lenSq > clampedMax * clampedMax) {
    return v * (clampedMax / std::sqrt(lenSq));
  }
  return v;
}

/// 两向量弧度角，[0, π]。零向量返回 0
constexpr float angle(const Vector3 &a, const Vector3 &b) {
  const float lenSqA = lengthSquared(a);
  const float lenSqB = lengthSquared(b);
  if (lenSqA == 0.0f || lenSqB == 0.0f) {
    return 0.0f;
  }
  const float cosTheta =
      std::clamp(dot(a, b) / std::sqrt(lenSqA * lenSqB), -1.0f, 1.0f);
  return std::acos(cosTheta);
}

/// 反射: v - 2 * dot(v, n) * n（n 应为单位向量）
constexpr Vector3 reflect(const Vector3 &v, const Vector3 &normal) {
  return v - normal * (2.0f * dot(v, normal));
}

/// 左操作数标量乘法
constexpr Vector3 operator*(float scalar, const Vector3 &v) {
  return v * scalar;
}

} // namespace lCYC::math
