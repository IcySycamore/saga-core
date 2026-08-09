#pragma once
/**
 * @brief 组合工具
 * @namespace lCYC::math
 * @note 应用层入口：供 ECS Transform、Camera、场景图消费
 * @note 欧拉顺序约定: 应用X 再 Y 再 Z
 */

#include "Math.h"
#include "Matrix3x3.h"
#include "Matrix4x4.h"
#include "Quaternion.h"
#include "Vector3.h"
#include <cassert>
#include <cmath>

namespace lCYC::math {

// ============================ TRS 合成 ============================

/**
 * @brief TRS 合成：M = T * R * S（先缩放再旋转再平移）
 * @param pos 平移量
 * @param rot 旋转（四元数）
 * @param scale 缩放（默认全 1 = 无缩放）
 * @return 4x4 模型矩阵
 * @note 列向量约定：v' = M * v = T(R(S(v)))
 */
inline Matrix4x4 trs(const Vector3 &pos, const Quaternion &rot,
                     const Vector3 &scale = Vector3::one()) {
  Matrix4x4 result = lCYC::math::rotation(rot); // 旋转
  // 应用缩放到旋转矩阵的列
  result.at(0, 0) *= scale.x;
  result.at(1, 0) *= scale.x;
  result.at(2, 0) *= scale.x;
  result.at(0, 1) *= scale.y;
  result.at(1, 1) *= scale.y;
  result.at(2, 1) *= scale.y;
  result.at(0, 2) *= scale.z;
  result.at(1, 2) *= scale.z;
  result.at(2, 2) *= scale.z;
  // 平移
  result.at(0, 3) = pos.x;
  result.at(1, 3) = pos.y;
  result.at(2, 3) = pos.z;
  return result;
}


/**
 * @brief 分解 TRS：从矩阵还原 pos/rot/scale
 * @param m 输入矩阵（须来自 trs 合成）
 * @param pos 输出平移
 * @param rot 输出旋转
 * @param scale 输出缩放
 * @return false 若矩阵奇异（缩放为 0 或退化）
 * @note 分解在非均匀缩放下有歧义（旋转与缩放耦合），调用方需保证来源是 TRS
 */
inline bool decomposeTRS(const Matrix4x4 &m, Vector3 &pos, Quaternion &rot,
                         Vector3 &scale) {
  // 平移 = 最后一列
  pos = Vector3(m.at(0, 3), m.at(1, 3), m.at(2, 3));

  // 缩放 = 旋转矩阵各列的长度（从列向量提取）
  const Vector3 col0(m.at(0, 0), m.at(1, 0), m.at(2, 0));
  const Vector3 col1(m.at(0, 1), m.at(1, 1), m.at(2, 1));
  const Vector3 col2(m.at(0, 2), m.at(1, 2), m.at(2, 2));

  scale.x = length(col0);
  scale.y = length(col1);
  scale.z = length(col2);

  if (scale.x < EPSILON || scale.y < EPSILON || scale.z < EPSILON) {
    return false; // 退化缩放，无法还原旋转
  }

  // 旋转 = 归一化后的列组成的 3x3
  Matrix3x3 rotMat;
  rotMat.at(0, 0) = col0.x / scale.x;
  rotMat.at(1, 0) = col0.y / scale.x;
  rotMat.at(2, 0) = col0.z / scale.x;
  rotMat.at(0, 1) = col1.x / scale.y;
  rotMat.at(1, 1) = col1.y / scale.y;
  rotMat.at(2, 1) = col1.z / scale.y;
  rotMat.at(0, 2) = col2.x / scale.z;
  rotMat.at(1, 2) = col2.y / scale.z;
  rotMat.at(2, 2) = col2.z / scale.z;

  // 3x3 → 四元数（Shepperd 算法）
  const float tr = rotMat.at(0, 0) + rotMat.at(1, 1) + rotMat.at(2, 2);
  if (tr > 0.0f) {
    const float s = std::sqrt(tr + 1.0f) * 2.0f;
    rot = Quaternion((rotMat.at(2, 1) - rotMat.at(1, 2)) / s,
                     (rotMat.at(0, 2) - rotMat.at(2, 0)) / s,
                     (rotMat.at(1, 0) - rotMat.at(0, 1)) / s, 0.25f * s);
  } else if (rotMat.at(0, 0) > rotMat.at(1, 1) &&
             rotMat.at(0, 0) > rotMat.at(2, 2)) {
    const float s =
        std::sqrt(1.0f + rotMat.at(0, 0) - rotMat.at(1, 1) - rotMat.at(2, 2)) *
        2.0f;
    rot = Quaternion(0.25f * s, (rotMat.at(0, 1) + rotMat.at(1, 0)) / s,
                     (rotMat.at(0, 2) + rotMat.at(2, 0)) / s,
                     (rotMat.at(2, 1) - rotMat.at(1, 2)) / s);
  } else if (rotMat.at(1, 1) > rotMat.at(2, 2)) {
    const float s =
        std::sqrt(1.0f + rotMat.at(1, 1) - rotMat.at(0, 0) - rotMat.at(2, 2)) *
        2.0f;
    rot = Quaternion((rotMat.at(0, 1) + rotMat.at(1, 0)) / s, 0.25f * s,
                     (rotMat.at(1, 2) + rotMat.at(2, 1)) / s,
                     (rotMat.at(0, 2) - rotMat.at(2, 0)) / s);
  } else {
    const float s =
        std::sqrt(1.0f + rotMat.at(2, 2) - rotMat.at(0, 0) - rotMat.at(1, 1)) *
        2.0f;
    rot = Quaternion((rotMat.at(0, 2) + rotMat.at(2, 0)) / s,
                     (rotMat.at(1, 2) + rotMat.at(2, 1)) / s, 0.25f * s,
                     (rotMat.at(1, 0) - rotMat.at(0, 1)) / s);
  }
  rot = rot.normalized();
  return true;
}

// ============================ 矩阵互转 ============================
// 3x3/4x4/四元数之间的转换，从 Matrix3x3 迁入（组合层职责）

/**
 * @brief 法线矩阵（逆转置，用于非均匀缩放下的法线变换）
 * @param m 变换矩阵（仅取左上 3x3 的旋转/缩放部分）
 * @return 3x3 法线变换矩阵
 */
inline Matrix3x3 normalMatrix(const Matrix3x3 &m) {
  return m.inverse().transposed();
}

/**
 * @brief 从 4x4 提取左上 3x3（去平移）
 * @param m4 输入 4x4 矩阵
 * @return 提取的 3x3 矩阵
 */
constexpr Matrix3x3 toMatrix3x3(const Matrix4x4 &m4) {
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

/**
 * @brief 3x3 扩展为 4x4（右下角 1，无平移）
 * @param m 输入 3x3 矩阵
 * @return 扩展后的 4x4 矩阵
 */
constexpr Matrix4x4 toMatrix4x4(const Matrix3x3 &m) {
  Matrix4x4 result;
  result.at(0, 0) = m.at(0, 0);
  result.at(1, 0) = m.at(1, 0);
  result.at(2, 0) = m.at(2, 0);
  result.at(0, 1) = m.at(0, 1);
  result.at(1, 1) = m.at(1, 1);
  result.at(2, 1) = m.at(2, 1);
  result.at(0, 2) = m.at(0, 2);
  result.at(1, 2) = m.at(1, 2);
  result.at(2, 2) = m.at(2, 2);
  return result;
}

/**
 * @brief 四元数 → 3x3 旋转矩阵
 * @param q 单位四元数（旋转）
 * @return 3x3 旋转矩阵
 */
inline Matrix3x3 rotation3x3(const Quaternion &q) {
  return toMatrix3x3(lCYC::math::rotation(q));
}

// ============================ 欧拉角互转 ============================

/**
 * @brief 四元数 → 欧拉弧度角
 * @param q 四元数（会自动归一化）
 * @return 欧拉角 (x=pitch, y=yaw, z=roll)，弧度
 * @note 匹配 euler 的参数布局（euler(pitch, yaw, roll) = qz·qy·qx）
 *   yaw  = asin(2(wy − xz))   [y 分量编码 yaw]
 *   pitch = atan2(2(wx + yz), 1 − 2(x² + y²))  [x 分量编码 pitch]
 *   roll  = atan2(2(wz + xy), 1 − 2(y² + z²))
 * @note 奇点：yaw = ±90° 时 pitch/roll 不可唯一分解，输出取一种约定
 */
inline Vector3 quatToEuler(const Quaternion &q) {
  const Quaternion n = q.normalized();
  const float x = n.x, y = n.y, z = n.z, w = n.w;

  const float sinYaw = 2.0f * (w * y - x * z); // = sin(yaw)
  float pitch = 0.0f, yaw = 0.0f, roll = 0.0f;

  if (std::abs(sinYaw) >= 1.0f - 1e-6f) {
    // 万向锁：yaw = ±90°，pitch 与 roll 合并
    yaw = std::copysign(lCYC::math::HALF_PI, sinYaw);
    pitch = std::atan2(y, w);
    roll = 0.0f;
  } else {
    yaw = std::asin(sinYaw);
    pitch = std::atan2(2.0f * (w * x + y * z), 1.0f - 2.0f * (x * x + y * y));
    roll = std::atan2(2.0f * (w * z + x * y), 1.0f - 2.0f * (y * y + z * z));
  }
  return Vector3(pitch, yaw, roll);
}

/**
 * @brief 欧拉角 → 旋转矩阵（4x4，无平移）
 * @param eulerAngles 欧拉角 (x=pitch, y=yaw, z=roll)，弧度
 * @return 4x4 旋转矩阵（左上 3x3 为旋转）
 */
inline Matrix4x4 eulerToMatrix(const Vector3 &eulerAngles) {
  return rotation(euler(eulerAngles.x, eulerAngles.y, eulerAngles.z));
}

/**
 * @brief 从 4x4 提取欧拉角（经四元数中转）
 * @param m 输入 4x4 矩阵
 * @return 欧拉角 (x=pitch, y=yaw, z=roll)，弧度
 */
inline Vector3 matrixToEuler(const Matrix4x4 &m) {
  // 3x3 → 四元数（复用 decompose 的 Shepperd 逻辑）
  Vector3 pos, scale;
  Quaternion rot;
  decomposeTRS(m, pos, rot, scale);
  return quatToEuler(rot);
}

// ============================ 变换 ============================

/**
 * @brief 变换方向（不带平移）：只用左上 3x3
 * @param m 4x4 变换矩阵
 * @param dir 方向向量
 * @return 变换后的方向
 * @note 变换点用 Matrix4x4::operator*(Vector3)（齐次 w=1，带平移）
 */
inline Vector3 transformDirection(const Matrix4x4 &m, const Vector3 &dir) {
  return Vector3(m.at(0, 0) * dir.x + m.at(0, 1) * dir.y + m.at(0, 2) * dir.z,
                 m.at(1, 0) * dir.x + m.at(1, 1) * dir.y + m.at(1, 2) * dir.z,
                 m.at(2, 0) * dir.x + m.at(2, 1) * dir.y + m.at(2, 2) * dir.z);
}

} // namespace lCYC::math
