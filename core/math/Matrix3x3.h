#pragma once
/**
 * @brief 三阶矩阵
 * @namespace lCYC::math
 * @note 存储约定: 列主序 column-major，m[col * 3 + row]
 */

#include "Math.h"
#include "Matrix4x4.h"
#include "Quaternion.h"
#include "Vector2.h"
#include "Vector3.h"
#include <cassert>
#include <cmath>

namespace lCYC::math {

struct Matrix3x3 {
  float m[9];

  // ============================ 构造 ============================

  /// 默认单位矩阵
  constexpr Matrix3x3() : m{1, 0, 0, 0, 1, 0, 0, 0, 1} {}

  constexpr explicit Matrix3x3(const float values[9]) {
    for (int i = 0; i < 9; ++i) {
      m[i] = values[i];
    }
  }

  // ============================ 静态常量 ============================

  static constexpr Matrix3x3 identity() { return Matrix3x3(); }

  // ============================ 索引访问 ============================
  // 列主序: 元素 (row, col) → m[col * 3 + row]

  constexpr float &at(int row, int col) {
    assert(row >= 0 && row < 3 && col >= 0 && col < 3 &&
           "Matrix3x3: index OOR");
    return m[col * 3 + row];
  }
  constexpr const float &at(int row, int col) const {
    assert(row >= 0 && row < 3 && col >= 0 && col < 3 &&
           "Matrix3x3: index OOR");
    return m[col * 3 + row];
  }

  constexpr float &operator[](int index) {
    assert(index >= 0 && index < 9 && "Matrix3x3: index OOR");
    return m[index];
  }
  constexpr const float &operator[](int index) const {
    assert(index >= 0 && index < 9 && "Matrix3x3: index OOR");
    return m[index];
  }

  constexpr const float *data() const { return m; }

  // ============================ 运算符 ============================

  constexpr Matrix3x3 operator*(const Matrix3x3 &rhs) const {
    Matrix3x3 result;
    for (int col = 0; col < 3; ++col) {
      for (int row = 0; row < 3; ++row) {
        float sum = 0.0f;
        for (int k = 0; k < 3; ++k) {
          sum += at(row, k) * rhs.at(k, col);
        }
        result.at(row, col) = sum;
      }
    }
    return result;
  }

  constexpr Vector3 operator*(const Vector3 &v) const {
    return Vector3(m[0] * v.x + m[3] * v.y + m[6] * v.z,
                   m[1] * v.x + m[4] * v.y + m[7] * v.z,
                   m[2] * v.x + m[5] * v.y + m[8] * v.z);
  }

  constexpr bool operator==(const Matrix3x3 &rhs) const {
    for (int i = 0; i < 9; ++i) {
      if (m[i] != rhs.m[i])
        return false;
    }
    return true;
  }
  constexpr bool operator!=(const Matrix3x3 &rhs) const {
    return !(*this == rhs);
  }

  // ============================ 基础运算 ============================

  /// 转置
  constexpr Matrix3x3 transposed() const {
    Matrix3x3 result;
    for (int col = 0; col < 3; ++col) {
      for (int row = 0; row < 3; ++row) {
        result.at(col, row) = at(row, col);
      }
    }
    return result;
  }

  /// 行列式
  constexpr float determinant() const {
    return m[0] * (m[4] * m[8] - m[5] * m[7]) -
           m[3] * (m[1] * m[8] - m[2] * m[7]) +
           m[6] * (m[1] * m[5] - m[2] * m[4]);
  }

  /// 逆。不可逆（det≈0）防御：返回单位矩阵
  Matrix3x3 inverse() const {
    const float det = determinant();
    if (std::abs(det) < EPSILON) {
      return identity();
    }
    const float invDet = 1.0f / det;
    Matrix3x3 result;
    result.at(0, 0) = (m[4] * m[8] - m[5] * m[7]) * invDet;
    result.at(1, 0) = -(m[1] * m[8] - m[2] * m[7]) * invDet;
    result.at(2, 0) = (m[1] * m[5] - m[2] * m[4]) * invDet;
    result.at(0, 1) = -(m[3] * m[8] - m[5] * m[6]) * invDet;
    result.at(1, 1) = (m[0] * m[8] - m[2] * m[6]) * invDet;
    result.at(2, 1) = -(m[0] * m[5] - m[2] * m[3]) * invDet;
    result.at(0, 2) = (m[3] * m[7] - m[4] * m[6]) * invDet;
    result.at(1, 2) = -(m[0] * m[7] - m[1] * m[6]) * invDet;
    result.at(2, 2) = (m[0] * m[4] - m[1] * m[3]) * invDet;
    return result;
  }

  /// 法线矩阵（= 逆转置，用于非均匀缩放下的法线变换）
  Matrix3x3 normalMatrix() const { return inverse().transposed(); }

  // ============================ 转换 ============================

  /// 从 4x4 提取 3x3（去平移，取左上 3x3）
  constexpr static Matrix3x3 fromMatrix4x4(const Matrix4x4 &m4) {
    Matrix3x3 result;
    result.at(0, 0) = m4.at(0, 0);
    result.at(1, 0) = m4.at(1, 0);
    result.at(2, 0) = m4.at(2, 0);
    result.at(0, 1) = m4.at(0, 1);
    result.at(1, 1) = m4.at(1, 1);
    result.at(2, 1) = m4.at(2, 1);
    result.at(0, 2) = m4.at(0, 2);
    result.at(1, 2) = m4.at(1, 2);
    result.at(2, 2) = m4.at(2, 2);
    return result;
  }

  /// 扩展为 4x4（右下角 1，无平移）
  constexpr Matrix4x4 toMatrix4x4() const {
    Matrix4x4 result;
    result.at(0, 0) = at(0, 0);
    result.at(1, 0) = at(1, 0);
    result.at(2, 0) = at(2, 0);
    result.at(0, 1) = at(0, 1);
    result.at(1, 1) = at(1, 1);
    result.at(2, 1) = at(2, 1);
    result.at(0, 2) = at(0, 2);
    result.at(1, 2) = at(1, 2);
    result.at(2, 2) = at(2, 2);
    return result;
  }
};

// ============================ 自由函数 ============================

/**
 * @brief 创建绕x轴的 旋转矩阵
 *
 * @param angleRad 从y正半轴逆时针旋转弧度
 * @return constexpr Matrix3x3
 * @details 在 xz 平面旋转
 */
constexpr Matrix3x3 rotationX(float angleRad) {
  Matrix3x3 result;
  const float c = std::cos(angleRad);
  const float s = std::sin(angleRad);
  result.at(1, 1) = c;
  result.at(2, 1) = s;  // 列1 (up) 的 z 分量 = sinθ
  result.at(1, 2) = -s; // 列2 (forward) 的 y 分量 = -sinθ
  result.at(2, 2) = c;
  return result;
}

/**
 * @brief 创建绕y轴的 旋转矩阵
 *
 * @param angleRad 从x正半轴逆时针旋转弧度
 * @return constexpr Matrix3x3
 * @details 在 xz 平面旋转
 */
constexpr Matrix3x3 rotationY(float angleRad) {
  Matrix3x3 result;
  const float c = std::cos(angleRad);
  const float s = std::sin(angleRad);
  result.at(0, 0) = c;
  result.at(2, 0) = -s; // 列0 (right) 的 z 分量 = -sinθ
  result.at(0, 2) = s;  // 列2 (forward) 的 x 分量 = sinθ
  result.at(2, 2) = c;
  return result;
}

/**
 * @brief 创建绕z轴的 旋转矩阵
 *
 * @param angleRad 从x正半轴逆时针旋转弧度
 * @return constexpr Matrix3x3
 * @details 在 xy 平面旋转（标准逆时针，右手系，行列式 +1）
 *  原向量 (x,y) = (Lcosα, Lsinα)，逆时针旋转 θ 后：
 *  x' = L·cos(α+θ) = L(cosα·cosθ − sinα·sinθ) = x·cosθ − y·sinθ
 *  y' = L·sin(α+θ) = L(sinα·cosθ + cosα·sinθ) = x·sinθ + y·cosθ
 *  矩阵（列向量约定 v' = M·v）：
 *  [ cosθ  −sinθ ]
 *  [ sinθ   cosθ ]
 */
constexpr Matrix3x3 rotationZ(float angleRad) {
  Matrix3x3 result;
  const float c = std::cos(angleRad);
  const float s = std::sin(angleRad);
  result.at(0, 0) = c;
  result.at(1, 0) = s;  // 列0 (right) 的 y 分量 = sinθ
  result.at(0, 1) = -s; // 列1 (up) 的 x 分量 = -sinθ
  result.at(1, 1) = c;
  return result;
}

/// 由四元数构造 3x3 旋转矩阵
inline Matrix3x3 rotation3x3(const Quaternion &q) {
  return Matrix3x3::fromMatrix4x4(lCYC::math::rotation(q));
}

/// 3x3 缩放矩阵，三维向量规格化到三维矩阵
constexpr Matrix3x3 scale3x3(const Vector3 &s) {
  Matrix3x3 result;
  result.at(0, 0) = s.x;
  result.at(1, 1) = s.y;
  result.at(2, 2) = s.z;
  return result;
}
/**
 * @brief 创建2D旋转矩阵
 *
 * @param angleRad 从x正半轴逆时针旋转角度
 * @return constexpr Matrix3x3
 * @details 2D 平面旋转 = 3D 绕 Z 轴旋转限制在 xy 平面（w 分量不变），
 *  直接复用 rotationZ。推导见 rotationZ：
 *  x' = x·cosθ − y·sinθ，y' = x·sinθ + y·cosθ（行列式 +1，非镜像）
 */
constexpr Matrix3x3 rotation2D(float angleRad) { return rotationZ(angleRad); }

/**
 * @brief 创建2D平移矩阵
 *
 * @param x x方向平移浮点量
 * @param y y方向平移浮点量
 * @return constexpr Matrix3x3
 */
constexpr Matrix3x3 translation2D(float x, float y) {
  Matrix3x3 result;
  result.at(0, 2) = x;
  result.at(1, 2) = y;
  return result;
}

} // namespace lCYC::math
