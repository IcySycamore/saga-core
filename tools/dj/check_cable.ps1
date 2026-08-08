# 从 CABLE Output 录音并报告 RMS，用于验证 relay 链路
# 用法: powershell -File check_cable.ps1 [秒数]
param([int]$Seconds = 2)

Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;

namespace CableCheck {
    [ComImport, Guid("BCDE0395-E52F-467C-8E3D-C4579291692E")] public class MMDeviceEnumerator { }

    [ComImport, Guid("A95664D2-9614-4F35-A746-DE8DB63617E6"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IMMDeviceEnumerator {
        [PreserveSig] int EnumAudioEndpoints(int dataFlow, int dwStateMask, out IMMDeviceCollection devices);
        [PreserveSig] int GetDefaultAudioEndpoint(int dataFlow, int role, out IMMDevice endpoint);
        [PreserveSig] int GetDevice(string pwstrId, out IMMDevice endpoint);
        [PreserveSig] int RegisterEndpointNotificationCallback(IntPtr client);
        [PreserveSig] int UnregisterEndpointNotificationCallback(IntPtr client);
    }

    [ComImport, Guid("0BD7A1BE-7A1A-44DB-8397-CC5392387B5E"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IMMDeviceCollection {
        [PreserveSig] int GetCount(out int count);
        [PreserveSig] int Item(int index, out IMMDevice device);
    }

    [ComImport, Guid("D666063F-1587-4E43-81F1-B948E807363F"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IMMDevice {
        [PreserveSig] int Activate(ref Guid iid, int dwClsCtx, IntPtr pActivationParams, out IntPtr pInterface);
        [PreserveSig] int OpenPropertyStore(int stgmAccess, out IPropertyStore properties);
        [PreserveSig] int GetId([MarshalAs(UnmanagedType.LPWStr)] out string id);
        [PreserveSig] int GetState(out int state);
    }

    [ComImport, Guid("886d8eeb-8cf2-4446-8d02-cdba1dbdcf99"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IPropertyStore {
        [PreserveSig] int GetCount(out int count);
        [PreserveSig] int GetAt(int index, out PropertyKey key);
        [PreserveSig] int GetValue(ref PropertyKey key, out PropVariant value);
        [PreserveSig] int SetValue(ref PropertyKey key, ref PropVariant value);
        [PreserveSig] int Commit();
    }

    [StructLayout(LayoutKind.Sequential)] public struct PropertyKey { public Guid fmtid; public int pid; }
    [StructLayout(LayoutKind.Sequential)] public struct PropVariant { public short vt; public short r1; public short r2; public short r3; public IntPtr value; }

    [ComImport, Guid("1CB9AD4C-DBFA-4C32-B178-C2F568A703B2"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IAudioClient {
        [PreserveSig] int Initialize(int shareMode, int streamFlags, long hnsBufferDuration, long hnsPeriodicity, IntPtr pFormat, IntPtr audioSessionGuid);
        [PreserveSig] int GetBufferSize(out uint bufferSize);
        [PreserveSig] int GetStreamLatency(out long latency);
        [PreserveSig] int GetCurrentPadding(out uint padding);
        [PreserveSig] int IsFormatSupported(int shareMode, IntPtr pFormat, out IntPtr closestMatch);
        [PreserveSig] int GetMixFormat(out IntPtr format);
        [PreserveSig] int GetDevicePeriod(out long defaultPeriod, out long minPeriod);
        [PreserveSig] int Start();
        [PreserveSig] int Stop();
        [PreserveSig] int Reset();
        [PreserveSig] int SetEventHandle(IntPtr handle);
        [PreserveSig] int GetService(ref Guid serviceId, out IntPtr service);
    }

    [ComImport, Guid("C8ADBD64-E71E-48A0-A4DE-185C395CD317"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IAudioCaptureClient {
        [PreserveSig] int GetBuffer(out IntPtr buffer, out uint frames, out int flags, out ulong devicePos, out ulong qpcPos);
        [PreserveSig] int ReleaseBuffer(uint frames);
        [PreserveSig] int GetNextPacketSize(out uint frames);
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct WaveFormatEx {
        public ushort wFormatTag;
        public ushort nChannels;
        public uint nSamplesPerSec;
        public uint nAvgBytesPerSec;
        public ushort nBlockAlign;
        public ushort wBitsPerSample;
        public ushort cbSize;
    }

    public static class Api {
        static string Name(IMMDevice d) {
            IPropertyStore s; d.OpenPropertyStore(0, out s);
            var k = new PropertyKey { fmtid = new Guid("a45c254e-df1c-4efd-8020-67d146a850e0"), pid = 14 };
            PropVariant pv; s.GetValue(ref k, out pv);
            return Marshal.PtrToStringUni(pv.value);
        }
        public static string FindCapture(string needle) {
            var e = (IMMDeviceEnumerator)(new MMDeviceEnumerator());
            IMMDeviceCollection c; e.EnumAudioEndpoints(1, 15, out c);
            int n; c.GetCount(out n);
            for (int i = 0; i < n; i++) {
                IMMDevice d; c.Item(i, out d);
                string name = Name(d);
                if (name != null && name.IndexOf(needle, StringComparison.OrdinalIgnoreCase) >= 0)
                    return name;
            }
            return null;
        }
        // 捕获并返回 RMS（0.0~1.0），找不到设备返回 -1
        public static double CaptureRMS(string needle, int seconds) {
            var e = (IMMDeviceEnumerator)(new MMDeviceEnumerator());
            IMMDeviceCollection c; e.EnumAudioEndpoints(1, 15, out c);
            int n; c.GetCount(out n);
            IMMDevice dev = null;
            for (int i = 0; i < n; i++) {
                IMMDevice d; c.Item(i, out d);
                string name = Name(d);
                if (name != null && name.IndexOf(needle, StringComparison.OrdinalIgnoreCase) >= 0) { dev = d; break; }
            }
            if (dev == null) return -1;

            var iidClient = new Guid("1CB9AD4C-DBFA-4C32-B178-C2F568A703B2");
            IntPtr pClient;
            int hr = dev.Activate(ref iidClient, 23, IntPtr.Zero, out pClient);
            if (hr != 0) return -2;
            var client = (IAudioClient)Marshal.GetObjectForIUnknown(pClient);

            IntPtr pFmt;
            hr = client.GetMixFormat(out pFmt);
            if (hr != 0) return -3;
            WaveFormatEx fmt = (WaveFormatEx)Marshal.PtrToStructure(pFmt, typeof(WaveFormatEx));

            hr = client.Initialize(0, 0, seconds * 10000L * 100L, 0, pFmt, IntPtr.Zero);
            if (hr != 0) return -4;

            var iidCap = new Guid("C8ADBD64-E71E-48A0-A4DE-185C395CD317");
            IntPtr pCap;
            hr = client.GetService(ref iidCap, out pCap);
            if (hr != 0) return -5;
            var cap = (IAudioCaptureClient)Marshal.GetObjectForIUnknown(pCap);

            client.Start();
            double sum = 0; long samples = 0;
            var sw = System.Diagnostics.Stopwatch.StartNew();
            bool isFloat = (fmt.wFormatTag == 3); // WAVE_FORMAT_IEEE_FLOAT
            while (sw.ElapsedMilliseconds < seconds * 1000L) {
                uint next; cap.GetNextPacketSize(out next);
                if (next > 0) {
                    IntPtr buf; uint frames; int flags; ulong dp, qp;
                    cap.GetBuffer(out buf, out frames, out flags, out dp, out qp);
                    for (uint i = 0; i < frames * fmt.nChannels; i++) {
                        double v;
                        if (isFloat) v = (double)Marshal.ReadSingle(buf, (int)i * 4);
                        else v = (double)Marshal.ReadInt16(buf, (int)i * 2) / 32768.0;
                        sum += v * v;
                    }
                    samples += frames * fmt.nChannels;
                    cap.ReleaseBuffer(frames);
                }
                System.Threading.Thread.Sleep(5);
            }
            client.Stop();
            if (samples == 0) return 0;
            return Math.Sqrt(sum / samples);
        }
    }
}
'@

$rms = [CableCheck.Api]::CaptureRMS("CABLE Output", $Seconds)
if ($rms -lt 0) { "ERROR: 无法打开 CABLE Output (code=$rms)" }
else { "CABLE Output RMS = {0:N5}  (0 = 静音，>0 = 有声)" -f $rms }
