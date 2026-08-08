# 渲染：RenderDevice 接口 + 双后端 + 像素格式约定

引擎渲染子系统的核心架构决策（M3-M4 落地，M5-M6 引用）。对应实现：`core/render/`；详细推导见 `docs/design/rendering-architecture.md` 与 `docs/design/rendering-tutorial.md`。

## 决策

1. **接口命名与分层**：底层设备抽象命名为 `RenderDevice`（对应 Godot RenderingDevice / Unreal RHI），负责初始化、资源、绘制；上层命令层 `RenderServer`（M6 引入，Godot RenderingServer 风格）负责命令队列与设备调度。逻辑层不直接碰设备。
2. **纯接口，零 SDL 依赖**：`RenderDevice.h` 不包含任何 SDL3/OpenGL/平台头。SDL 只存在于后端实现（`SDL3Device.h`）与应用层（`render_demo.cpp`）。引擎层可独立编译测试，未来换平台/API 不动接口。
3. **强类型句柄**：资源句柄用独立结构体（`MeshHandle`：`value` + `kInvalid` + `operator bool` + `operator==`），不同资源类型不可混传（编译期防护）；平台窗口句柄用 `NativeWindowHandle<NativeT>` 模板 + 类型擦除（`NativeWindowHandle<void>`）进入接口。
4. **像素格式约定（RGBA 内存序）**：帧缓冲 `uint32_t` 按**内存序 RGBA** 打包 `(A<<24)|(B<<16)|(G<<8)|R`（R 最低字节），匹配 `SDL_PIXELFORMAT_RGBA32`。清屏色 `0xFF302018u`（深灰蓝 24,32,48）。**全链路（打包/插值/测试提取/BMP 导出）统一此约定**——字节序不匹配曾导致纯红背景事故。
5. **固定渲染分辨率 + letterbox 输出缩放**：软光栅帧缓冲固定 1280×720，不随窗口 resize 重建；窗口拉伸由显示层 letterbox（等比缩放 + 居中 + 背景填充）适配。好处：画面比例恒定、FPS 恒定（动画速度不随窗口大小变化）。
6. **近平面裁剪简化**：`transformVertex` 检测裁剪坐标 `cw≤0`（相机后方）标记 `invalid`，`drawMesh` 丢弃含无效顶点的图元。**不做真裁剪**（Sutherland-Hodgman 留作后续），代价：跨近平面的三角形被整体丢弃，画面边缘可能缺角。
7. **动画时间**：固定步长逻辑时钟（`clockns::Clock`）只驱动确定性模拟；渲染动画要求真实时间或保证 FPS 恒定（当前 demo 用固定分辨率保证 FPS 恒定）。固定步长时钟直接驱动动画会导致速度随 FPS 漂移。
8. **矩阵约定**：列主序（`m[col*4+row]`），`A*B` = 先应用 B 再应用 A；`MVP = P*V*M`。物体模型矩阵 `M = T·R`（先旋转后平移）。

## 理由（背景推导）

- 引擎层/游戏层分离（ADR-0001）延伸到渲染：渲染是未来通用引擎的一部分，必须可复用、可替换后端。
- 软光栅（CPU）先于 GPU：教学上把管线焊死，确定性可单测；`RenderDevice` 抽象让两种后端切换零成本。
- 字节序事故（陷阱 1）证明：像素格式是"字节级协议"，必须在 ADR 层定死。
- 固定分辨率事故（陷阱 6）：帧缓冲跟随窗口 → FPS 漂移 → 逻辑时钟驱动的动画速度随窗口大小变化；固定分辨率 + letterbox 一并解决变形、FPS、速度三问题。

## 边界（不做什么）

- 不做纹理/材质/光照（Vertex 目前只有 pos+color，未来扩展法线/UV）。
- 不引入 boost::signals2（ADR-0007 语义不变）。
- 不做真近平面裁剪、不做背面剔除（绕向未校准前禁开，靠深度测试）。
- 不做多窗口/多设备。
- 不做 GPU 资源生命周期托管（设备由外部拥有，接口不管理生命周期）。

## 状态

accepted（2026-08-09）
