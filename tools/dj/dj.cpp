// ============================================================================
// dj — 命令行音乐电台 / DJ 工具
//
// 功能:
//   - 播放本地音乐到"广播设备"（如 VB-Cable 的 CABLE Input / 立体声混音）
//   - 可选同时送"监听设备"（如蓝牙耳机）
//   - 交互快捷键: 播放/暂停、上下首、音量、循环
//
// 典型链路:  dj → CABLE Input → CABLE Output(=虚拟麦克风) → 腾讯会议/微信
//  或:       dj → Realtek 扬声器 → 立体声混音 → 腾讯会议/微信
// ============================================================================

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

#ifdef _WIN32
#include <conio.h>
#include <signal.h>
#include <windows.h>
#endif

namespace fs = std::filesystem;

static constexpr ma_uint32 kSampleRate = 48000;
static constexpr ma_uint32 kChannels = 2;

static std::atomic<bool> g_quit = false;
static void on_sigint(int) { g_quit.store(true); }

// ---------------- UTF-8 <-> UTF-16 ----------------
static std::wstring to_wide(const std::string &s) {
  if (s.empty())
    return L"";
  int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
  std::wstring w((size_t)n, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), n);
  return w;
}
static std::string to_utf8(const std::wstring &w) {
  if (w.empty())
    return "";
  int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0,
                              nullptr, nullptr);
  std::string s((size_t)n, '\0');
  WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), s.data(), n, nullptr,
                      nullptr);
  return s;
}

// ---------------- Player 状态 ----------------
struct Player {
  ma_context context{};
  ma_device broadcast{};
  ma_device monitor{};
  bool hasMonitor = false;

  std::vector<std::string> playlist; // UTF-8 路径
  size_t current = 0;
  bool loop = false;
  float fadeSeconds = 2.0f;

  std::mutex mtx; // 保护 decoder 与淡入状态
  ma_decoder *decoder = nullptr;

  std::atomic<bool> paused{false};
  std::atomic<float> volume{0.9f};
  std::atomic<bool> wantNext{false};
  std::atomic<bool> wantPrev{false};
  std::atomic<bool> endReached{false}; // 列表播完只提示一次

  ma_uint32 fadeInTotal = 0;
  ma_uint32 fadeInRemaining = 0;
};

// ---------------- 音频回调（实时线程，保持轻量） ----------------
static void data_callback(ma_device *pDevice, void *pOutput, const void *pInput,
                          ma_uint32 frameCount) {
  Player *pl = (Player *)pDevice->pUserData;
  float *out = (float *)pOutput;
  ma_uint32 total = frameCount * kChannels;

  std::lock_guard<std::mutex> lock(pl->mtx);

  if (pl->paused.load() || pl->decoder == nullptr) {
    std::memset(out, 0, total * sizeof(float));
    return;
  }

  ma_uint64 framesRead = 0;
  ma_result res =
      ma_decoder_read_pcm_frames(pl->decoder, out, frameCount, &framesRead);

  if (res == MA_AT_END) {
    std::memset(out, 0, total * sizeof(float));
    // 只有广播端推进歌单，且只提示一次
    if (pDevice == &pl->broadcast && !pl->endReached.load()) {
      pl->endReached.store(true);
      pl->wantNext.store(true);
    }
    return;
  }
  if (framesRead < frameCount) {
    std::memset(out + framesRead * kChannels, 0,
                (size_t)(frameCount - framesRead) * kChannels * sizeof(float));
  }

  // 音量 + 淡入（防爆音）
  const float vol = pl->volume.load();
  if (pl->fadeInRemaining > 0 && pl->fadeInTotal > 0) {
    for (ma_uint32 i = 0; i < frameCount; ++i) {
      float gain = vol;
      if (pl->fadeInRemaining > 0) {
        gain =
            vol * (1.0f - (float)pl->fadeInRemaining / (float)pl->fadeInTotal);
        --pl->fadeInRemaining;
      }
      out[i * kChannels] *= gain;
      out[i * kChannels + 1] *= gain;
    }
  } else if (vol != 1.0f) {
    for (ma_uint32 i = 0; i < total; ++i)
      out[i] *= vol;
  }
}

// ---------------- 设备查找 ----------------
static bool device_matches(const char *devName, const std::string &needle) {
  std::string name(devName);
  auto it = std::search(name.begin(), name.end(), needle.begin(), needle.end(),
                        [](char a, char b) {
                          return std::tolower((unsigned char)a) ==
                                 std::tolower((unsigned char)b);
                        });
  return it != name.end();
}

// 按名称（子串、大小写不敏感）查找播放设备 ID
static bool find_playback_device(ma_context *ctx, const std::string &name,
                                 ma_device_id *outId) {
  ma_device_info *infos = nullptr;
  ma_uint32 count = 0;
  if (ma_context_get_devices(ctx, &infos, &count, nullptr, nullptr) !=
      MA_SUCCESS)
    return false;
  for (ma_uint32 i = 0; i < count; ++i) {
    if (device_matches(infos[i].name, name)) {
      *outId = infos[i].id;
      return true;
    }
  }
  return false;
}

// 自动选择广播设备: 优先 VB-Cable 的 "CABLE Input"，否则用系统默认
static std::string auto_broadcast_name(ma_context *ctx) {
  ma_device_info *infos = nullptr;
  ma_uint32 count = 0;
  if (ma_context_get_devices(ctx, &infos, &count, nullptr, nullptr) !=
      MA_SUCCESS)
    return "";
  for (ma_uint32 i = 0; i < count; ++i) {
    if (device_matches(infos[i].name, "CABLE"))
      return "CABLE";
  }
  return "";
}

static ma_result init_playback_device(ma_context *ctx, const std::string &name,
                                      bool wantDefaultIfMissing, Player *pl,
                                      ma_device *dev) {
  ma_device_config cfg = ma_device_config_init(ma_device_type_playback);
  cfg.playback.format = ma_format_f32;
  cfg.playback.channels = kChannels;
  cfg.sampleRate = kSampleRate;
  cfg.dataCallback = data_callback;
  cfg.pUserData = pl;

  if (!name.empty()) {
    ma_device_id id;
    if (find_playback_device(ctx, name, &id)) {
      cfg.playback.pDeviceID = &id;
    } else if (!wantDefaultIfMissing) {
      return MA_NO_DEVICE;
    }
  }
  return ma_device_init(ctx, &cfg, dev);
}

// ---------------- relay（系统声音 → 虚拟麦克风 转发） ----------------
static void print_relay_help(); // 前向声明，定义见子命令区

// 按名称查找录音设备 ID
static bool find_capture_device(ma_context *ctx, const std::string &name,
                                ma_device_id *outId) {
  ma_device_info *infos = nullptr;
  ma_uint32 count = 0;
  if (ma_context_get_devices(ctx, nullptr, nullptr, &infos, &count) !=
      MA_SUCCESS)
    return false;
  for (ma_uint32 i = 0; i < count; ++i) {
    if (device_matches(infos[i].name, name)) {
      *outId = infos[i].id;
      return true;
    }
  }
  return false;
}

// 自动选择音频源: 优先“立体声混音”
static std::string auto_capture_name(ma_context *ctx) {
  static const char *candidates[] = {"立体声混音", "Stereo Mix", "混音"};
  ma_device_info *infos = nullptr;
  ma_uint32 count = 0;
  if (ma_context_get_devices(ctx, nullptr, nullptr, &infos, &count) !=
      MA_SUCCESS)
    return "";
  for (ma_uint32 i = 0; i < count; ++i) {
    for (auto cand : candidates) {
      if (device_matches(infos[i].name, cand))
        return cand;
    }
  }
  return "";
}

// Windows 10 Build 20348+ 才支持进程级 loopback（排除自身防回声）
static bool os_supports_process_loopback() {
  struct OsVersionInfo {
    ULONG dwOSVersionInfoSize;
    ULONG dwMajorVersion;
    ULONG dwMinorVersion;
    ULONG dwBuildNumber;
    ULONG dwPlatformId;
    WCHAR szCSDVersion[128];
  };
  HMODULE h = GetModuleHandleW(L"ntdll.dll");
  if (!h)
    return false;
  using Fn = LONG(WINAPI *)(OsVersionInfo *);
  Fn fn = (Fn)GetProcAddress(h, "RtlGetVersion");
  if (!fn)
    return false;
  OsVersionInfo vi{};
  vi.dwOSVersionInfoSize = sizeof(vi);
  if (fn(&vi) != 0)
    return false;
  return vi.dwBuildNumber >= 20348;
}

// SPSC 环形缓冲（单生产者单消费者，帧为单位，容量=帧数+1 以区分空/满）
class RelayBuffer {
public:
  void init(size_t frameCount) {
    capacity = frameCount + 1;
    data.assign(capacity * kChannels, 0.0f);
    readPos.store(0);
    writePos.store(0);
  }

  // 写入最多 frameCount 帧（满则丢弃新帧），返回实际写入帧数
  size_t write(const float *frames, size_t frameCount) {
    const size_t w = writePos.load(std::memory_order_relaxed);
    const size_t r = readPos.load(std::memory_order_acquire);
    const size_t space = (r + capacity - 1 - w) % capacity;
    const size_t n = std::min(space, frameCount);
    for (size_t i = 0; i < n; ++i) {
      const size_t idx = (w + i) % capacity;
      std::memcpy(&data[idx * kChannels], frames + i * kChannels,
                  kChannels * sizeof(float));
    }
    writePos.store((w + n) % capacity, std::memory_order_release);
    return n;
  }

  // 带增益写入（同时处理静音），满则丢弃新帧
  size_t write_gain(const float *frames, size_t frameCount, float gain) {
    const size_t w = writePos.load(std::memory_order_relaxed);
    const size_t r = readPos.load(std::memory_order_acquire);
    const size_t space = (r + capacity - 1 - w) % capacity;
    const size_t n = std::min(space, frameCount);
    if (gain == 1.0f) {
      for (size_t i = 0; i < n; ++i) {
        const size_t idx = (w + i) % capacity;
        std::memcpy(&data[idx * kChannels], frames + i * kChannels,
                    kChannels * sizeof(float));
      }
    } else {
      for (size_t i = 0; i < n; ++i) {
        const size_t idx = (w + i) % capacity;
        data[idx * kChannels] = frames[i * kChannels] * gain;
        data[idx * kChannels + 1] = frames[i * kChannels + 1] * gain;
      }
    }
    writePos.store((w + n) % capacity, std::memory_order_release);
    return n;
  }

  // 读取最多 frameCount 帧，返回实际读取帧数
  size_t read(float *frames, size_t frameCount) {
    const size_t r = readPos.load(std::memory_order_relaxed);
    const size_t w = writePos.load(std::memory_order_acquire);
    const size_t avail = (w + capacity - r) % capacity;
    const size_t n = std::min(avail, frameCount);
    for (size_t i = 0; i < n; ++i) {
      const size_t idx = (r + i) % capacity;
      std::memcpy(frames + i * kChannels, &data[idx * kChannels],
                  kChannels * sizeof(float));
    }
    readPos.store((r + n) % capacity, std::memory_order_release);
    return n;
  }

private:
  std::vector<float> data;
  std::atomic<size_t> readPos{0};
  std::atomic<size_t> writePos{0};
  size_t capacity = 0;
};

struct Relay {
  ma_context context{};
  ma_device source{};    // 音频源（loopback 或 capture）
  ma_device broadcast{}; // 播放: 广播（CABLE）
  ma_device monitor{};   // 播放: 监听（可选）
  bool hasMonitor = false;
  bool useLoopback = false; // true=捕获系统默认播放; false=捕获指定录音设备

  RelayBuffer rbBroadcast; // source → 广播
  RelayBuffer rbMonitor;   // source → 监听（可选）

  std::atomic<float> volume{0.9f};
  std::atomic<bool> muted{false};
};

// 源回调（loopback/capture）: 应用音量/静音 → 写入广播与监听缓冲
static void source_callback(ma_device *pDevice, void *pOutput,
                            const void *pInput, ma_uint32 frameCount) {
  Relay *r = (Relay *)pDevice->pUserData;
  if (pInput == nullptr)
    return; // 无数据
  const float *in = (const float *)pInput;
  const float gain = r->muted.load() ? 0.0f : r->volume.load();
  r->rbBroadcast.write_gain(in, frameCount, gain);
  if (r->hasMonitor)
    r->rbMonitor.write_gain(in, frameCount, gain);
}

// 播放回调: 从环形缓冲读，不足补静音
static void relay_playback_callback(ma_device *pDevice, void *pOutput,
                                    const void *pInput, ma_uint32 frameCount,
                                    RelayBuffer *rb) {
  float *out = (float *)pOutput;
  ma_uint32 framesRead = (ma_uint32)rb->read(out, frameCount);
  if (framesRead < frameCount)
    memset(out + framesRead * kChannels, 0,
           (size_t)(frameCount - framesRead) * kChannels * sizeof(float));
}

static void broadcast_callback(ma_device *pDevice, void *pOutput,
                               const void *pInput, ma_uint32 frameCount) {
  Relay *r = (Relay *)pDevice->pUserData;
  relay_playback_callback(pDevice, pOutput, pInput, frameCount,
                          &r->rbBroadcast);
}

// 监听设备回调: 从缓冲读
static void monitor_callback(ma_device *pDevice, void *pOutput,
                             const void *pInput, ma_uint32 frameCount) {
  Relay *r = (Relay *)pDevice->pUserData;
  relay_playback_callback(pDevice, pOutput, pInput, frameCount, &r->rbMonitor);
}

static int cmd_relay(std::vector<std::string> &args) {
  Relay r;
  std::string sourceName, broadcastName, monitorName;
  float volume = 0.9f;

  for (size_t i = 1; i < args.size(); ++i) {
    const std::string &a = args[i];
    if (a == "--source" && i + 1 < args.size())
      sourceName = args[++i];
    else if (a == "--broadcast" && i + 1 < args.size())
      broadcastName = args[++i];
    else if (a == "--monitor" && i + 1 < args.size())
      monitorName = args[++i];
    else if (a == "--volume" && i + 1 < args.size())
      volume = (float)std::atof(args[++i].c_str()) / 100.0f;
    else if (a == "-h" || a == "--help") {
      print_relay_help();
      return 0;
    }
  }

  if (ma_context_init(nullptr, 0, nullptr, &r.context) != MA_SUCCESS) {
    printf("错误: 无法初始化音频上下文\n");
    return 1;
  }

  // 默认: 捕获系统默认播放设备（loopback，任何声卡都行）
  r.useLoopback = sourceName.empty();
  if (sourceName.empty())
    sourceName = auto_capture_name(&r.context);
  if (broadcastName.empty())
    broadcastName = auto_broadcast_name(&r.context);

  r.rbBroadcast.init(kSampleRate);
  r.rbMonitor.init(kSampleRate);
  r.volume.store(std::max(0.0f, std::min(1.0f, volume)));

  ma_device_id srcId, bcastId;
  bool srcFound = false, bcastFound = false;
  if (!broadcastName.empty())
    bcastFound = find_playback_device(&r.context, broadcastName, &bcastId);
  if (!r.useLoopback && !sourceName.empty())
    srcFound = find_capture_device(&r.context, sourceName, &srcId);

  // 源设备（loopback 或 capture）
  ma_device_config scfg = ma_device_config_init(
      r.useLoopback ? ma_device_type_loopback : ma_device_type_capture);
  scfg.sampleRate = kSampleRate;
  scfg.capture.format = ma_format_f32;
  scfg.capture.channels = kChannels;
  scfg.dataCallback = source_callback;
  scfg.pUserData = &r;
  if (srcFound)
    scfg.capture.pDeviceID = &srcId;

  ma_result res;
  if (r.useLoopback && os_supports_process_loopback()) {
    // 优先: 进程级 loopback（排除本进程，防监听回声）。失败则回退纯 loopback。
    scfg.wasapi.loopbackProcessID = (ma_uint32)GetCurrentProcessId();
    scfg.wasapi.loopbackProcessExclude = true;
    res = ma_device_init(&r.context, &scfg, &r.source);
    if (res != MA_SUCCESS) {
      printf("提示: 进程级 loopback 不可用(%s)，回退为捕获所有进程。\n",
             ma_result_description(res));
      if (!monitorName.empty())
        printf(
            "      注意: 若监听设备与系统默认播放设备相同，可能产生回声。\n");
      scfg.wasapi.loopbackProcessID = 0;
      scfg.wasapi.loopbackProcessExclude = false;
      res = ma_device_init(&r.context, &scfg, &r.source);
    }
  } else {
    res = ma_device_init(&r.context, &scfg, &r.source);
  }
  if (res != MA_SUCCESS) {
    printf("错误: 无法打开音频源: %s\n", ma_result_description(res));
    ma_context_uninit(&r.context);
    return 1;
  }

  // 广播设备（CABLE）
  ma_device_config bcfg = ma_device_config_init(ma_device_type_playback);
  bcfg.sampleRate = kSampleRate;
  bcfg.playback.format = ma_format_f32;
  bcfg.playback.channels = kChannels;
  bcfg.dataCallback = broadcast_callback;
  bcfg.pUserData = &r;
  if (bcastFound)
    bcfg.playback.pDeviceID = &bcastId;
  res = ma_device_init(&r.context, &bcfg, &r.broadcast);
  if (res != MA_SUCCESS) {
    printf("错误: 无法打开广播设备: %s\n", ma_result_description(res));
    ma_device_uninit(&r.source);
    ma_context_uninit(&r.context);
    return 1;
  }

  if (!monitorName.empty()) {
    ma_device_config mcfg = ma_device_config_init(ma_device_type_playback);
    mcfg.playback.format = ma_format_f32;
    mcfg.playback.channels = kChannels;
    mcfg.sampleRate = kSampleRate;
    mcfg.dataCallback = monitor_callback;
    mcfg.pUserData = &r;
    ma_device_id mid;
    if (find_playback_device(&r.context, monitorName, &mid)) {
      mcfg.playback.pDeviceID = &mid;
      if (ma_device_init(&r.context, &mcfg, &r.monitor) == MA_SUCCESS)
        r.hasMonitor = true;
      else
        printf("警告: 监听设备 %s 初始化失败，已忽略\n", monitorName.c_str());
    } else {
      printf("警告: 监听设备 %s 未找到，已忽略\n", monitorName.c_str());
    }
  }

  res = ma_device_start(&r.source);
  if (res != MA_SUCCESS) {
    printf("错误: 无法启动音频源: %s\n", ma_result_description(res));
    ma_device_uninit(&r.source);
    ma_device_uninit(&r.broadcast);
    if (r.hasMonitor)
      ma_device_uninit(&r.monitor);
    ma_context_uninit(&r.context);
    return 1;
  }
  res = ma_device_start(&r.broadcast);
  if (res != MA_SUCCESS) {
    printf("错误: 无法启动广播设备: %s\n", ma_result_description(res));
    return 1;
  }
  if (r.hasMonitor) {
    res = ma_device_start(&r.monitor);
    if (res != MA_SUCCESS)
      printf("警告: 监听设备启动失败: %s\n", ma_result_description(res));
  }

  printf("音频源: %s\n",
         r.useLoopback ? "<系统默认播放设备, loopback>"
                       : (srcFound ? sourceName.c_str() : "<系统默认录音>"));
  printf("广播设备: %s\n",
         bcastFound ? broadcastName.c_str() : "<系统默认播放>");
  printf("监听设备: %s\n", r.hasMonitor ? monitorName.c_str() : "<无>");
  printf("音量: %d%%\n", (int)(r.volume.load() * 100));
  printf("\nrelay 运行中: [空格]静音/恢复  [+/-]音量  [q]退出\n");

  while (!g_quit.load()) {
#ifdef _WIN32
    while (_kbhit()) {
      int c = _getch();
      if (c == ' ') {
        bool was = r.muted.load();
        r.muted.store(!was);
        printf("%s\n", was ? "▶ 恢复" : "⏸ 静音");
      } else if (c == '+' || c == '=') {
        float v = std::min(1.0f, r.volume.load() + 0.05f);
        r.volume.store(v);
        printf("音量: %d%%\n", (int)(v * 100));
      } else if (c == '-' || c == '_') {
        float v = std::max(0.0f, r.volume.load() - 0.05f);
        r.volume.store(v);
        printf("音量: %d%%\n", (int)(v * 100));
      } else if (c == 'q' || c == 'Q' || c == 27) {
        g_quit.store(true);
      }
    }
#endif
    ma_sleep(20);
  }

  ma_device_uninit(&r.source);
  ma_device_uninit(&r.broadcast);
  if (r.hasMonitor)
    ma_device_uninit(&r.monitor);
  ma_context_uninit(&r.context);
  printf("已退出\n");
  return 0;
}

// ---------------- 播放列表 ----------------
static bool is_audio_file(const fs::path &p) {
  std::string ext = p.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(),
                 [](char c) { return std::tolower((unsigned char)c); });
  return ext == ".mp3" || ext == ".flac" || ext == ".wav" || ext == ".ogg";
}

static void collect_paths(const std::string &arg,
                          std::vector<std::string> &out) {
  fs::path p(to_wide(arg));
  std::error_code ec;
  if (fs::is_directory(p, ec)) {
    for (auto &e : fs::recursive_directory_iterator(
             p, fs::directory_options::skip_permission_denied, ec)) {
      if (e.is_regular_file(ec) && is_audio_file(e.path()))
        out.push_back(to_utf8(e.path().wstring()));
    }
  } else if (fs::is_regular_file(p, ec)) {
    out.push_back(arg);
  } else {
    printf("  忽略不存在的路径: %s\n", arg.c_str());
  }
}

static std::string track_name(const std::string &path) {
  fs::path p(to_wide(path));
  return to_utf8(p.filename().wstring());
}

// ---------------- 切歌 ----------------
static bool load_track(Player *pl, size_t idx) {
  std::lock_guard<std::mutex> lock(pl->mtx);

  if (pl->decoder) {
    ma_decoder_uninit(pl->decoder);
    delete pl->decoder;
    pl->decoder = nullptr;
  }
  if (idx >= pl->playlist.size())
    return false;

  auto *dec = new ma_decoder;
  ma_decoder_config dcfg =
      ma_decoder_config_init(ma_format_f32, kChannels, kSampleRate);
  ma_result res =
      ma_decoder_init_file_w(to_wide(pl->playlist[idx]).c_str(), &dcfg, dec);
  if (res != MA_SUCCESS) {
    printf("  无法解码: %s (%s)\n", pl->playlist[idx].c_str(),
           ma_result_description(res));
    delete dec;
    return false;
  }
  pl->decoder = dec;
  pl->current = idx;
  pl->endReached.store(false);
  pl->fadeInTotal = (ma_uint32)(pl->fadeSeconds * kSampleRate);
  pl->fadeInRemaining = pl->fadeInTotal;

  printf("[%zu/%zu] ▶ %s\n", pl->current + 1, pl->playlist.size(),
         track_name(pl->playlist[idx]).c_str());
  return true;
}

static void next_track(Player *pl) {
  if (pl->playlist.empty())
    return;
  size_t idx = pl->current + 1;
  if (idx >= pl->playlist.size()) {
    if (pl->loop) {
      idx = 0;
    } else {
      printf("  —— 播放列表已播完 (--loop 可循环) ——\n");
      return;
    }
  }
  pl->endReached.store(false);
  load_track(pl, idx);
}
static void prev_track(Player *pl) {
  if (pl->playlist.empty())
    return;
  size_t idx = (pl->current == 0) ? 0 : pl->current - 1;
  pl->endReached.store(false);
  load_track(pl, idx);
}

// ---------------- 主交互循环 ----------------
static void run_interactive(Player *pl) {
  printf("\n运行中。快捷键: [空格]播放/暂停  [n]下一首  [p]上一首  "
         "[+]音量  [-]音量  [l]循环  [q]退出\n");
  while (!g_quit.load()) {
    if (pl->wantNext.exchange(false))
      next_track(pl);
    if (pl->wantPrev.exchange(false))
      prev_track(pl);

#ifdef _WIN32
    while (_kbhit()) {
      int c = _getch();
      if (c == ' ') {
        bool was = pl->paused.load();
        pl->paused.store(!was);
        printf("%s\n", was ? "▶ 继续" : "⏸ 暂停");
      } else if (c == 'n' || c == 'N') {
        next_track(pl);
      } else if (c == 'p' || c == 'P') {
        prev_track(pl);
      } else if (c == '+' || c == '=') {
        float v = std::min(1.0f, pl->volume.load() + 0.05f);
        pl->volume.store(v);
        printf("音量: %d%%\n", (int)(v * 100));
      } else if (c == '-' || c == '_') {
        float v = std::max(0.0f, pl->volume.load() - 0.05f);
        pl->volume.store(v);
        printf("音量: %d%%\n", (int)(v * 100));
      } else if (c == 'l' || c == 'L') {
        pl->loop = !pl->loop;
        printf("循环: %s\n", pl->loop ? "开" : "关");
      } else if (c == 'q' || c == 'Q' || c == 27) {
        g_quit.store(true);
      }
    }
#endif
    ma_sleep(20);
  }
}

// ---------------- 子命令 ----------------
static void print_usage() {
  printf("dj — 命令行音乐电台工具（播放音乐 → 虚拟麦克风 → 会议/微信）\n"
         "\n"
         "用法:\n"
         "  dj <命令> [参数...]\n"
         "\n"
         "命令:\n"
         "  play          播放音乐（默认命令，可省略）\n"
         "  relay         把系统声音转发到虚拟麦克风（音频中继）\n"
         "  list-devices  列出所有音频设备\n"
         "  help          显示帮助（dj help <命令> 查看子命令详情）\n"
         "\n"
         "通用选项:\n"
         "  -h, --help    显示帮助\n"
         "\n"
         "运行 'dj help <命令>' 查看子命令详细用法。\n");
}

static void print_play_help() {
  printf("dj play — 播放音乐到虚拟麦克风\n"
         "\n"
         "用法:\n"
         "  dj play <文件或目录...> [选项]\n"
         "  dj <文件或目录...> [选项]        （play 可省略）\n"
         "\n"
         "说明:\n"
         "  播放本地音乐到广播设备（默认选 VB-Cable 的 CABLE Input，\n"
         "  无则用系统默认播放设备），可选同时送监听设备。\n"
         "  支持目录递归收集 mp3/flac/wav/ogg。\n"
         "\n"
         "选项:\n"
         "  --broadcast <设备名>   广播设备（名称子串匹配，大小写不敏感）\n"
         "  --monitor <设备名>     监听设备（可选，如 iKF）\n"
         "  --volume <0-100>       音量百分比（默认 90）\n"
         "  --fade <秒>            每首歌淡入时长（默认 2，防爆音）\n"
         "  --loop                 循环播放列表\n"
         "  -h, --help             显示本帮助\n"
         "\n"
         "播放中交互快捷键:\n"
         "  空格  播放/暂停    n   下一首      p   上一首\n"
         "  + -   音量增减      l   循环开关    q   退出\n"
         "\n"
         "示例:\n"
         "  dj \"D:\\音乐\"\n"
         "  dj play \"song.mp3\" --monitor \"iKF\" --loop\n"
         "  dj \"歌单\" --broadcast \"扬声器\" --volume 100\n");
}

static void print_list_devices_help() {
  printf("dj list-devices — 列出所有音频设备\n"
         "\n"
         "用法:\n"
         "  dj list-devices\n"
         "\n"
         "说明:\n"
         "  列出系统所有播放设备（广播/监听目标）和录音设备\n"
         "  （会议/微信的麦克风可选）。\n"
         "  播放设备含 CABLE Input、蓝牙耳机等；\n"
         "  录音设备含 CABLE Output、立体声混音等。\n"
         "\n"
         "示例:\n"
         "  dj list-devices\n");
}

static void print_relay_help() {
  printf(
      "dj relay — 把系统声音转发到虚拟麦克风\n"
      "\n"
      "用法:\n"
      "  dj relay [选项]\n"
      "\n"
      "说明:\n"
      "  把\"音频源\"（默认立体声混音，即系统正在播放的一切声音）\n"
      "  实时转发到广播设备（默认 CABLE Input → CABLE Output 虚拟麦克风），\n"
      "  可选同时送监听设备（蓝牙耳机）。\n"
      "  适合: 把网页音乐/视频/游戏声音当麦克风传给会议/微信，无需本地文件。\n"
      "\n"
      "选项:\n"
      "  --source <录音设备名>   音频源（默认自动选立体声混音）\n"
      "  --broadcast <播放设备名> 广播设备（默认自动选 CABLE Input）\n"
      "  --monitor <播放设备名>   监听设备（可选，如 iKF）\n"
      "  --volume <0-100>         音量百分比（默认 90）\n"
      "  -h, --help               显示本帮助\n"
      "\n"
      "交互快捷键:\n"
      "  空格  静音/恢复    + -  音量增减    q  退出\n"
      "\n"
      "示例:\n"
      "  dj relay\n"
      "  dj relay --source \"立体声混音\" --monitor \"iKF\"\n"
      "  dj relay --broadcast \"扬声器\" --volume 100\n");
}

static void print_help_for(const std::string &topic) {
  if (topic == "play")
    print_play_help();
  else if (topic == "relay")
    print_relay_help();
  else if (topic == "list-devices")
    print_list_devices_help();
  else {
    printf("未知命令: %s\n\n", topic.c_str());
    print_usage();
  }
}

static int cmd_list_devices() {
  ma_context ctx;
  if (ma_context_init(nullptr, 0, nullptr, &ctx) != MA_SUCCESS) {
    printf("错误: 无法初始化音频上下文\n");
    return 1;
  }
  ma_device_info *pinfos = nullptr;
  ma_uint32 pcount = 0;
  ma_device_info *cinfos = nullptr;
  ma_uint32 ccount = 0;
  if (ma_context_get_devices(&ctx, &pinfos, &pcount, &cinfos, &ccount) !=
      MA_SUCCESS) {
    printf("错误: 无法枚举音频设备\n");
    ma_context_uninit(&ctx);
    return 1;
  }
  printf("== 广播/监听设备 ==\n");
  for (ma_uint32 i = 0; i < pcount; ++i)
    printf("  [%u] %s\n", i, pinfos[i].name);
  printf("== 录音设备） ==\n");
  for (ma_uint32 i = 0; i < ccount; ++i)
    printf("  [%u] %s\n", i, cinfos[i].name);
  ma_context_uninit(&ctx);
  return 0;
}

static int cmd_play(std::vector<std::string> &args) {
  // args[0] 可能是 "play"（跳过）或直接是文件
  size_t start = 0;
  if (!args.empty() && args[0] == "play")
    start = 1;

  Player pl;
  std::string broadcastName, monitorName;
  float volume = 0.9f, fade = 2.0f;
  bool loop = false;
  std::vector<std::string> paths;

  for (size_t i = start; i < args.size(); ++i) {
    const std::string &a = args[i];
    if (a == "--broadcast" && i + 1 < args.size())
      broadcastName = args[++i];
    else if (a == "--monitor" && i + 1 < args.size())
      monitorName = args[++i];
    else if (a == "--volume" && i + 1 < args.size())
      volume = (float)std::atof(args[++i].c_str()) / 100.0f;
    else if (a == "--fade" && i + 1 < args.size())
      fade = (float)std::atof(args[++i].c_str());
    else if (a == "--loop")
      loop = true;
    else if (a == "-h" || a == "--help") {
      print_play_help();
      return 0;
    } else
      paths.push_back(a);
  }
  if (paths.empty()) {
    printf("错误: 未指定音乐文件/目录（dj -h 查看用法）\n");
    return 1;
  }

  std::vector<std::string> playlist;
  for (auto &p : paths)
    collect_paths(p, playlist);
  if (playlist.empty()) {
    printf("错误: 没有找到可播放的音频文件（支持 mp3/flac/wav/ogg）\n");
    return 1;
  }
  pl.playlist = std::move(playlist);
  pl.loop = loop;
  pl.fadeSeconds = fade;
  pl.volume.store(std::max(0.0f, std::min(1.0f, volume)));

  // ---- 音频上下文 ----
  if (ma_context_init(nullptr, 0, nullptr, &pl.context) != MA_SUCCESS) {
    printf("错误: 无法初始化音频上下文\n");
    return 1;
  }

  // ---- 广播设备 ----
  if (broadcastName.empty())
    broadcastName = auto_broadcast_name(&pl.context);
  ma_result res = init_playback_device(&pl.context, broadcastName, true, &pl,
                                       &pl.broadcast);
  if (res != MA_SUCCESS) {
    printf("错误: 无法打开广播设备(%s): %s\n",
           broadcastName.empty() ? "<默认>" : broadcastName.c_str(),
           ma_result_description(res));
    ma_context_uninit(&pl.context);
    return 1;
  }

  // ---- 监听设备（可选） ----
  if (!monitorName.empty()) {
    res =
        init_playback_device(&pl.context, monitorName, false, &pl, &pl.monitor);
    if (res == MA_SUCCESS)
      pl.hasMonitor = true;
    else
      printf("警告: 监听设备 %s 未找到，已忽略\n", monitorName.c_str());
  }

  if (!load_track(&pl, 0)) {
    printf("错误: 无法加载第一首歌\n");
    ma_device_uninit(&pl.broadcast);
    if (pl.hasMonitor)
      ma_device_uninit(&pl.monitor);
    ma_context_uninit(&pl.context);
    return 1;
  }

  res = ma_device_start(&pl.broadcast);
  if (res != MA_SUCCESS) {
    printf("错误: 无法启动广播设备: %s\n", ma_result_description(res));
    return 1;
  }
  if (pl.hasMonitor) {
    res = ma_device_start(&pl.monitor);
    if (res != MA_SUCCESS)
      printf("警告: 监听设备启动失败: %s\n", ma_result_description(res));
  }

  printf("广播设备: %s\n",
         broadcastName.empty() ? "<系统默认>" : broadcastName.c_str());
  printf("监听设备: %s\n", pl.hasMonitor ? monitorName.c_str() : "<无>");
  printf("音量: %d%%    循环: %s\n", (int)(pl.volume.load() * 100),
         loop ? "开" : "关");

  run_interactive(&pl);

  // ---- 清理 ----
  ma_device_uninit(&pl.broadcast);
  if (pl.hasMonitor)
    ma_device_uninit(&pl.monitor);
  if (pl.decoder) {
    ma_decoder_uninit(pl.decoder);
    delete pl.decoder;
  }
  ma_context_uninit(&pl.context);
  printf("已退出\n");
  return 0;
}

// ---------------- 入口 ----------------
int main() {
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
  signal(SIGINT, on_sigint);

  int wargc = 0;
  wchar_t **wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
  std::vector<std::string> args;
  if (wargv) {
    for (int i = 1; i < wargc; ++i)
      args.push_back(to_utf8(wargv[i]));
    LocalFree(wargv);
  }

  if (args.empty() || args[0] == "-h" || args[0] == "--help") {
    print_usage();
    return 0;
  }
  if (args[0] == "help") {
    if (args.size() > 1)
      print_help_for(args[1]);
    else
      print_usage();
    return 0;
  }
  if (args[0] == "list-devices") {
    // 支持 'dj list-devices -h'
    if (args.size() > 1 && (args[1] == "-h" || args[1] == "--help"))
      print_list_devices_help();
    else
      cmd_list_devices();
    return 0;
  }
  if (args[0] == "relay") {
    // 支持 'dj relay -h'
    if (args.size() > 1 && (args[1] == "-h" || args[1] == "--help"))
      print_relay_help();
    else
      cmd_relay(args);
    return 0;
  }

  return cmd_play(args);
}
