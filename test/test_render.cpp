// test/test_render.cpp
// SoftwareBackend 软光栅单测（确定性验证）
//
// 编译与运行:
//   cmake -S . -B build/vscodeBuild && cmake --build build/vscodeBuild --target
//   test_render
//   .\build\vscodeBuild\test_render.exe

#include "core/render/SoftwareBackend.h"
#include <iostream>

// ===================== 轻量断言宏 =====================
static int g_passed = 0;
static int g_failed = 0;

#define TEST(name) std::cout << "\n[RUN] " << name << std::endl
#define EXPECT(cond, msg)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::cerr << "  FAILED: " << msg << std::endl;                           \
      ++g_failed;                                                              \
    } else {                                                                   \
      ++g_passed;                                                              \
    }                                                                          \
  } while (false)
#define EXPECT_EQ(a, b) EXPECT((a) == (b), #a " == " #b)

// 颜色通道提取（RGBA 内存序：R 最低字节）
static int r8(uint32_t c) { return (c >> 0) & 0xFF; }
static int g8(uint32_t c) { return (c >> 8) & 0xFF; }
static int b8(uint32_t c) { return (c >> 16) & 0xFF; }

// beginFrame 后的清屏背景色（深灰蓝，RGBA 内存序打包）
static constexpr uint32_t kClearColor = 0xFF302018u;

// ===================== 1: init 后帧缓冲清空 =====================
static void test_init_clear() {
  TEST("init_clear");

  render::SoftwareBackend sw;
  render::NativeWindowHandle<void> dummy;
  EXPECT(sw.init(dummy, 64, 48), "init should succeed");
  EXPECT_EQ(sw.width(), 64);
  EXPECT_EQ(sw.height(), 48);
  // 未 beginFrame 时默认全黑
  EXPECT_EQ(sw.pixel(0, 0), 0xFF000000u);
  sw.shutdown();
}

// ===================== 2: 全屏三角形（确定性颜色） =====================
static void test_fullscreen_triangle() {
  TEST("fullscreen_triangle");

  render::SoftwareBackend sw;
  render::NativeWindowHandle<void> dummy;
  sw.init(dummy, 32, 32);

  // 覆盖整个屏幕的三角形（三个顶点在 NDC 角落）
  const render::Vertex verts[3] = {
      {{-1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}}, // 红
      {{3.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
      {{-1.0f, 3.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
  };
  const render::MeshHandle h = sw.createMesh(verts);

  sw.beginFrame();
  sw.drawMesh(h, math::Matrix4x4::identity());
  sw.endFrame();

  // 屏幕中心应被红色覆盖
  const uint32_t c = sw.pixel(16, 16);
  EXPECT(r8(c) > 200 && g8(c) < 50 && b8(c) < 50, "center pixel should be red");
  sw.shutdown();
}

// ===================== 3: 深度测试（近的遮挡远的） =====================
static void test_depth_test() {
  TEST("depth_test");

  render::SoftwareBackend sw;
  render::NativeWindowHandle<void> dummy;
  sw.init(dummy, 32, 32);

  // 两个全屏三角形：一个远（z=0.5），一个近（z=-0.5）
  const render::Vertex far[3] = {
      {{-1.0f, -1.0f, 0.5f}, {0.0f, 0.0f, 1.0f}}, // 蓝（远）
      {{3.0f, -1.0f, 0.5f}, {0.0f, 0.0f, 1.0f}},
      {{-1.0f, 3.0f, 0.5f}, {0.0f, 0.0f, 1.0f}},
  };
  const render::Vertex near_[3] = {
      {{-1.0f, -1.0f, -0.5f}, {1.0f, 0.0f, 0.0f}}, // 红（近）
      {{3.0f, -1.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
      {{-1.0f, 3.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
  };
  const render::MeshHandle hFar = sw.createMesh(far);
  const render::MeshHandle hNear = sw.createMesh(near_);

  sw.beginFrame();
  sw.drawMesh(hFar, math::Matrix4x4::identity());  // 先画远的
  sw.drawMesh(hNear, math::Matrix4x4::identity()); // 再画近的
  sw.endFrame();

  // 中心应显示红色（近的遮挡远的，无论绘制顺序）
  const uint32_t c = sw.pixel(16, 16);
  EXPECT(r8(c) > 200 && b8(c) < 50, "near (red) should occlude far (blue)");
  sw.shutdown();
}

// ===================== 4: 网格线渲染（非全黑） =====================
static void test_grid_lines() {
  TEST("grid_lines");

  render::SoftwareBackend sw;
  render::NativeWindowHandle<void> dummy;
  sw.init(dummy, 64, 64);

  sw.beginFrame();
  sw.drawGrid(2.0f, 4, math::Matrix4x4::identity());
  sw.endFrame();

  // 网格线应产生非背景像素
  bool any = false;
  for (int y = 0; y < 64 && !any; y += 2) {
    for (int x = 0; x < 64; x += 2) {
      if (sw.pixel(x, y) != kClearColor) {
        any = true;
        break;
      }
    }
  }
  EXPECT(any, "grid should draw some pixels");
  sw.shutdown();
}

// ===================== 5: 无效句柄安全 =====================
static void test_invalid_handle() {
  TEST("invalid_handle");

  render::SoftwareBackend sw;
  render::NativeWindowHandle<void> dummy;
  sw.init(dummy, 32, 32);

  sw.beginFrame();
  sw.drawMesh(render::MeshHandle{}, math::Matrix4x4::identity()); // 无效句柄
  sw.drawMesh(render::MeshHandle{999}, math::Matrix4x4::identity());
  sw.endFrame();
  // 不应崩溃，帧缓冲仍为清屏背景色
  EXPECT_EQ(sw.pixel(0, 0), kClearColor);
  sw.shutdown();
}

// ===================== main =====================
int main() {
  std::cout << "=== SoftwareBackend Tests ===" << std::endl;

  test_init_clear();
  test_fullscreen_triangle();
  test_depth_test();
  test_grid_lines();
  test_invalid_handle();

  std::cout << "\n=== Results: " << g_passed << " passed, " << g_failed
            << " failed ===" << std::endl;
  return g_failed == 0 ? 0 : 1;
}
