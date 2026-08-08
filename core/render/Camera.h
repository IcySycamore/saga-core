#pragma once
/**
 * @brief 相机
 * @namespace lCYC::render
 * @note 职责:
 *   - 位置/朝向（lookAt）→ 视图矩阵 V
 *   - 投影类型（透视/正交）→ 投影矩阵 P
 *   - MVP = P * V * M 的 V 和 P 部分
 * @note C++20
 */

#include "core/math/Math.h"
#include "core/math/Matrix4x4.h"
#include "core/math/Vector3.h"

namespace lCYC::render {
enum class Proj { Perspective, Orthographic };
class Camera {
private:
  Proj m_proj = Proj::Perspective;
  lCYC::math::Vector3 m_eye{0.0f, 0.0f, 5.0f};
  lCYC::math::Vector3 m_target{0.0f, 0.0f, 0.0f};
  lCYC::math::Vector3 m_up{0.0f, 1.0f, 0.0f};
  float m_fovY = 60.0f * lCYC::math::PI / 180.0f;
  float m_near = 0.1f;
  float m_far = 100.0f;
  float m_l = -1.0f, m_r = 1.0f, m_b = -1.0f, m_t = 1.0f;

public:
  /**
   * @brief 设置透视投影
   * @param fovY 垂直视场角，弧度范围(0, π)
   * @param near_ 近裁剪面距离 > 0
   * @param far_ 远裁剪面距离 > near_
   */
  void setPerspective(float fovY, float near_, float far_) {
    m_proj = Proj::Perspective;
    m_fovY = fovY;
    m_near = near_;
    m_far = far_;
  }

  /**
   * @brief 设置正交投影
   * @param l 左边界
   * @param r 右边界 > l
   * @param b 下边界
   * @param t 上边界 > b
   * @param near_ 近裁剪面距离 > 0
   * @param far_ 远裁剪面距离 > near_
   */
  void setOrthographic(float l, float r, float b, float t, float near_,
                       float far_) {
    m_proj = Proj::Orthographic;
    m_l = l;
    m_r = r;
    m_b = b;
    m_t = t;
    m_near = near_;
    m_far = far_;
  }
  /**
   * @brief 设置相机位置与朝向
   * @param eye 相机位置
   * @param target 观察目标点
   * @param up 头顶方向，用于摆正画面（默认世界向上）
   */
  void lookAt(const lCYC::math::Vector3 &eye, const lCYC::math::Vector3 &target,
              const lCYC::math::Vector3 &up = lCYC::math::Vector3::up()) {
    m_eye = eye;
    m_target = target;
    m_up = up;
  }
  /**
   * @brief 生成视图矩阵 V
   * @return 4×4 视图矩阵
   */
  lCYC::math::Matrix4x4 view() const { return lCYC::math::lookAt(m_eye, m_target, m_up); }

  /**
   * @brief 生成投影矩阵 P
   * @param aspect 宽高比（width/height）
   * @return 4×4 投影矩阵 按投影类型
   */
  lCYC::math::Matrix4x4 projection(float aspect) const {
    if (m_proj == Proj::Perspective) {
      return lCYC::math::perspective(m_fovY, aspect, m_near, m_far);
    }
    return lCYC::math::orthographic(m_l, m_r, m_b, m_t, m_near, m_far);
  }

  /**
   * @brief 生成视图投影矩阵 VP = P * V
   * @param aspect 宽高比（width/height）
   * @return 4×4 视图投影矩阵
   */
  lCYC::math::Matrix4x4 viewProjection(float aspect) const {
    return projection(aspect) * view();
  }

  /** @brief 当前投影类型 */
  Proj projectionType() const { return m_proj; }
  /** @brief 相机位置 */
  const lCYC::math::Vector3 &eye() const { return m_eye; }
  /** @brief 目标点 */
  const lCYC::math::Vector3 &target() const { return m_target; }
};

} // namespace lCYC::render
