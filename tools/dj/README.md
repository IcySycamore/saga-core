# dj — 命令行音乐电台 / DJ 工具

把本地音乐播放到**广播设备**（VB-Cable 虚拟麦克风 / 立体声混音），让腾讯会议、微信语音等把音乐当麦克风传出去；可选同时送**监听设备**（如蓝牙耳机）。

```
dj → CABLE Input → CABLE Output(=虚拟麦克风) → 腾讯会议/微信
```

## 构建

```bash
cmake --build build/vscodeBuild --target dj
# 产物: build/vscodeBuild/dj.exe
```

依赖：仅 `miniaudio.h`（已随仓库附带，单头文件库），Windows 下链接 `ole32 oleaut32 winmm user32 shell32`。

## 用法

```bash
dj list-devices                       # 列出播放/录音设备
dj play <文件或目录...> [选项]         # 播放本地音乐
 dj <文件或目录...> [选项]              # 同上（可省略 play）
dj relay [选项]                       # 把系统声音转发到虚拟麦克风
```

### 选项

| 选项                   | 说明                                                                      |
| ---------------------- | ------------------------------------------------------------------------- |
| `--broadcast <设备名>` | 广播设备。默认自动选名称含 `CABLE` 的（VB-Cable），否则用系统默认播放设备 |
| `--monitor <设备名>`   | 监听设备（可选，如蓝牙耳机 `iKF-Nano`）                                   |
| `--volume <0-100>`     | 音量百分比（默认 90）                                                     |
| `--fade <秒>`          | 每首歌淡入时长（默认 2，防爆音）                                          |
| `--loop`               | 循环播放列表                                                              |

### 交互快捷键（播放中）

```
空格  播放/暂停     n   下一首       p   上一首
+ -   音量增减      l   循环开关     q   退出
```

## relay — 把系统声音当音源（无需本地文件）

把**立体声混音**（系统正在播放的一切声音：网页音乐/视频/游戏）实时转发到虚拟麦克风：

```bash
dj relay                    # 源=立体声混音，广播=CABLE Input
 dj relay --monitor "iKF"   # 同时送蓝牙耳机监听
 dj relay --source "立体声混音" --broadcast "CABLE"
```

快捷键：`空格`静音/恢复、`+/-`音量、`q`退出。

> 注意：立体声混音只捕获 **Realtek 声卡** 的播放输出。若想转发蓝牙/其他设备播放的声音，
> 需先把该声音切到 Realtek（或用 `dj play --broadcast 扬声器` 强制）。

## 完整配置（推荐：VB-Cable，完全静默）

1. 下载安装 [VB-Audio Virtual Cable](https://vb-audio.com/Cable/)（免费）
2. 装好后会出现 `CABLE Input`（播放）和 `CABLE Output`（录音）两个设备
3. 播放：`dj "你的歌单目录" --monitor "iKF"`（自动广播到 CABLE Input，静默不外放）
4. 腾讯会议/微信：麦克风选择 **`CABLE Output`**
5. 想监听就戴蓝牙耳机（`--monitor` 指定设备名）

> 系统默认播放设备无需改动：dj 用 WASAPI 直接指定输出设备，其他应用的声音照常走系统默认设备。

## 备选方案：立体声混音（无需安装软件）

系统声卡自带"立体声混音"（Realtek 等），但**只能捕获同一声卡的播放输出**，且无法完全静默：

- 需把默认播放设备设为 Realtek（笔记本喇叭/有线耳机），蓝牙耳机听不到
- 腾讯会议/微信麦克风选择"立体声混音"

适合临时用；长期 DJ 建议 VB-Cable。

## 已知限制

- 支持格式：mp3 / flac / wav / ogg
- 双设备（广播+监听）采样率统一为 48kHz，miniaudio 自动重采样，监听端可能有极小延迟
- 广播端（Realtek）若没有物理输出会无声——这是硬件限制，VB-Cable 无此问题
