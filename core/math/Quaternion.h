#pragma once
/**
 * @brief 四元数（旋转）
 * @namespace math
 * @note 内存布局: 4 × float = 16 字节
 * @note 避免欧拉角万向锁；用于实体朝向、相机旋转、插值旋转
 * @note C++20，constexpr 优先，无 RTTI
 */

#include "Math.h"
#include "Vector3.h"
#include <cassert>
#include <cmath>

namespace math {

struct Quaternion {
  float x;
  float y;
  float z;
  float w;

  // ============================ 构造 ============================

  /// 默认单位四元数
  constexpr Quaternion() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
  constexpr Quaternion(float x_, float y_, float z_, float w_)
      : x(x_), y(y_), z(z_), w(w_) {}

  // ============================ 静态常量 ============================

  static constexpr Quaternion identity() { return Quaternion(0, 0, 0, 1); }

  // ============================ 运算符 ============================

  /// 四元数合成：this * rhs = 先应用 rhs 再应用 this
  constexpr Quaternion operator*(const Quaternion &rhs) const {
    return Quaternion(w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
                      w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x,
                      w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w,
                      w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z);
  }

  /// 旋转向量（将 v 按此四元数旋转）。q 应为单位四元数
  constexpr Vector3 operator*(const Vector3 &v) const {
    // v' = q * v * q⁻¹（q 单位时 q⁻¹ = conjugate）
    const Vector3 qv(x, y, z);
    const Vector3 t = 2.0f * cross(qv, v);
    return v + w * t + cross(qv, t);
  }

  constexpr bool operator==(const Quaternion &rhs) const {
    return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w;
  }
  constexpr bool operator!=(const Quaternion &rhs) const {
    return !(*this == rhs);
  }

  // ============================ 属性 ============================

  constexpr float lengthSquared() const {
    return x * x + y * y + z * z + w * w;
  }
  constexpr float length() const { return std::sqrt(lengthSquared()); }

  /// 共轭
  constexpr Quaternion conjugate() const { return Quaternion(-x, -y, -z, w); }

  /// 逆。零四元数防御
  constexpr Quaternion inverse() const {
    const float lenSq = lengthSquared();
    if (lenSq == 0.0f) {
      return identity();
    }
    const Quaternion conj = conjugate();
    const float inv = 1.0f / lenSq;
    return Quaternion(conj.x * inv, conj.y * inv, conj.z * inv, conj.w * inv);
  }

  // ============================ 归一化 ============================

  /// 归一化副本。零四元数返回单位四元数
  constexpr Quaternion normalized() const {
    const float lenSq = lengthSquared();
    if (lenSq == 0.0f || lenSq == 1.0f) {
      return lenSq == 0.0f ? identity() : *this;
    }
    const float inv = 1.0f / std::sqrt(lenSq);
    return Quaternion(x * inv, y * inv, z * inv, w * inv);
  }

  /// 就地归一化
  inline void normalize() {
    const float lenSq = lengthSquared();
    if (lenSq == 0.0f) {
      *this = identity();
    } else if (lenSq != 1.0f) {
      const float inv = 1.0f / std::sqrt(lenSq);
      x *= inv;
      y *= inv;
      z *= inv;
      w *= inv;
    }
  }
};

// ============================ 自由函数 ============================

/// 四元数点积（slerp/最短路径用）
constexpr float dot(const Quaternion &a, const Quaternion &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

/// 轴角构造。axis 会被归一化（零轴返回单位四元数）
inline Quaternion axisAngle(const Vector3 &axis, float angleRad) {
  const float lenSq = lengthSquared(axis);
  if (lenSq == 0.0f) {
    return Quaternion::identity();
  }
  const Vector3 n = axis * (1.0f / std::sqrt(lenSq));
  const float half = angleRad * 0.5f;
  const float s = std::sin(half);
  return Quaternion(n.x * s, n.y * s, n.z * s, std::cos(half));
}

/// 欧拉角构造（弧度，ZYX 应用顺序：先绕 X 再绕 Y 再绕 Z，即 q = qz·qy·qx）
/// 这是标准 ZYX 欧拉约定（与 Unity 一致）
/// @note 万向锁：pitch = ±90° 时 yaw/roll 不可唯一分解（本构造无歧义，
///       但 quatToEuler 提取时会奇异，见 TransformTools）
inline Quaternion euler(float pitchX, float yawY, float rollZ) {
  const float cx = std::cos(pitchX * 0.5f);
  const float sx = std::sin(pitchX * 0.5f);
  const float cy = std::cos(yawY * 0.5f);
  const float sy = std::sin(yawY * 0.5f);
  const float cz = std::cos(rollZ * 0.5f);
  const float sz = std::sin(rollZ * 0.5f);

  return Quaternion(sx * cy * cz - cx * sy * sz, // x
                    cx * sy * cz + sx * cy * sz, // y
                    cx * cy * sz - sx * sy * cz, // z
                    cx * cy * cz + sx * sy * sz  // w
  );
}

/// 球面线性插值（最短路径，双倍角处理）
/// @note t 会被钳制到 [0,1]（超出视为外推的调用方错误）
inline Quaternion slerp(const Quaternion &a, const Quaternion &b, float t) {
  // 钳制 t 到 [0,1]：slerp 语义是插值而非外推
  t = std::clamp(t, 0.0f, 1.0f);
  float cosTheta = dot(a, b);

  // 处理负点积（取最短路径）
  Quaternion b2 = b;
  if (cosTheta < 0.0f) {
    b2 = Quaternion(-b.x, -b.y, -b.z, -b.w);
    cosTheta = -cosTheta;
  }

  // 接近平行时退化为线性插值（避免除零）
  if (cosTheta > 0.9995f) {
    const Quaternion result(a.x + t * (b2.x - a.x), a.y + t * (b2.y - a.y),
                            a.z + t * (b2.z - a.z), a.w + t * (b2.w - a.w));
    return result.normalized();
  }

  const float theta = std::acos(std::clamp(cosTheta, -1.0f, 1.0f));
  const float sinTheta = std::sin(theta);
  const float wa = std::sin((1.0f - t) * theta) / sinTheta;
  const float wb = std::sin(t * theta) / sinTheta;
  return Quaternion(a.x * wa + b2.x * wb, a.y * wa + b2.y * wb,
                    a.z * wa + b2.z * wb, a.w * wa + b2.w * wb);
}

/// 朝向构造：让 forward 指向目标方向，up 指定上方向（世界系）
/// 万向锁防御：forward 与 up 平行时用 worldUp 替代
/// 约定：构造的旋转把世界 forward(+Z) 映射到目标 forward
inline Quaternion lookRotation(const Vector3 &forward, const Vector3 &up) {
  const Vector3 f = normalized(forward);
  if (lengthSquared(f) == 0.0f) {
    return Quaternion::identity();
  }

  Vector3 right = cross(up, f);
  if (lengthSquared(right) < EPSILON * EPSILON) {
    // forward 与 up 平行（万向锁）→ 选任意与 f 垂直的向量作为参考
    // 优先世界 up；若 f 与世界 up 平行则用世界 right
    const Vector3 ref =
        (std::abs(f.y) < 1.0f - EPSILON) ? Vector3::up() : Vector3::right();
    right = cross(ref, f);
  }
  right = normalized(right);
  const Vector3 trueUp = cross(f, right);

  // 旋转矩阵（列主序布局）：每列是一个基向量在目标系的像
  // 列0 = right, 列1 = trueUp, 列2 = f
  // 标准 3x3 → 四元数转换
  const float m00 = right.x;
  const float m10 = right.y;
  const float m20 = right.z;
  const float m01 = trueUp.x;
  const float m11 = trueUp.y;
  const float m21 = trueUp.z;
  const float m02 = f.x;
  const float m12 = f.y;
  const float m22 = f.z;

  const float trace = m00 + m11 + m22;
  if (trace > 0.0f) {
    const float s = std::sqrt(trace + 1.0f) * 2.0f;
    return Quaternion((m21 - m12) / s, (m02 - m20) / s, (m10 - m01) / s,
                      0.25f * s);
  } else if (m00 > m11 && m00 > m22) {
    const float s = std::sqrt(1.0f + m00 - m11 - m22) * 2.0f;
    return Quaternion(0.25f * s, (m01 + m10) / s, (m02 + m20) / s,
                      (m21 - m12) / s);
  } else if (m11 > m22) {
    const float s = std::sqrt(1.0f + m11 - m00 - m22) * 2.0f;
    return Quaternion((m01 + m10) / s, 0.25f * s, (m12 + m21) / s,
                      (m02 - m20) / s);
  } else {
    const float s = std::sqrt(1.0f + m22 - m00 - m11) * 2.0f;
    return Quaternion((m02 + m20) / s, (m12 + m21) / s, 0.25f * s,
                      (m10 - m01) / s);
  }
}

} // namespace math
