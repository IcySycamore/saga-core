# 渲染 MVP 实现规划（SDL2 + OpenGL / 软光栅双后端）

> 关联：issue #13（图形 API 抽象层）、#14（Camera）、#15（场景图）、#16（基础渲染循环）、#17（主循环整合）
> 前置：`core/math`（M1-M7，301 断言已绿）、`clockns::Clock`（时间系统，25 断言已绿）、SDL2（zlib 许可，已安装）

---

## 1. 计划实现目标

**在 2-3 周内跑通一条完整的 3D 渲染管线**，从"窗口"到"可交互的 3D 场景"，为后续（材质/光照/完整 ECS 场景图）打地基。

可交付物：

1. 一个可运行的窗口程序（`render_demo`）：SDL2 窗口 + 可交互 3D 场景
2. 一个可测试的软件光栅化后端（`SoftwareDevice`）：CPU 渲染到帧缓冲，可单测
3. 一个 OpenGL 后端（`OpenGLDevice`）：GPU 渲染，作为实际显示后端
4. 可替换后端的渲染抽象层（`RenderDevice` + `RenderServer`）
5. 主循环整合：`clockns::Clock` 驱动逻辑 tick + 独立渲染帧

**非目标（YAGNI）**：不实现光照/阴影/纹理/材质/粒子/动画——它们是后续 issue。

## 2. 实现架构

```
┌────────────────────────────────────────────────┐
│ 应用层  App（render_demo / 游戏逻辑）           │
│  - 持有 Clock / SceneGraph / Camera / RenderServer │
│  - 每 tick: 逻辑更新 → 提交绘制命令             │
└──────────────┬─────────────────────────────────┘
               │ 命令队列（解耦）
┌──────────────▼─────────────────────────────────┐
│ 引擎层  RenderServer（core/render/）            │
│  - setBackend / setCamera / drawMesh / render   │
│  - SceneGraph（Transform 树）+ Camera 组件       │
└──────────────┬─────────────────────────────────┘
               │ 后端接口
┌──────────────▼─────────────────────────────────┐
│ 图形抽象层  RenderDevice（core/render/）        │
│  ├── SoftwareDevice：CPU 光栅化 → 帧缓冲（可测）│
│  └── OpenGLDevice：GPU → SDL2 窗口（显示）     │
└─────────────────────────────────────────────────┘
```

### 关键设计决策

| 决策     | 选择                                              | 理由                                                              |
| -------- | ------------------------------------------------- | ----------------------------------------------------------------- |
| 后端抽象 | `RenderDevice` 纯接口 + 双实现                    | 软光栅可单测（确定性），OpenGL 可显示；切换零成本                 |
| 解耦方式 | RenderServer 命令队列                             | Godot RenderingServer 风格，逻辑层不碰 GPU，天然支持 Headless     |
| 场景组织 | `SceneGraph`（Transform 树）独立于 ECS            | 渲染专用结构，避免污染 EntityManager；issue #11 后再决定 ECS 融合 |
| 渲染帧率 | 逻辑 tick（Clock 固定步长）+ 渲染帧（vsync 独立） | 确定性逻辑 + 流畅渲染                                             |
| 绘制内容 | 线框网格 + 线框立方体                             | 无光照/材质，验证全管线足够                                       |

### 关键设计决策（四维度审查补充）

> 针对草案接口的严格审查（封装性/安全性、性能、可扩展性、复用性），见 3.2 改进版接口。

| 维度      | 原草案问题                             | 改进方案                                                              |
| --------- | -------------------------------------- | --------------------------------------------------------------------- |
| 封装/安全 | `void* windowHandle` 类型不安全        | `NativeWindowHandle` 不透明句柄 + `explicit operator bool`            |
| 封装/安全 | 裸指针+长度 `(float*, int)`            | `std::span<const Vertex>`（C++20，自带长度与边界）                    |
| 封装/安全 | `Transform* parent` 悬垂风险           | `SceneGraph` 唯一所有权，节点句柄 = 索引，destroy 级联 + 修复孤儿     |
| 封装/安全 | `setBackend(RenderDevice*)` 所有权不明 | 引用传参（外部拥有，RenderServer 不删）；`clearBackend()` 显式解除    |
| 封装/安全 | public 字段无校验                      | setter 带不变量（fov∈(0,π)、near>0、far>near）；节点访问经 SceneGraph |
| 性能      | 每帧传顶点数组 → VBO 重建              | `createMesh` 上传一次，`drawMesh` 引用 `MeshHandle`                   |
| 性能      | `world()` 每次递归重算                 | 脏标记 + `worldCache`（setLocal 置脏，world 惰性重算）                |
| 性能      | 每帧动态分配命令                       | RenderServer 预分配命令池（vector 复用，不清容量）                    |
| 扩展      | 顶点格式固定 `float*`                  | `Vertex{pos, color}` 结构，未来可加法线/UV 字段                       |
| 扩展      | 无资源概念                             | `MeshHandle` 句柄 + create/destroy，未来纹理/材质同模式               |
| 扩展      | 正交参数缺失                           | `setOrthographic(l,r,b,t,n,f)` 完整参数                               |
| 复用      | 裸类型贯穿接口                         | 不透明句柄 + 资源池模式，多后端共享同一接口                           |

## 3. 实现要点

### 3.1 目录与文件

```
core/render/
  RenderDevice.h       // RenderDevice 纯接口（底层设备抽象）
  SoftwareDevice.h    // 软光栅（framebuffer + depthBuffer）
  OpenGLDevice.h      // OpenGL 后端
  RenderServer.h       // 命令队列 + 设备调度
  SceneGraph.h         // Transform 树（local/world 矩阵）
  Camera.h             // 相机（位置/朝向/投影）
core/engine/
  Engine.h             // 主循环（Clock + 各系统组装）
tools/ (已存在)
  render_demo.cpp      // 演示入口（SDL2 窗口 + 场景）
test/
  test_render.cpp      // 软光栅/场景图/相机 单测
```

### 3.2 接口草案（改进版：四维度审查后）

```cpp
// core/render/RenderDevice.h —— 纯接口，逻辑层唯一接触的图形面（对应 Godot RenderingDevice / Unreal RHI）
namespace render {

// 不透明窗口句柄：封装平台类型，避免裸 void*（类型安全）
struct NativeWindowHandle {
  void* native = nullptr;
  explicit operator bool() const { return native != nullptr; }
};

// 网格资源句柄：上传一次，绘制引用（性能关键）
using MeshHandle = uint32_t;
inline constexpr MeshHandle kInvalidMesh = ~0u;

// 顶点格式：支持位置+颜色；未来加法线/UV 只需扩展字段
struct Vertex {
  math::Vector3 pos;
  math::Vector3 color;   // 或 uint32_t 打包（RGBA）
};

class RenderDevice {
public:
  virtual ~RenderDevice() = default;

  // ---- 生命周期 ----
  virtual bool init(const NativeWindowHandle& win, int width, int height) = 0;
  virtual void shutdown() = 0;

  // ---- 帧控制 ----
  virtual void beginFrame() = 0;   // 清屏 + 清深度
  virtual void endFrame() = 0;     // present

  // ---- 资源（上传一次，句柄复用）----
  virtual MeshHandle createMesh(std::span<const Vertex> verts) = 0;
  virtual void destroyMesh(MeshHandle h) = 0;

  // ---- 绘制（引用句柄 + 变换，不传裸数据）----
  virtual void drawMesh(MeshHandle h, const math::Matrix4x4& mvp) = 0;
  virtual void drawGrid(float size, int divs, const math::Matrix4x4& vp) = 0;
};

} // namespace render
```

```cpp
// core/render/SceneGraph.h —— Transform 树（唯一所有权，句柄安全）
namespace render {

class SceneGraph {
public:
  using NodeId = uint32_t;
  static constexpr NodeId kInvalidNode = ~0u;

  // ---- 生命周期：SceneGraph 拥有全部节点，返回句柄而非裸指针 ----
  NodeId createNode(NodeId parent = kInvalidNode);
  void destroyNode(NodeId id);   // 级联销毁子树 + 修复孤儿引用

  // ---- 访问（生命周期由 SceneGraph 保证，无悬垂）----
  NodeId parentOf(NodeId id) const;
  bool isValid(NodeId id) const;

  // ---- 局部变换（setter 置脏）----
  void setLocalPos(NodeId id, const math::Vector3& p);
  void setLocalRot(NodeId id, const math::Quaternion& q);
  void setLocalScale(NodeId id, const math::Vector3& s);

  // ---- 世界矩阵（脏标记 + 缓存，惰性重算）----
  const math::Matrix4x4& world(NodeId id) const;

private:
  struct Node {
    math::Vector3    localPos{0,0,0};
    math::Quaternion localRot{};
    math::Vector3    localScale{1,1,1};
    NodeId parent = kInvalidNode;
    mutable bool dirty = true;
    mutable math::Matrix4x4 worldCache;
  };
  std::vector<Node>  m_nodes;   // 句柄 = 索引
  std::vector<NodeId> m_free;   // 空闲回收
};

} // namespace render
```

```cpp
// core/render/Camera.h —— 视图 + 投影（不变量经 setter 保护）
namespace render {

class Camera {
public:
  enum class Proj { Perspective, Orthographic };

  // setter 校验不变量：fov∈(0,π)、near>0、far>near、正交 left<right etc.
  void setPerspective(float fovY, float near_, float far_);
  void setOrthographic(float l, float r, float b, float t, float near_, float far_);
  void setProj(Proj p);
  void lookAt(const math::Vector3& eye, const math::Vector3& target,
              const math::Vector3& up = math::Vector3::up());

  math::Matrix4x4 view() const;         // lookAt
  math::Matrix4x4 projection(float aspect) const;

private:
  Proj m_proj = Proj::Perspective;
  math::Vector3 m_eye{0,0,5}, m_target{0,0,0}, m_up{0,1,0};
  float m_fovY = 60.0f * math::PI / 180.0f;
  float m_near = 0.1f, m_far = 100.0f;
  // 正交参数（Proj==Orthographic 时有效）
  float m_l=-1, m_r=1, m_b=-1, m_t=1;
};

} // namespace render
```

```cpp
// core/render/RenderServer.h —— 命令队列（逻辑层唯一入口，所有权明确）
namespace render {

class RenderServer {
public:
  // 所有权：设备由外部拥有（引用），RenderServer 不删除；显式解除
  void setBackend(RenderDevice& b);
  void clearBackend();
  bool hasBackend() const;

  // ---- 每帧作用域 ----
  void beginFrame(const Camera& cam, float aspect);
  void endFrame();

  // ---- 命令收集（只存句柄+变换，不存数据）----
  void drawMesh(MeshHandle h, const math::Matrix4x4& model);

  // ---- 执行（命令池预分配复用，避免每帧 new）----
  void render();

private:
  struct DrawCmd { MeshHandle mesh; math::Matrix4x4 model; };
  RenderDevice* m_backend = nullptr;
  std::vector<DrawCmd> m_cmds;   // 复用，不清容量
  Camera m_cam;                  // 每帧 beginFrame 快照
  float m_aspect = 1.0f;
};

} // namespace render
```

### 3.3 软件光栅化要点（对应教学第 6 讲）

- `SoftwareDevice` 持 `std::vector<uint32_t> m_framebuffer` + `std::vector<float> m_depthBuffer`
- `drawMesh`：把顶点 MVP 变换 → 屏幕坐标，按教学 6.7 伪代码光栅化（重心坐标 + 深度测试）
- 线框模式：先画三角形边（Bresenham 直线），MVP 变换后连线
- **可测试性**：渲染已知三角形 → 断言帧缓冲像素颜色/深度

### 3.4 OpenGL 要点（对应教学第 5 部分）

- SDL2 建窗口 + GL 上下文（`SDL_GL_CreateContext`）
- glad 加载函数（或 SDL 自带的 OpenGL 头）
- 最小 shader：顶点 shader `uMVP * aPos`，片元 shader `uColor`
- VAO/VBO 上传顶点，`glUniformMatrix4fv(..., mvp.data())`（列主序直传）
- 线框：`glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)`

### 3.5 主循环（对应教学第 7 讲 + issue #17）

```cpp
// Engine::run()
while (running) {
  m_clock.update(steady_clock::now());   // 逻辑 tick（固定步长）
  while (m_clock.getTick() > m_lastTick) { updateLogic(); m_lastTick++; }
  m_renderServer.render();               // 渲染帧（vsync 驱动）
  handleInput();                         // SDL 事件
}
```

## 4. 支持的功能（MVP 范围）

| 功能            | 说明                                                      |
| --------------- | --------------------------------------------------------- |
| SDL2 窗口       | 可关窗、可调大小（aspect 自适应）                         |
| 清屏 + 线框网格 | 地面网格线，验证正交/透视可见差异                         |
| 正交/透视切换   | 按键切换，对比"工程图 vs 照片"                            |
| 相机控制        | 鼠标拖动轨道旋转（看立方体转），滚轮缩放（拉近 fov/距离） |
| 线框立方体      | 12 条边，MVP 变换 + 旋转动画                              |
| 多物体          | 几个立方体不同位置/大小（验证 M 矩阵）                    |
| 深度测试        | 立方体互相遮挡正确（软光栅断言 + OpenGL 默认开启）        |
| 后端切换        | 运行时按键切换 SoftwareDevice ↔ OpenGLDevice              |
| 逻辑 tick       | Clock 固定步长驱动旋转动画，渲染帧独立                    |

## 5. 验收规则

### 5.1 功能验收（手动）

- [ ] `render_demo` 启动出现窗口，显示网格 + 若干立方体
- [ ] 按 P 切换正交/透视，画面差异明显且正确
- [ ] 鼠标拖动相机绕场景旋转，无 NaN/抖动
- [ ] 滚轮缩放，近大远小正确
- [ ] 按 B 切换软光栅/OpenGL 后端，画面内容一致
- [ ] 关窗程序正常退出（无崩溃/泄漏）

### 5.2 测试验收（自动化，`test_render`）

- [ ] **软光栅确定性**：渲染一个已知三角形 → 断言帧缓冲特定像素颜色
- [ ] **深度测试**：前后两个三角形 → 断言近的遮挡远的（像素级）
- [ ] **SceneGraph**：父旋转 90° → 子世界坐标 = 数学预期（复用已测 `trs`）
- [ ] **SceneGraph 生命周期**：destroyNode 级联销毁子树；重建后句柄无效（isValid=false）
- [ ] **SceneGraph 缓存**：setLocal 置脏 → world() 重算；未变更 → 命中缓存（同一矩阵地址）
- [ ] **资源句柄**：createMesh 返回有效句柄 → destroyMesh 后绘制无效句柄应安全 no-op（无崩溃）
- [ ] **Camera 不变量**：fovY=0 / near≤0 / far≤near → 抛异常或拒绝（不产生除零 NaN）
- [ ] **Camera**：正交/透视投影边界点 → 裁剪坐标断言
- [ ] **MVP 组合**：已知 M/V/P → 顶点变换结果断言（对照手工计算）
- [ ] **命令池复用**：连续两帧 drawMesh → m_cmds 容量不增长（无每帧分配）
- [ ] **主循环**：注入式 Clock → 断言逻辑 tick 与渲染帧解耦

### 5.3 质量规则

- [ ] 所有数学复用 `core/math`，不新写矩阵/四元数逻辑
- [ ] 引擎层（core/render）不包含任何 SDL2/OpenGL 头（抽象在接口后；SDL/GL 只在 render_demo 与后端 .cpp）
- [ ] 分层：core/render 不依赖游戏层；render_demo 只做组装
- [ ] 所有权：`RenderServer` 持有后端引用（非拥有）；`SceneGraph` 是节点唯一所有者（无裸指针外泄）
- [ ] 所有接口传 `span`/句柄，无裸指针+长度分离（安全）
- [ ] 测试独立编译，遵循项目自写断言框架
- [ ] 无编译警告（-Wall -Wextra）

## 6. 里程碑（对应 todo）

| 里程碑 | 内容                                | 验收                       |
| ------ | ----------------------------------- | -------------------------- |
| M1     | RenderDevice 接口 + SDL2 空窗口     | 窗口出现、可关窗           |
| M2     | SoftwareDevice 画网格线             | 软光栅可见网格             |
| M3     | Camera + MVP 串联（正交/透视）      | 网格随相机变化正确         |
| M4     | 线框立方体 + 旋转 + 深度测试        | 多立方体遮挡正确           |
| M5     | OpenGLDevice 同样内容               | 双后端画面一致             |
| M6     | RenderServer + Engine 主循环 + 输入 | 完整可交互 demo + 测试全绿 |
