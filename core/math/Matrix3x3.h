#pragma once
/**
 * @brief 三阶矩阵
 * @namespace lCYC::math
 * @note 存储约定: 列主序 column-major，m[col * 3 + row]
 */

#include "Math.h"
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

  // ============================ 操作 ============================

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

  /// 逆。不可逆返回单位矩阵
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
};

/// 三维向量规格化到三维矩阵
constexpr Matrix3x3 scale3x3(const Vector3 &s) {
  Matrix3x3 result;
  result.at(0, 0) = s.x;
  result.at(1, 1) = s.y;
  result.at(2, 2) = s.z;
  return result;
}

// ============================ 矩阵创建 ============================

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
  result.at(2, 1) = s;
  result.at(1, 2) = -s;
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
  result.at(2, 0) = -s;
  result.at(0, 2) = s;
  result.at(2, 2) = c;
  return result;
}

/**
 * @brief 创建绕z轴的 旋转矩阵
 *
 * @param angleRad 从x正半轴逆时针旋转弧度
 * @return constexpr Matrix3x3
 * @details 在 xy 平面旋转 右手系
 *  原向量 (x,y) = (Lcosα, Lsinα)，逆时针旋转 θ 后：
 *  x' = L·cos(α+θ) = L(cosα·cosθ − sinα·sinθ) = x·cosθ − y·sinθ
 *  y' = L·sin(α+θ) = L(sinα·cosθ + cosα·sinθ) = x·sinθ + y·cosθ
 *  矩阵：
 *  [ cosθ  −sinθ ]
 *  [ sinθ   cosθ ]
 */
constexpr Matrix3x3 rotationZ(float angleRad) {
  Matrix3x3 result;
  const float c = std::cos(angleRad);
  const float s = std::sin(angleRad);
  result.at(0, 0) = c;
  result.at(1, 0) = s;
  result.at(0, 1) = -s;
  result.at(1, 1) = c;
  return result;
}

/**
 * @brief 创建2D 旋转矩阵
 *
 * @param angleRad 从x正半轴逆时针旋转角度
 * @return constexpr Matrix3x3
 * @details 2D 平面旋转 直接复用 rotationZ。推导见 rotationZ
 */
constexpr Matrix3x3 rotation2D(float angleRad) { return rotationZ(angleRad); }

/**
 * @brief 创建2D 平移矩阵
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
