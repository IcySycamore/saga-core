/* tools/render_demo.cpp
 * 渲染 MVP 演示入口（M3-M5：SDL3 窗口 + RenderDevice 抽象 + Camera + 立方体）
 *
 * 编译与运行:
 *   cmake -S . -B build/vscodeBuild && cmake --build build/vscodeBuild --target
 * render_demo
 *   .\build\vscodeBuild\render_demo.exe              # 默认 SDL3 后端
 *   .\build\vscodeBuild\render_demo.exe --backend software   # 软件光栅化后端
 *
 * 说明:
 *   - 本文件是"应用层"：负责创建 SDL3 窗口（容器）、组装 RenderDevice、场景内容
 *   - 引擎层 core/render 不包含 SDL 头；SDL 只在这里（窗口创建）与
 * SDL3RenderDevice.h
 *   - M3：Camera + 透视投影（MVP 串联）
 *   - M4：线框立方体 + Clock 驱动旋转动画 + 多物体
 *   - M5：SoftwareBackend（软光栅）接入，用 SDL 纹理显示帧缓冲
 *
 * 操作:
 *   - P 键：切换透视/正交投影
 *   - 关闭窗口退出
 */

#include "core/Time/Clock.h"
#include "core/render/Camera.h"
#include "core/render/RenderDevice.h"
#include "core/render/SDL3RenderDevice.h"
#include "core/render/SoftwareBackend.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

/**
 * @brief 构建立方体线框顶点（12 条边 = 24 个顶点，成对表示线段）
 * @param half 半边长
 * @param color 线框颜色
 * @return 顶点数组（成对 = 一条线段）
 */
static std::vector<render::Vertex> makeCubeEdges(float half,
                                                 const math::Vector3 &color) {
  // 8 个角点（模型空间，中心在原点）
  const math::Vector3 v[8] = {
      {-half, -half, -half}, {half, -half, -half}, {half, half, -half},
      {-half, half, -half},  {-half, -half, half}, {half, -half, half},
      {half, half, half},    {-half, half, half},
  };
  // 12 条边（顶点索引对）
  const int edges[12][2] = {
      {0, 1}, {1, 2}, {2, 3}, {3, 0}, // 前面
      {4, 5}, {5, 6}, {6, 7}, {7, 4}, // 后面
      {0, 4}, {1, 5}, {2, 6}, {3, 7}, // 连接
  };
  std::vector<render::Vertex> verts;
  verts.reserve(24);
  for (const auto &e : edges) {
    verts.push_back({v[e[0]], color});
    verts.push_back({v[e[1]], color});
  }
  return verts;
}

/**
 * @brief 构建立方体实心三角形顶点（6 面 × 2 = 12 个三角形 = 36 顶点）
 * @param half 半边长
 * @param color 表面颜色（每面不同色用于区分）
 * @return 顶点数组（每 3 个顶点 = 一个三角形）
 */
static std::vector<render::Vertex> makeCubeSolid(float half,
                                                 const math::Vector3 &color) {
  // 8 个角点
  const math::Vector3 v[8] = {
      {-half, -half, -half}, {half, -half, -half}, {half, half, -half},
      {-half, half, -half},  {-half, -half, half}, {half, -half, half},
      {half, half, half},    {-half, half, half},
  };
  // 6 面，每面 2 个三角形（逆时针绕向，正面朝外）
  const int faces[6][4] = {
      {0, 3, 2, 1}, // 前面 z-
      {5, 6, 7, 4}, // 后面 z+
      {4, 7, 3, 0}, // 左面 x-
      {1, 2, 6, 5}, // 右面 x+
      {3, 7, 6, 2}, // 上面 y+
      {4, 0, 1, 5}, // 下面 y-
  };
  // 每面一个颜色微调（便于观察背面剔除/旋转）
  const float shades[6][3] = {
      {1.0f, 0.85f, 0.85f}, {0.85f, 0.85f, 1.0f}, {0.85f, 1.0f, 0.85f},
      {1.0f, 1.0f, 0.85f},  {1.0f, 0.9f, 0.95f},  {0.9f, 0.95f, 1.0f},
  };
  std::vector<render::Vertex> verts;
  verts.reserve(36);
  for (int f = 0; f < 6; ++f) {
    const math::Vector3 fc{color.x * shades[f][0], color.y * shades[f][1],
                           color.z * shades[f][2]};
    const auto &q = faces[f];
    verts.push_back({v[q[0]], fc});
    verts.push_back({v[q[1]], fc});
    verts.push_back({v[q[2]], fc});
    verts.push_back({v[q[0]], fc});
    verts.push_back({v[q[2]], fc});
    verts.push_back({v[q[3]], fc});
  }
  return verts;
}

/**
 * @brief 把 SoftwareBackend 帧缓冲保存为 BMP 文件（调试用）
 * @param sw 软光栅后端
 * @param path 输出路径
 * @return true 成功
 */
static bool swSaveBMP(const render::SoftwareBackend *sw, const char *path) {
  FILE *fp = std::fopen(path, "wb");
  if (!fp) {
    return false;
  }
  const int w = sw->width();
  const int h = sw->height();
  const int rowSize = (w * 3 + 3) & ~3; // 每行补到 4 字节对齐
  const int dataSize = rowSize * h;

  // BMP 头（BITMAPFILEHEADER + BITMAPINFOHEADER）
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

  // 像素（BGR，从底行往上）——RGBA 内存序：R 最低字节
  std::vector<unsigned char> row(rowSize);
  for (int y = h - 1; y >= 0; --y) {
    int i = 0;
    for (int x = 0; x < w; ++x) {
      const uint32_t c = sw->pixel(x, y);
      row[i++] = static_cast<unsigned char>((c >> 16) & 0xFF); // B
      row[i++] = static_cast<unsigned char>((c >> 8) & 0xFF);  // G
      row[i++] = static_cast<unsigned char>((c >> 0) & 0xFF);  // R
    }
    std::fwrite(row.data(), 1, rowSize, fp);
  }
  std::fclose(fp);
  return true;
}

int main(int argc, char *argv[]) {
  // 可选参数：--backend software（用软光栅）/ --screenshot <path>
  const char *shotPath = nullptr;
  bool useSoftware = false;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--screenshot") == 0 && i + 1 < argc) {
      shotPath = argv[i + 1];
    }
    if (std::strcmp(argv[i], "--backend") == 0 && i + 1 < argc) {
      useSoftware = (std::strcmp(argv[i + 1], "software") == 0);
    }
  }

  const int width = 1280;
  const int height = 720;

  // ---- 初始化 SDL3（翻译官）----
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    std::fprintf(stderr, "SDL_Init 失败: %s\n", SDL_GetError());
    return 1;
  }

  // ---- 创建窗口（容器/屏幕）----
  // SDL3: SDL_CreateWindow(title, w, h, flags)——位置由属性控制，默认居中
  SDL_Window *window =
      SDL_CreateWindow("Saga Render Demo", width, height, SDL_WINDOW_RESIZABLE);
  if (!window) {
    std::fprintf(stderr, "SDL_CreateWindow 失败: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  /* ---- 组装 RenderDevice ----
   * SDL3 后端：直接用 SDL 渲染器画（线框）
   * SoftwareBackend：CPU 光栅化到帧缓冲，再用 SDL 纹理显示 */
  render::NativeWindowHandle<SDL_Window> handle;
  handle.native = window;

  // SDL 渲染器（两种后端都需要：软件后端用它显示帧缓冲纹理）
  SDL_Renderer *sdlRenderer = SDL_CreateRenderer(window, nullptr);
  if (!sdlRenderer) {
    std::fprintf(stderr, "SDL_CreateRenderer 失败: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  // 软件后端的显示纹理（RGBA8888 帧缓冲 → GPU 纹理）
  SDL_Texture *fbTexture = nullptr;
  if (useSoftware) {
    fbTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_RGBA32,
                                  SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!fbTexture) {
      std::fprintf(stderr, "SDL_CreateTexture 失败: %s\n", SDL_GetError());
      SDL_DestroyRenderer(sdlRenderer);
      SDL_DestroyWindow(window);
      SDL_Quit();
      return 1;
    }
  }

  render::RenderDevice *device = nullptr;
  render::SDL3RenderDevice sdlDevice;
  render::SoftwareBackend swDevice;
  if (useSoftware) {
    swDevice.init({}, width, height); // 软光栅无需窗口句柄
    device = &swDevice;
  } else {
    sdlDevice.init(handle, width, height);
    device = &sdlDevice;
  }

  // ---- 相机（M3：透视 + 斜视角）----
  render::Camera camera;
  camera.setPerspective(60.0f * math::PI / 180.0f, 0.1f, 100.0f);
  camera.lookAt({3.0f, 3.0f, 3.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

  // 屏幕宽高比（渲染分辨率固定 1280x720，因此宽高比恒定）
  const float aspect = static_cast<float>(width) / static_cast<float>(height);

  // ---- 场景内容（M4）----
  // 旋转立方体：SDL3 后端用线框，软光栅用实心三角形
  const render::MeshHandle rotatingCube =
      useSoftware ? device->createMesh(makeCubeSolid(0.8f, {0.9f, 0.9f, 0.9f}))
                  : device->createMesh(makeCubeEdges(0.8f, {0.9f, 0.9f, 0.9f}));
  // 静止立方体（橙色，偏移到右侧，用于对比遮挡）
  const render::MeshHandle staticCube =
      useSoftware ? device->createMesh(makeCubeSolid(0.5f, {1.0f, 0.6f, 0.2f}))
                  : device->createMesh(makeCubeEdges(0.5f, {1.0f, 0.6f, 0.2f}));

  // 逻辑时钟（固定步长，驱动动画）
  clockns::Clock clock;

  std::printf("窗口已创建: %dx%d，后端=%s 就绪\n", width, height,
              useSoftware ? "Software(软光栅)" : "SDL3");
  std::printf("（P 键切换透视/正交，关闭窗口退出）\n");

  // ---- 主循环：处理事件直到用户关窗 ----
  bool running = true;
  int frame = 0;
  bool shotSaved = (shotPath == nullptr); // 无截图参数则无需保存
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
      /* 窗口 resize：渲染分辨率固定 1280x720（软光栅帧缓冲不随窗口变），
       * 显示层用 letterbox 等比缩放适配窗口，因此这里无需重建帧缓冲/纹理。
       * 好处：画面比例恒定、FPS 恒定（旋转速度不随窗口大小变化） */
      // P 键：切换透视/正交投影
      // 正交范围按 aspect 调整（x 范围更宽），保证世界比例不被屏幕拉伸
      if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_P) {
        if (camera.projectionType() == render::Proj::Perspective) {
          camera.setOrthographic(-3.0f * aspect, 3.0f * aspect, -3.0f, 3.0f,
                                 0.1f, 100.0f);
        } else {
          camera.setPerspective(60.0f * math::PI / 180.0f, 0.1f, 100.0f);
        }
      }
    }

    // 驱动逻辑时钟（固定步长 tick，驱动动画）
    clock.addDuration(1.0 / 60.0);

    // 渲染一帧（aspect 用当前帧缓冲/窗口尺寸，resize 后自动更新）
    device->beginFrame();
    const int curW = useSoftware ? swDevice.width() : width;
    const int curH = useSoftware ? swDevice.height() : height;
    const float curAspect = static_cast<float>(curW) / static_cast<float>(curH);
    const math::Matrix4x4 vp = camera.viewProjection(curAspect);

    // 地面网格
    device->drawGrid(4.0f, 8, vp);

    /* 旋转立方体：绕 Y 轴匀速旋转（用 Clock 的逻辑时间驱动，确定性）
     * angle = omega * t，t = clock.getLogicTime()（tick 对齐，可回放）
     * M = T(0,0.8,0) * R：先绕自身中心旋转，再平移到网格上方（底贴 y=0） */
    const double t = clock.getLogicTime();
    const float angle = static_cast<float>(t) * 1.5f; // 1.5 rad/s
    const math::Quaternion rot = math::axisAngle({0.0f, 1.0f, 0.0f}, angle);
    const math::Matrix4x4 M_rot =
        math::translation({0.0f, 0.8f, 0.0f}) * math::rotation(rot);
    device->drawMesh(rotatingCube, vp * M_rot);

    // 静止立方体：偏移到 (1.5, 0.5, 0)（半边长 0.5 → 底贴网格）
    const math::Matrix4x4 M_static = math::translation({1.5f, 0.5f, 0.0f});
    device->drawMesh(staticCube, vp * M_static);

    device->endFrame();

    // 显示：SDL3 后端直接 present；软光栅把帧缓冲上传到纹理再画
    if (useSoftware) {
      const auto sw = static_cast<render::SoftwareBackend *>(device);
      /* 关键：用当前帧缓冲尺寸拷贝（resize 后帧缓冲/纹理已重建为
       * 新尺寸，若用初始 width/height 会行错位 → 画面一片一片） */
      const int fbW = sw->width();
      const int fbH = sw->height();
      void *pixels = nullptr;
      int pitch = 0;
      if (SDL_LockTexture(fbTexture, nullptr, &pixels, &pitch)) {
        const auto fb = sw->framebuffer();
        const int bytesPerRow = fbW * 4;
        for (int y = 0; y < fbH; ++y) {
          std::memcpy(static_cast<char *>(pixels) + y * pitch,
                      fb.data() + y * fbW, bytesPerRow);
        }
        SDL_UnlockTexture(fbTexture);
        /* letterbox：背景填帧缓冲清屏色（深灰蓝 24,32,48），
         * 画面按当前宽高比居中缩放，拉伸窗口不变形 */
        SDL_SetRenderDrawColor(sdlRenderer, 24, 32, 48, 255);
        SDL_RenderClear(sdlRenderer);
        int winW = 0, winH = 0;
        SDL_GetWindowSize(window, &winW, &winH);
        if (winW > 0 && winH > 0) {
          const float scale = std::min(static_cast<float>(winW) / fbW,
                                       static_cast<float>(winH) / fbH);
          const float dstW = static_cast<float>(fbW) * scale;
          const float dstH = static_cast<float>(fbH) * scale;
          const SDL_FRect dst{(winW - dstW) * 0.5f, (winH - dstH) * 0.5f, dstW,
                              dstH};
          SDL_RenderTexture(sdlRenderer, fbTexture, nullptr, &dst);
        }
        SDL_RenderPresent(sdlRenderer);
      }
    }

    // 截图模式：N 帧后保存并退出
    if (!shotSaved && ++frame >= 3) {
      if (useSoftware) {
        // 软光栅：帧缓冲直接存 BMP
        const auto sw = static_cast<render::SoftwareBackend *>(device);
        if (swSaveBMP(sw, shotPath)) {
          std::printf("截图已保存: %s\n", shotPath);
        } else {
          std::fprintf(stderr, "截图失败\n");
        }
      } else {
        if (sdlDevice.saveScreenshot(shotPath)) {
          std::printf("截图已保存: %s\n", shotPath);
        } else {
          std::fprintf(stderr, "截图失败\n");
        }
      }
      shotSaved = true;
      running = false;
    }
  }

  // ---- 清理 ----
  device->shutdown();
  if (fbTexture) {
    SDL_DestroyTexture(fbTexture);
  }
  SDL_DestroyRenderer(sdlRenderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  std::printf("窗口已关闭，正常退出\n");
  return 0;
}
