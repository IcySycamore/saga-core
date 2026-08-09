#pragma once
/**
 * @brief SDL3 渲染设备实现
 * @namespace lCYC::render
 * @details 用 SDL3 软件渲染器实现 RenderDevice 纯接口
 */

#include "core/render/RenderDevice.h"
#include <SDL3/SDL.h>
#include <vector>

namespace lCYC::render {

class SDL3Device : public RenderDevice {

private:
  SDL_Window *m_window;
  SDL_Renderer *m_renderer;
  int m_width;
  int m_height;
  std::vector<std::vector<Vertex>> m_meshes; // 顶点缓存
public:
  SDL3Device()
      : m_window(nullptr), m_renderer(nullptr), m_height(0), m_width(0) {}
  ~SDL3Device() override { shutdown(); }

  bool init(const NativeWindowHandle<void> &win, int width,
            int height) override {
    m_window = static_cast<SDL_Window *>(win.native);
    if (!m_window) {
      return false;
    }
    m_width = width;
    m_height = height;

    /*
      SDL3：SDL_CreateRenderer(window, name)——name 传 NULL 用默认渲染器
       默认是 GPU/加速渲染器；后续自建软光栅是帧缓冲
     */
    m_renderer = SDL_CreateRenderer(m_window, nullptr);
    if (!m_renderer) {
      SDL_Log("SDL3Device: SDL_CreateRenderer failed: %s", SDL_GetError());
      return false;
    }
    return true;
  }

  void shutdown() override {
    if (m_renderer) {
      SDL_DestroyRenderer(m_renderer);
      m_renderer = nullptr;
    }
    m_window = nullptr;
  }

  void beginFrame() override {
    SDL_SetRenderDrawColor(m_renderer, 0x10, 0x20, 0x40, 0xFF); // 深蓝
    SDL_RenderClear(m_renderer);
  }

  void endFrame() override { SDL_RenderPresent(m_renderer); }

  MeshHandle createMesh(std::span<const Vertex> verts) override {
    m_meshes.push_back(std::vector<Vertex>(verts.begin(), verts.end()));
    MeshHandle h;
    h.value = static_cast<uint32_t>(m_meshes.size() - 1);
    return h;
  }

  void destroyMesh(MeshHandle h) override {
    if (h.value < m_meshes.size()) {
      m_meshes[h.value].clear();
    }
  }

  void drawMesh(MeshHandle h, const lCYC::math::Matrix4x4 &mvp) override {
    if (h.value >= m_meshes.size() || m_meshes[h.value].empty()) {
      return;
    }
    const auto &verts = m_meshes[h.value];

    /* 逐段连线（线框模式）：假设顶点按顺序成对构成线段 */
    for (size_t i = 0; i + 1 < verts.size(); i += 2) {
      const auto &a = verts[i];
      const auto &b = verts[i + 1];
      drawSegment(mvp, a, b);
    }
  }

  /**
   * @brief 把当前渲染结果截图保存为 BMP 文件（调试用）
   * @param path 输出文件路径（.bmp）
   * @return true 成功；false 失败（未初始化或无渲染器）
   * @note 使用 SDL_RenderReadPixels 读取帧缓冲
   */
  bool saveScreenshot(const char *path) const {
    if (!m_renderer) {
      return false;
    }
    SDL_Surface *surface = SDL_RenderReadPixels(m_renderer, nullptr);
    if (!surface) {
      SDL_Log("saveScreenshot failed: %s", SDL_GetError());
      return false;
    }
    const bool ok = SDL_SaveBMP(surface, path) == 0;
    SDL_DestroySurface(surface);
    return ok;
  }

  void drawGrid(float size, int divs, const lCYC::math::Matrix4x4 &vp) override {
    // 地面网格：XZ 平面（y=0），从 -size 到 +size，共 divs 格
    const float half = size * 0.5f;
    const float step = size / static_cast<float>(divs);
    const lCYC::math::Vector3 color(0.4f, 0.5f, 0.6f); // 灰蓝网格线

    // 沿 X 方向的线（固定 z）
    for (int i = 0; i <= divs; ++i) {
      const float coord = -half + step * static_cast<float>(i);
      Vertex a{{coord, 0.0f, -half}, color};
      Vertex b{{coord, 0.0f, half}, color};
      drawSegment(vp, a, b);
    }
    // 沿 Z 方向的线（固定 x）
    for (int i = 0; i <= divs; ++i) {
      const float coord = -half + step * static_cast<float>(i);
      Vertex a{{-half, 0.0f, coord}, color};
      Vertex b{{half, 0.0f, coord}, color};
      drawSegment(vp, a, b);
    }
  }

private:
  /**
   * @brief 用矩阵变换一条线段端点，映射到屏幕坐标并画线
   * @param m 变换矩阵（vp 或 mvp，含投影）
   * @param a 线段端点 A
   * @param b 线段端点 B
   * @note 内部做：矩阵变换（含齐次除法）→ 裁剪 → NDC→屏幕映射 → SDL_RenderLine
   */
  void drawSegment(const lCYC::math::Matrix4x4 &m, const Vertex &a, const Vertex &b) {
    if (!m_renderer) {
      return;
    }
    // 顶点 → 裁剪（含齐次除法）→ NDC
    const lCYC::math::Vector3 ndcA = m * a.pos;
    const lCYC::math::Vector3 ndcB = m * b.pos;
    // 裁剪：完全在 NDC 之外的线段跳过（简化：任一端点越界就跳过）
    if (isOutsideNDC(ndcA) && isOutsideNDC(ndcB)) {
      return;
    }
    // NDC → 屏幕坐标
    const int x0 = static_cast<int>((ndcA.x + 1.0f) * 0.5f * m_width);
    const int y0 = static_cast<int>((1.0f - ndcA.y) * 0.5f * m_height);
    const int x1 = static_cast<int>((ndcB.x + 1.0f) * 0.5f * m_width);
    const int y1 = static_cast<int>((1.0f - ndcB.y) * 0.5f * m_height);

    /* 使用线段端点的颜色（Vertex.color 0..1 → SDL 0..255）
     */
    const auto toByte = [](float c) {
      int v = static_cast<int>(c * 255.0f + 0.5f);
      return v < 0 ? 0 : (v > 255 ? 255 : v);
    };
    SDL_SetRenderDrawColor(m_renderer, toByte(a.color.x), toByte(a.color.y),
                           toByte(a.color.z), 0xFF);
    SDL_RenderLine(m_renderer, x0, y0, x1, y1);
  }

  /**
   * @brief 判断点是否在 NDC 立方体（[-1,1]³）之外
   * @param p 归一化设备坐标
   * @return true 在可视范围外
   */
  static bool isOutsideNDC(const lCYC::math::Vector3 &p) {
    return p.x < -1.0f || p.x > 1.0f || p.y < -1.0f || p.y > 1.0f ||
           p.z < -1.0f || p.z > 1.0f;
  }
};

} // namespace lCYC::render
