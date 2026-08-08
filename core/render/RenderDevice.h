#pragma once
/**
 * @brief 图形后端抽象接口（RenderDevice：底层设备抽象，对应 Godot
 * RenderingDevice / Unreal RHI）
 * @namespace render
 * @note 原则:
 *   - 纯接口：不包含任何 SDL3/OpenGL/平台头
 *   - 性能：mesh 上传一次，绘制引用句柄
 *   - 所有权：device 由外部拥有，本接口不管理生命周期
 *   - 安全：span/句柄
 *   - 编译：C++20
 */

#include "core/math/Matrix4x4.h"
#include "core/math/Vector3.h"
#include <cstdint>
#include <span>

namespace render {

// ============================ 句柄 ============================

/**
 * @brief 窗口句柄
 * @usage NativeWindowHandle<SDL_Window>
 *        NativeWindowHandle<HWND>
 *        类型擦除 NativeWindowHandle<void>
 * @brief 提供bool() native <NativeT*>
 */
template <typename NativeT> struct NativeWindowHandle {
  NativeT *native = nullptr;
  explicit operator bool() const { return native != nullptr; }
  operator NativeWindowHandle<void>() const {
    NativeWindowHandle<void> erased;
    erased.native = native;
    return erased;
  }
};

/// 网格 资源句柄
struct MeshHandle {
  uint32_t value = kInvalid; /// 后端内部资源索引
  static constexpr uint32_t kInvalid = ~0u;

  explicit operator bool() const { return value != kInvalid; }
  friend bool operator==(MeshHandle a, MeshHandle b) = default;
};

/// 顶点：位置 + 颜色（未来可扩展法线/UV 字段）
struct Vertex {
  math::Vector3 pos;   ///< 模型空间位置
  math::Vector3 color; ///< RGB（0..1）
};

/// 渲染设备抽象，句柄应当在外部构造
class RenderDevice {
public:
  /** @brief 析构函数（虚，支持多态释放） */
  virtual ~RenderDevice() = default;

  /**
   * @brief 初始化设备（绑定平台窗口）
   * @param win 平台窗口句柄（外部创建，设备不拥有）
   * @param width 逻辑宽度（像素）
   * @param height 逻辑高度（像素）
   * @return true 成功；false 失败
   */
  virtual bool init(const NativeWindowHandle<void> &win, int width,
                    int height) = 0;

  /**
   * @brief 释放设备资源
   * @note 不销毁外部窗口（窗口由应用层拥有）
   */
  virtual void shutdown() = 0;

  /**
   * @brief 清屏 + 清深度，开始新一帧
   */
  virtual void beginFrame() = 0;

  /**
   * @brief 交换缓冲，把画好的内容显示到窗口
   */
  virtual void endFrame() = 0;

  /**
   * @brief 上传顶点数据到设备，返回资源句柄
   * @param verts 顶点数组（借用视图，设备内部复制/上传）
   * @return 资源句柄；失败返回无效句柄（operator bool == false）
   */
  virtual MeshHandle createMesh(std::span<const Vertex> verts) = 0;

  /**
   * @brief 释放网格资源
   * @param h 要释放的句柄（无效句柄安全 no-op）
   */
  virtual void destroyMesh(MeshHandle h) = 0;

  /**
   * @brief 绘制网格（线框模式）
   * @param h 网格句柄
   * @param mvp 模型视图投影矩阵 = P*V*M（列主序）
   */
  virtual void drawMesh(MeshHandle h, const math::Matrix4x4 &mvp) = 0;

  /**
   * @brief 绘制地面网格线（XZ 平面）
   * @param size 网格总尺寸（世界单位）
   * @param divs 网格分割数（每边格数）
   * @param vp 视图投影矩阵 = V*P
   */
  virtual void drawGrid(float size, int divs, const math::Matrix4x4 &vp) = 0;
};

} // namespace render
