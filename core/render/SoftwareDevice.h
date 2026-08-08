#pragma once
/**
 * @brief 软件光栅化后端（RenderDevice 的 CPU 实现）
 * @namespace render
 * @note 职责:
 *   - 纯 CPU 光栅化：顶点变换 → 屏幕坐标 → 逐像素填帧缓冲
 *   - 深度缓冲：逐像素深度测试（第 5 讲 z-buffer）
 *   - 三角形光栅化：重心坐标 + 插值（第 6 讲）
 *   - 画线：Bresenham + 深度插值
 * @note 设计:
 *   - 纯引擎层：不依赖任何 SDL/OpenGL/平台 API（可单测、确定性）
 *   - 帧缓冲可被外部读取（framebuffer()），由显示层/测试消费
 *   - init 忽略窗口句柄（纯 CPU 无窗口），endFrame 无 present（显示在外部）
 * @note 像素格式：uint32_t RGBA8888（0xRRGGBBAA）
 * @note C++20
 */

#include "core/math/Math.h"
#include "core/render/RenderDevice.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <span>
#include <vector>

namespace render {

class SoftwareDevice : public RenderDevice {
public:
  // ============================ 生命周期 ============================

  /**
   * @brief 初始化软光栅（纯 CPU，无需窗口）
   * @param win 窗口句柄（忽略，软件后端不绑定窗口）
   * @param width 帧缓冲宽度（像素）
   * @param height 帧缓冲高度（像素）
   * @return true 成功
   */
  bool init(const NativeWindowHandle<void> &win, int width,
            int height) override {
    (void)win;
    if (width <= 0 || height <= 0) {
      return false;
    }
    m_width = width;
    m_height = height;
    m_framebuffer.assign(static_cast<size_t>(width) * height, 0xFF000000u);
    m_depthBuffer.assign(static_cast<size_t>(width) * height, 1.0f);
    return true;
  }

  /** @brief 释放资源 */
  void shutdown() override {
    m_framebuffer.clear();
    m_depthBuffer.clear();
    m_meshes.clear();
  }

  // ============================ 帧控制 ============================

  /** @brief 清帧缓冲（深灰蓝背景）+ 清深度缓冲（无限远=1.0） */
  void beginFrame() override {
    // RGBA 内存序（小端打包）：内存 = R,G,B,A = 0x18,0x20,0x30,0xFF
    std::fill(m_framebuffer.begin(), m_framebuffer.end(), 0xFF302018u);
    std::fill(m_depthBuffer.begin(), m_depthBuffer.end(), 1.0f);
  }

  /** @brief 帧完成（无平台 present，显示由外部读 framebuffer()） */
  void endFrame() override {}

  // ============================ 资源 ============================

  /**
   * @brief 上传顶点数据（复制到内部缓存）
   * @param verts 顶点数组
   * @return 句柄
   */
  MeshHandle createMesh(std::span<const Vertex> verts) override {
    m_meshes.push_back(std::vector<Vertex>(verts.begin(), verts.end()));
    MeshHandle h;
    h.value = static_cast<uint32_t>(m_meshes.size() - 1);
    return h;
  }

  /**
   * @brief 释放网格资源
   * @param h 句柄（无效句柄安全 no-op）
   */
  void destroyMesh(MeshHandle h) override {
    if (h.value < m_meshes.size()) {
      m_meshes[h.value].clear();
    }
  }

  // ============================ 绘制 ============================

  /**
   * @brief 绘制网格：每 3 个顶点 = 一个实心三角形；剩余 2 个 = 一条线
   * @param h 网格句柄
   * @param mvp 模型视图投影矩阵 = P*V*M
   * @note 近平面裁剪：含相机后方（w≤0）顶点的图元丢弃，避免 NDC 翻转爆炸
   */
  void drawMesh(MeshHandle h, const math::Matrix4x4 &mvp) override {
    if (h.value >= m_meshes.size() || m_meshes[h.value].empty()) {
      return;
    }
    const auto &verts = m_meshes[h.value];
    const size_t n = verts.size();
    // 三角形：每 3 个顶点
    size_t i = 0;
    for (; i + 2 < n; i += 3) {
      const ScreenVert a = transformVertex(verts[i], mvp);
      const ScreenVert b = transformVertex(verts[i + 1], mvp);
      const ScreenVert c = transformVertex(verts[i + 2], mvp);
      if (a.invalid || b.invalid || c.invalid) {
        continue; // 有顶点在相机后方 → 跳过（简化裁剪）
      }
      rasterizeTriangle(a, b, c);
    }
    // 剩余一对：画线
    for (; i + 1 < n; i += 2) {
      const ScreenVert a = transformVertex(verts[i], mvp);
      const ScreenVert b = transformVertex(verts[i + 1], mvp);
      if (a.invalid || b.invalid) {
        continue;
      }
      rasterizeLine(a, b);
    }
  }

  /**
   * @brief 绘制地面网格线（XZ 平面 y=0）
   * @param size 总尺寸（世界单位）
   * @param divs 每边格数
   * @param vp 视图投影矩阵 V*P
   */
  void drawGrid(float size, int divs, const math::Matrix4x4 &vp) override {
    const float half = size * 0.5f;
    const float step = size / static_cast<float>(divs);
    const Vertex color{{0.65f, 0.75f, 0.85f}}; // 亮灰蓝（可见）
    for (int i = 0; i <= divs; ++i) {
      const float coord = -half + step * static_cast<float>(i);
      const ScreenVert a =
          transformVertex(Vertex{{coord, 0.0f, -half}, color.color}, vp);
      const ScreenVert b =
          transformVertex(Vertex{{coord, 0.0f, half}, color.color}, vp);
      rasterizeLine(a, b);
      const ScreenVert c =
          transformVertex(Vertex{{-half, 0.0f, coord}, color.color}, vp);
      const ScreenVert d =
          transformVertex(Vertex{{half, 0.0f, coord}, color.color}, vp);
      rasterizeLine(c, d);
    }
  }

  // ============================ 帧缓冲访问（供显示层/测试）
  // ============================

  /** @brief 帧缓冲只读访问（RGBA8888，行长 = width） */
  std::span<const uint32_t> framebuffer() const { return m_framebuffer; }

  /**
   * @brief 读取单个像素（供测试断言）
   * @param x 列
   * @param y 行（0 = 顶部）
   * @return RGBA8888；越界返回 0
   */
  uint32_t pixel(int x, int y) const {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
      return 0;
    }
    return m_framebuffer[static_cast<size_t>(y) * m_width + x];
  }

  /** @brief 帧缓冲宽度 */
  int width() const { return m_width; }

  /** @brief 帧缓冲高度 */
  int height() const { return m_height; }

private:
  // 屏幕空间顶点（含深度与颜色，供光栅化插值）
  struct ScreenVert {
    float x, y;     ///< 屏幕坐标（像素）
    float depth;    ///< 归一化深度 [0,1]，近=0 远=1
    uint32_t color; ///< RGBA8888
    bool invalid;   ///< true = 顶点在相机后方（近平面后），应跳过
  };

  /**
   * @brief 顶点变换：模型 → 裁剪 → NDC → 屏幕 + 深度
   * @param v 模型空间顶点
   * @param m MVP 矩阵
   * @return 屏幕空间顶点（invalid=true 表示在近平面后）
   * @note 检测裁剪 w≤0（相机后方），避免透视除法翻转 NDC 导致图元爆炸
   */
  ScreenVert transformVertex(const Vertex &v, const math::Matrix4x4 &m) const {
    // 裁剪坐标（含齐次 w）——手动计算以获取 w 符号
    const float cx = m[0] * v.pos.x + m[4] * v.pos.y + m[8] * v.pos.z + m[12];
    const float cy = m[1] * v.pos.x + m[5] * v.pos.y + m[9] * v.pos.z + m[13];
    const float cz = m[2] * v.pos.x + m[6] * v.pos.y + m[10] * v.pos.z + m[14];
    const float cw = m[3] * v.pos.x + m[7] * v.pos.y + m[11] * v.pos.z + m[15];
    if (cw <= 0.0f) {
      // 相机后方（或近平面后的无效点）：标记无效
      return {0.0f, 0.0f, 1.0f, toRGBA(v.color), true};
    }
    const float inv = 1.0f / cw;
    const math::Vector3 ndc{cx * inv, cy * inv, cz * inv};
    const float sx = (ndc.x + 1.0f) * 0.5f * static_cast<float>(m_width);
    const float sy = (1.0f - ndc.y) * 0.5f * static_cast<float>(m_height);
    // NDC z ∈ [-1,1] → 深度 [0,1]
    const float depth = (ndc.z + 1.0f) * 0.5f;
    return {sx, sy, depth, toRGBA(v.color), false};
  }

  /**
   * @brief 颜色 0..1 → RGBA8888（内存序：R 最低字节，匹配
   * SDL_PIXELFORMAT_RGBA32）
   * @note 打包 = (A<<24)|(B<<16)|(G<<8)|R
   */
  static uint32_t toRGBA(const math::Vector3 &c) {
    const auto b = [](float x) {
      const int v = static_cast<int>(x * 255.0f + 0.5f);
      return static_cast<uint32_t>(v < 0 ? 0 : (v > 255 ? 255 : v));
    };
    return (0xFFu << 24) | (b(c.z) << 16) | (b(c.y) << 8) | b(c.x);
  }

  /** @brief 写像素（带深度测试） */
  void setPixel(int x, int y, float depth, uint32_t color) {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
      return;
    }
    const size_t idx = static_cast<size_t>(y) * m_width + x;
    // 深度测试：更近（depth 更小）才覆盖
    if (depth < m_depthBuffer[idx]) {
      m_depthBuffer[idx] = depth;
      m_framebuffer[idx] = color;
    }
  }

  /**
   * @brief Bresenham 画线（含深度插值 + 深度测试）
   */
  void rasterizeLine(const ScreenVert &a, const ScreenVert &b) {
    // 简化裁剪：两端都在 NDC 外跳过（此处以屏幕范围判断）
    const int x0 = static_cast<int>(std::lround(a.x));
    const int y0 = static_cast<int>(std::lround(a.y));
    const int x1 = static_cast<int>(std::lround(b.x));
    const int y1 = static_cast<int>(std::lround(b.y));
    if ((x0 < 0 || x0 >= m_width || y0 < 0 || y0 >= m_height) &&
        (x1 < 0 || x1 >= m_width || y1 < 0 || y1 >= m_height)) {
      return;
    }
    const int dx = std::abs(x1 - x0);
    const int dy = std::abs(y1 - y0);
    const int steps = std::max(dx, dy);
    if (steps == 0) {
      setPixel(x0, y0, a.depth, a.color);
      return;
    }
    for (int i = 0; i <= steps; ++i) {
      const float t = static_cast<float>(i) / static_cast<float>(steps);
      const int x =
          x0 + static_cast<int>(std::lround(t * static_cast<float>(x1 - x0)));
      const int y =
          y0 + static_cast<int>(std::lround(t * static_cast<float>(y1 - y0)));
      const float depth = a.depth + (b.depth - a.depth) * t;
      const uint32_t color = lerpColor(a.color, b.color, t);
      setPixel(x, y, depth, color);
    }
  }

  /**
   * @brief 重心坐标光栅化一个实心三角形（第 6 讲算法）
   */
  void rasterizeTriangle(const ScreenVert &v0, const ScreenVert &v1,
                         const ScreenVert &v2) {
    // 包围盒
    const int minX =
        std::max(0, static_cast<int>(std::floor(std::min({v0.x, v1.x, v2.x}))));
    const int maxX = std::min(
        m_width - 1, static_cast<int>(std::ceil(std::max({v0.x, v1.x, v2.x}))));
    const int minY =
        std::max(0, static_cast<int>(std::floor(std::min({v0.y, v1.y, v2.y}))));
    const int maxY =
        std::min(m_height - 1,
                 static_cast<int>(std::ceil(std::max({v0.y, v1.y, v2.y}))));
    if (minX > maxX || minY > maxY) {
      return;
    }

    // 2D 叉积（有符号面积 ×2）——p 用像素坐标 (px,py)
    const auto edge = [](const ScreenVert &a, const ScreenVert &b, float px,
                         float py) {
      return (b.x - a.x) * (py - a.y) - (b.y - a.y) * (px - a.x);
    };
    const float area = edge(v0, v1, v2.x, v2.y); // 有符号面积×2
    if (std::abs(area) < 1e-6f) {
      return; // 退化三角形
    }
    // 背面剔除：屏幕坐标下逆时针（正面积）= 正面（面向相机）
    // 注意：屏幕 y 向下，绕向与标准相反；此处保留逆时针面
    // TODO: 与模型绕向约定统一后开启（当前立方体绕向未校准，先靠深度测试）
    // if (area < 0.0f) { return; }

    for (int y = minY; y <= maxY; ++y) {
      for (int x = minX; x <= maxX; ++x) {
        // 像素中心点（仅用 x/y 做重心坐标判断）
        const float px = static_cast<float>(x) + 0.5f;
        const float py = static_cast<float>(y) + 0.5f;
        const float w0 = edge(v1, v2, px, py);
        const float w1 = edge(v2, v0, px, py);
        const float w2 = edge(v0, v1, px, py);
        // 重心坐标
        const float alpha = w0 / area;
        const float beta = w1 / area;
        const float gamma = w2 / area;
        if (alpha < 0.0f || beta < 0.0f || gamma < 0.0f) {
          continue; // 三角形外
        }
        // 插值深度 + 颜色
        const float depth =
            alpha * v0.depth + beta * v1.depth + gamma * v2.depth;
        const uint32_t color =
            lerp3(v0.color, v1.color, v2.color, alpha, beta, gamma);
        setPixel(x, y, depth, color);
      }
    }
  }

  /** @brief 颜色线性插值（RGBA8888，逐通道） */
  static uint32_t lerpColor(uint32_t a, uint32_t b, float t) {
    const auto ch = [](uint32_t v, int s) { return (v >> s) & 0xFFu; };
    /* 关键：先把通道转 float 再做减法（uint32_t 无符号减法会回绕成巨大数） */
    const auto mix = [t](uint32_t ca, uint32_t cb) {
      const float fa = static_cast<float>(ca);
      const float fb = static_cast<float>(cb);
      return static_cast<uint32_t>(fa + (fb - fa) * t + 0.5f);
    };
    // RGBA 内存序：R 最低字节
    const uint32_t r = mix(ch(a, 0), ch(b, 0));
    const uint32_t g = mix(ch(a, 8), ch(b, 8));
    const uint32_t bl = mix(ch(a, 16), ch(b, 16));
    const uint32_t al = mix(ch(a, 24), ch(b, 24));
    return (al << 24) | (bl << 16) | (g << 8) | r;
  }

  /** @brief 三色重心插值 */
  static uint32_t lerp3(uint32_t c0, uint32_t c1, uint32_t c2, float a, float b,
                        float g) {
    const auto ch = [](uint32_t v, int s) { return (v >> s) & 0xFFu; };
    const auto mix = [a, b, g](uint32_t c0, uint32_t c1, uint32_t c2) {
      return static_cast<uint32_t>(c0 * a + c1 * b + c2 * g + 0.5f);
    };
    // RGBA 内存序：R 最低字节
    const uint32_t r = mix(ch(c0, 0), ch(c1, 0), ch(c2, 0));
    const uint32_t gr = mix(ch(c0, 8), ch(c1, 8), ch(c2, 8));
    const uint32_t bl = mix(ch(c0, 16), ch(c1, 16), ch(c2, 16));
    const uint32_t al = mix(ch(c0, 24), ch(c1, 24), ch(c2, 24));
    return (al << 24) | (bl << 16) | (gr << 8) | r;
  }

private:
  int m_width = 0;
  int m_height = 0;
  std::vector<uint32_t> m_framebuffer; ///< RGBA8888
  std::vector<float> m_depthBuffer;    ///< [0,1]，近=0 远=1
  std::vector<std::vector<Vertex>> m_meshes;
};

} // namespace render
