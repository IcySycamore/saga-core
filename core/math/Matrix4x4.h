#pragma once
/**
 * @brief 四阶矩阵
 * @namespace lCYC::math
 * @note 存储约定: 列主序 column-major，m[col * 4 + row]
 */

#include "Math.h"
#include "Quaternion.h"
#include "Vector3.h"
#include <cassert>
#include <cmath>

namespace lCYC::math {

struct Matrix4x4 {
  float m[16];

  // ============================ 构造 ============================
  constexpr Matrix4x4() : m{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1} {}
  constexpr explicit Matrix4x4(const float values[16]) {
    for (int i = 0; i < 16; ++i) {
      m[i] = values[i];
    }
  }

  // ============================ 静态常量 ============================

  static constexpr Matrix4x4 identity() { return Matrix4x4(); }

  // ============================ 索引访问 ============================
  ///  (row, col) 访问
  constexpr float &at(int row, int col) {
    assert(row >= 0 && row < 4 && col >= 0 && col < 4 &&
           "Matrix4x4: index OOR");
    return m[col * 4 + row];
  }
  constexpr const float &at(int row, int col) const {
    assert(row >= 0 && row < 4 && col >= 0 && col < 4 &&
           "Matrix4x4: index OOR");
    return m[col * 4 + row];
  }

  /// 线性访问
  constexpr float &operator[](int index) {
    assert(index >= 0 && index < 16 && "Matrix4x4: index OOR");
    return m[index];
  }
  constexpr const float &operator[](int index) const {
    assert(index >= 0 && index < 16 && "Matrix4x4: index OOR");
    return m[index];
  }

  /// 裸数组（供 GPU）
  constexpr const float *data() const { return m; }

  // ============================ 运算符 ============================

  /// 矩阵乘
  constexpr Matrix4x4 operator*(const Matrix4x4 &rhs) const {
    Matrix4x4 result{};
    for (int col = 0; col < 4; ++col) {
      for (int row = 0; row < 4; ++row) {
        float sum = 0.0f;
        for (int k = 0; k < 4; ++k) {
          sum += at(row, k) * rhs.at(k, col);
        }
        result.at(row, col) = sum;
      }
    }
    return result;
  }

  /// 变换向量 v' = M * (v, 1)
  constexpr Vector3 operator*(const Vector3 &v) const {
    const float x = m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12];
    const float y = m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13];
    const float z = m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14];
    const float w = m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15];
    // 齐次话
    if (w != 0.0f && w != 1.0f) {
      const float inv = 1.0f / w;
      return Vector3(x * inv, y * inv, z * inv);
    }
    return Vector3(x, y, z);
  }

  constexpr bool operator==(const Matrix4x4 &rhs) const {
    for (int i = 0; i < 16; ++i) {
      if (m[i] != rhs.m[i])
        return false;
    }
    return true;
  }
  constexpr bool operator!=(const Matrix4x4 &rhs) const {
    return !(*this == rhs);
  }

  // ============================ 基础运算 ============================

  /// 转置
  constexpr Matrix4x4 transposed() const {
    Matrix4x4 result;
    for (int col = 0; col < 4; ++col) {
      for (int row = 0; row < 4; ++row) {
        result.at(col, row) = at(row, col);
      }
    }
    return result;
  }

  /// 行列式
  constexpr float determinant() const {
    // 拉普拉斯展开（按第一行）
    float det = 0.0f;
    det += m[0] * (m[5] * (m[10] * m[15] - m[11] * m[14]) -
                   m[6] * (m[9] * m[15] - m[11] * m[13]) +
                   m[7] * (m[9] * m[14] - m[10] * m[13]));
    det -= m[4] * (m[1] * (m[10] * m[15] - m[11] * m[14]) -
                   m[2] * (m[9] * m[15] - m[11] * m[13]) +
                   m[3] * (m[9] * m[14] - m[10] * m[13]));
    det += m[8] * (m[1] * (m[6] * m[15] - m[7] * m[14]) -
                   m[2] * (m[5] * m[15] - m[7] * m[13]) +
                   m[3] * (m[5] * m[14] - m[6] * m[13]));
    det -= m[12] * (m[1] * (m[6] * m[11] - m[7] * m[10]) -
                    m[2] * (m[5] * m[11] - m[7] * m[9]) +
                    m[3] * (m[5] * m[10] - m[6] * m[9]));
    return det;
  }

  /// 逆。不可逆返回单位矩阵
  constexpr Matrix4x4 inverse() const {
    const float det = determinant();
    const float absDet = det < 0.0f ? -det : det;
    if (absDet < EPSILON) {
      return identity();
    }
    const float invDet = 1.0f / det;

    Matrix4x4 result;
    // 伴随矩阵 / det
    result.at(0, 0) = (m[5] * (m[10] * m[15] - m[11] * m[14]) -
                       m[6] * (m[9] * m[15] - m[11] * m[13]) +
                       m[7] * (m[9] * m[14] - m[10] * m[13])) *
                      invDet;
    result.at(1, 0) = -(m[1] * (m[10] * m[15] - m[11] * m[14]) -
                        m[2] * (m[9] * m[15] - m[11] * m[13]) +
                        m[3] * (m[9] * m[14] - m[10] * m[13])) *
                      invDet;
    result.at(2, 0) = (m[1] * (m[6] * m[15] - m[7] * m[14]) -
                       m[2] * (m[5] * m[15] - m[7] * m[13]) +
                       m[3] * (m[5] * m[14] - m[6] * m[13])) *
                      invDet;
    result.at(3, 0) = -(m[1] * (m[6] * m[11] - m[7] * m[10]) -
                        m[2] * (m[5] * m[11] - m[7] * m[9]) +
                        m[3] * (m[5] * m[10] - m[6] * m[9])) *
                      invDet;

    result.at(0, 1) = -(m[4] * (m[10] * m[15] - m[11] * m[14]) -
                        m[6] * (m[8] * m[15] - m[11] * m[12]) +
                        m[7] * (m[8] * m[14] - m[10] * m[12])) *
                      invDet;
    result.at(1, 1) = (m[0] * (m[10] * m[15] - m[11] * m[14]) -
                       m[2] * (m[8] * m[15] - m[11] * m[12]) +
                       m[3] * (m[8] * m[14] - m[10] * m[12])) *
                      invDet;
    result.at(2, 1) = -(m[0] * (m[6] * m[15] - m[7] * m[14]) -
                        m[2] * (m[4] * m[15] - m[7] * m[12]) +
                        m[3] * (m[4] * m[14] - m[6] * m[12])) *
                      invDet;
    result.at(3, 1) = (m[0] * (m[6] * m[11] - m[7] * m[10]) -
                       m[2] * (m[4] * m[11] - m[7] * m[8]) +
                       m[3] * (m[4] * m[10] - m[6] * m[8])) *
                      invDet;

    result.at(0, 2) = (m[4] * (m[9] * m[15] - m[11] * m[13]) -
                       m[5] * (m[8] * m[15] - m[11] * m[12]) +
                       m[7] * (m[8] * m[13] - m[9] * m[12])) *
                      invDet;
    result.at(1, 2) = -(m[0] * (m[9] * m[15] - m[11] * m[13]) -
                        m[1] * (m[8] * m[15] - m[11] * m[12]) +
                        m[3] * (m[8] * m[13] - m[9] * m[12])) *
                      invDet;
    result.at(2, 2) = (m[0] * (m[5] * m[15] - m[7] * m[13]) -
                       m[1] * (m[4] * m[15] - m[7] * m[12]) +
                       m[3] * (m[4] * m[13] - m[5] * m[12])) *
                      invDet;
    result.at(3, 2) = -(m[0] * (m[5] * m[11] - m[7] * m[9]) -
                        m[1] * (m[4] * m[11] - m[7] * m[8]) +
                        m[3] * (m[4] * m[9] - m[5] * m[8])) *
                      invDet;

    result.at(0, 3) = -(m[4] * (m[9] * m[14] - m[10] * m[13]) -
                        m[5] * (m[8] * m[14] - m[10] * m[12]) +
                        m[6] * (m[8] * m[13] - m[9] * m[12])) *
                      invDet;
    result.at(1, 3) = (m[0] * (m[9] * m[14] - m[10] * m[13]) -
                       m[1] * (m[8] * m[14] - m[10] * m[12]) +
                       m[2] * (m[8] * m[13] - m[9] * m[12])) *
                      invDet;
    result.at(2, 3) = -(m[0] * (m[5] * m[14] - m[6] * m[13]) -
                        m[1] * (m[4] * m[14] - m[6] * m[12]) +
                        m[2] * (m[4] * m[13] - m[5] * m[12])) *
                      invDet;
    result.at(3, 3) = (m[0] * (m[5] * m[10] - m[6] * m[9]) -
                       m[1] * (m[4] * m[10] - m[6] * m[8]) +
                       m[2] * (m[4] * m[9] - m[5] * m[8])) *
                      invDet;

    return result;
  }
};

// ============================ 自由函数 ============================

/// 平移矩阵
constexpr Matrix4x4 translation(const Vector3 &t) {
  Matrix4x4 result;
  result.at(0, 3) = t.x;
  result.at(1, 3) = t.y;
  result.at(2, 3) = t.z;
  return result;
}

/// 缩放矩阵（对角）
constexpr Matrix4x4 scale(const Vector3 &s) {
  Matrix4x4 result;
  result.at(0, 0) = s.x;
  result.at(1, 1) = s.y;
  result.at(2, 2) = s.z;
  return result;
}

/// 旋转矩阵（由四元数）
inline Matrix4x4 rotation(const Quaternion &q) {
  const Quaternion n = q.normalized();
  const float x = n.x, y = n.y, z = n.z, w = n.w;
  const float xx = x * x, yy = y * y, zz = z * z;
  const float xy = x * y, xz = x * z, yz = y * z;
  const float wx = w * x, wy = w * y, wz = w * z;

  Matrix4x4 result;
  result.at(0, 0) = 1.0f - 2.0f * (yy + zz);
  result.at(1, 0) = 2.0f * (xy + wz);
  result.at(2, 0) = 2.0f * (xz - wy);
  result.at(0, 1) = 2.0f * (xy - wz);
  result.at(1, 1) = 1.0f - 2.0f * (xx + zz);
  result.at(2, 1) = 2.0f * (yz + wx);
  result.at(0, 2) = 2.0f * (xz + wy);
  result.at(1, 2) = 2.0f * (yz - wx);
  result.at(2, 2) = 1.0f - 2.0f * (xx + yy);
  return result;
}

/// 正交投影矩阵（右手系，[-1,1] 裁剪空间）
constexpr Matrix4x4 orthographic(float left, float right, float bottom,
                                 float top, float near, float far) {
  Matrix4x4 result;
  result.at(0, 0) = 2.0f / (right - left);
  result.at(1, 1) = 2.0f / (top - bottom);
  result.at(2, 2) = -2.0f / (far - near);
  result.at(0, 3) = -(right + left) / (right - left);
  result.at(1, 3) = -(top + bottom) / (top - bottom);
  result.at(2, 3) = -(far + near) / (far - near);
  return result;
}

/// 透视投影矩阵（右手系，fovY 弧度，[-1,1] 深度）
inline Matrix4x4 perspective(float fovY, float aspect, float near, float far) {
  assert(near > 0.0f && far > near && "perspective: invalid near/far");
  const float tanHalfFov = std::tan(fovY * 0.5f);
  Matrix4x4 result;
  result.at(0, 0) = 1.0f / (aspect * tanHalfFov);
  result.at(1, 1) = 1.0f / tanHalfFov;
  result.at(2, 2) = -(far + near) / (far - near);
  result.at(3, 2) = -1.0f;
  result.at(2, 3) = -(2.0f * far * near) / (far - near);
  return result;
}

/**
 * @brief 视图矩阵：lookAt
 * @note eye == target 或 up // forward 时返回单位矩阵
 * @note 右手系
 * @param m_eye 世界坐标系相机位置
 * @param m_target 世界坐标系目标位置
 * @param m_
 */
inline Matrix4x4 lookAt(const Vector3 &eye, const Vector3 &target,
                        const Vector3 &up) {
  const Vector3 f = normalized(target - eye); // forward
  if (lengthSquared(f) < EPSILON * EPSILON) {
    return Matrix4x4::identity();
  }

  Vector3 s = cross(f, up); // right
  if (lengthSquared(s) < EPSILON * EPSILON) {
    // up ∥ forward up 重算
    const Vector3 ref =
        (std::abs(f.y) < 1.0f - EPSILON) ? Vector3::up() : Vector3::right();
    s = cross(f, ref);
  }
  s = normalized(s);
  const Vector3 u = cross(s, f); // true up

  Matrix4x4 result;
  // 旋转部分（s, u, -f 作为行）列主序写入
  result.at(0, 0) = s.x;
  result.at(0, 1) = s.y;
  result.at(0, 2) = s.z;
  result.at(1, 0) = u.x;
  result.at(1, 1) = u.y;
  result.at(1, 2) = u.z;
  result.at(2, 0) = -f.x;
  result.at(2, 1) = -f.y;
  result.at(2, 2) = -f.z;
  // 平移部分（-R * eye）
  result.at(0, 3) = -dot(s, eye);
  result.at(1, 3) = -dot(u, eye);
  result.at(2, 3) = dot(f, eye);
  return result;
}

} // namespace lCYC::math
