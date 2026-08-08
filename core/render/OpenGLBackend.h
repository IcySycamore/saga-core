#pragma once
/**
 * @brief OpenGL 渲染后端（RenderDevice 的 GPU 实现）
 * @namespace render
 * @note 职责:
 *   - 用 OpenGL（兼容模式固定管线）实现 RenderDevice 纯接口
 *   - 与 SoftwareBackend 共用同一接口 → 双后端画面一致（M5 验收）
 *   - 深度测试由 GPU 内置（GL_DEPTH_TEST），近平面裁剪由 GPU 处理
 * @note 设计:
 *   - 固定管线（glBegin/glEnd/glLoadMatrixf）：MVP 列主序直传，顶点逐个发出
 *   - 现代管线升级点（VAO/VBO + shader）：顶点缓冲一次性上传 + uMVP/uColor，
 *     本实现为教学 MVP 保持最直观的"逐顶点发出"形式
 *   - 生命周期：GL 上下文由本类创建/销毁；窗口由外部拥有
 * @note 依赖: SDL3（窗口/GL 上下文）+ opengl32（Windows 提供 GL 1.1 核心函数）
 * @note 像素格式：帧缓冲 GL_RGBA 内存序（R 最低字节），与软光栅一致
 * @note C++20
 */

#include "core/render/RenderDevice.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include <cstdio>
#include <vector>

namespace render {

class OpenGLBackend : public RenderDevice {
public:
  // ============================ 生命周期 ============================

  /**
   * @brief 初始化 OpenGL 上下文（绑定到平台窗口）
   * @param win 平台窗口句柄（外部创建，本类不拥有）
   * @param width 逻辑宽度（像素）
   * @param height 逻辑高度（像素）
   * @return true 成功；false 失败（窗口无效或上下文创建失败）
   * @note 要求窗口以 SDL_WINDOW_OPENGL 标志创建
   */
  bool init(const NativeWindowHandle<void> &win, int width,
            int height) override {
    m_window = static_cast<SDL_Window *>(win.native);
    if (!m_window) {
      return false;
    }
    m_width = width;
    m_height = height;
    m_gl = SDL_GL_CreateContext(m_window);
    if (!m_gl) {
      SDL_Log("OpenGLBackend: SDL_GL_CreateContext failed: %s", SDL_GetError());
      return false;
    }
    SDL_GL_SetSwapInterval(1); // vsync（垂直同步）
    glEnable(GL_DEPTH_TEST);   // 深度测试（GPU 内置，对应软光栅 setPixel）
    // 清屏色 = 软光栅清屏色深灰蓝 (24,32,48)，保证双后端画面一致
    glClearColor(24.0f / 255.0f, 32.0f / 255.0f, 48.0f / 255.0f, 1.0f);
    return true;
  }

  /** @brief 释放 GL 上下文与网格缓存（不销毁窗口） */
  void shutdown() override {
    m_meshes.clear();
    if (m_gl) {
      SDL_GL_DestroyContext(m_gl);
      m_gl = nullptr;
    }
    m_window = nullptr;
  }

  // ============================ 帧控制 ============================

  /** @brief 清颜色 + 深度，开始新一帧 */
  void beginFrame() override {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  /** @brief 交换前后缓冲，把画好的内容显示到窗口 */
  void endFrame() override { SDL_GL_SwapWindow(m_window); }

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
   * @param mvp 模型视图投影矩阵 = P*V*M（列主序，直传 GPU）
   * @note 近平面裁剪由 GPU 自动处理（与软光栅 invalid 丢弃不同的完整裁剪）
   */
  void drawMesh(MeshHandle h, const math::Matrix4x4 &mvp) override {
    if (h.value >= m_meshes.size() || m_meshes[h.value].empty()) {
      return;
    }
    const auto &verts = m_meshes[h.value];
    loadMVP(mvp);
    const size_t n = verts.size();
    // 三角形：每 3 个顶点
    size_t i = 0;
    glBegin(GL_TRIANGLES);
    for (; i + 2 < n; i += 3) {
      for (int k = 0; k < 3; ++k) {
        emitVertex(verts[i + static_cast<size_t>(k)]);
      }
    }
    glEnd();
    // 剩余一对：线段
    glBegin(GL_LINES);
    for (; i + 1 < n; i += 2) {
      emitVertex(verts[i]);
      emitVertex(verts[i + 1]);
    }
    glEnd();
  }

  /**
   * @brief 绘制地面网格线（XZ 平面 y=0）
   * @param size 总尺寸（世界单位）
   * @param divs 每边格数
   * @param vp 视图投影矩阵 V*P（列主序）
   */
  void drawGrid(float size, int divs, const math::Matrix4x4 &vp) override {
    const float half = size * 0.5f;
    const float step = size / static_cast<float>(divs);
    loadMVP(vp);
    glBegin(GL_LINES);
    for (int i = 0; i <= divs; ++i) {
      const float coord = -half + step * static_cast<float>(i);
      glColor3f(0.65f, 0.75f, 0.85f); // 网格色与软光栅一致
      glVertex3f(coord, 0.0f, -half);
      glVertex3f(coord, 0.0f, half);
      glVertex3f(-half, 0.0f, coord);
      glVertex3f(half, 0.0f, coord);
    }
    glEnd();
  }

  /**
   * @brief 把当前帧缓冲截图保存为 BMP（调试/对比双后端用）
   * @param path 输出路径（.bmp）
   * @return true 成功
   * @note 用 glReadPixels 读后缓冲，调用须在 swap 之前（endFrame 前）
   */
  bool saveScreenshot(const char *path) const {
    if (!m_gl) {
      return false;
    }
    std::vector<unsigned char> pixels(static_cast<size_t>(m_width) * m_height *
                                      4);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE,
                 pixels.data());
    return writeBMP(path, pixels.data(), m_width, m_height);
  }

private:
  /**
   * @brief 把列主序矩阵直传给 GPU（MODELVIEW 载入；PROJECTION 恒等）
   * @param m 变换矩阵（列主序，与 OpenGL 内存布局一致，直接 glLoadMatrixf）
   */
  static void loadMVP(const math::Matrix4x4 &m) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(m.data()); // 列主序直传
  }

  /**
   * @brief 发出一个顶点（颜色 + 位置；模型空间，由 GPU 完成 MVP 变换）
   * @param v 模型空间顶点
   */
  static void emitVertex(const Vertex &v) {
    glColor3f(v.color.x, v.color.y, v.color.z);
    glVertex3f(v.pos.x, v.pos.y, v.pos.z);
  }

  /**
   * @brief 写 BMP 文件（像素为 GL_RGBA 内存序 R,G,B,A；GL 行 0 = 底部，
   *        与 BMP 数据行顺序一致，无需翻转）
   * @param path 输出路径
   * @param rgba RGBA 像素数据（width*height*4 字节）
   * @param w 宽度
   * @param h 高度
   * @return true 成功
   */
  static bool writeBMP(const char *path, const unsigned char *rgba, int w,
                       int h) {
    FILE *fp = std::fopen(path, "wb");
    if (!fp) {
      return false;
    }
    const int rowSize = (w * 3 + 3) & ~3; // 每行补到 4 字节对齐
    const int dataSize = rowSize * h;
    unsigned char header[54] = {0};
    header[0] = 'B';
    header[1] = 'M';
    const unsigned int fileSize = 54u + static_cast<unsigned int>(dataSize);
    header[2] = static_cast<unsigned char>(fileSize);
    header[3] = static_cast<unsigned char>(fileSize >> 8);
    header[4] = static_cast<unsigned char>(fileSize >> 16);
    header[5] = static_cast<unsigned char>(fileSize >> 24);
    header[10] = 54; // 数据偏移
    header[14] = 40; // info 头大小
    header[18] = static_cast<unsigned char>(w);
    header[19] = static_cast<unsigned char>(w >> 8);
    header[20] = static_cast<unsigned char>(w >> 16);
    header[21] = static_cast<unsigned char>(w >> 24);
    header[22] = static_cast<unsigned char>(h);
    header[23] = static_cast<unsigned char>(h >> 8);
    header[24] = static_cast<unsigned char>(h >> 16);
    header[25] = static_cast<unsigned char>(h >> 24);
    header[26] = 1;  // 平面数
    header[28] = 24; // 24bit
    std::fwrite(header, 1, 54, fp);
    std::vector<unsigned char> row(static_cast<size_t>(rowSize));
    for (int y = 0; y < h; ++y) {
      int i = 0;
      for (int x = 0; x < w; ++x) {
        const unsigned char *p = rgba + (static_cast<size_t>(y) * w + x) * 4;
        row[i++] = p[2]; // B
        row[i++] = p[1]; // G
        row[i++] = p[0]; // R
      }
      std::fwrite(row.data(), 1, static_cast<size_t>(rowSize), fp);
    }
    std::fclose(fp);
    return true;
  }

  SDL_Window *m_window = nullptr;
  SDL_GLContext m_gl = nullptr;
  int m_width = 0;
  int m_height = 0;
  std::vector<std::vector<Vertex>> m_meshes;
};

} // namespace render
