// Regression check for external/wgpu-native/patch/0001-dx12-footprint-rows-per-image.patch: a 3D
// buffer -> texture -> buffer round trip whose buffer pads each slice (rowsPerImage larger than the copy height).
// Without the patch the D3D12 backend steps between slices by the copy height and misplaces every slice after the
// first; the Vulkan backend was never affected.
//
// usage: wgpu_rows_per_image_3d <dx12|vulkan> [warp]
// exit code: 0 = data intact, 2 = mismatch, 1 = no adapter or device.
#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace
{
    constexpr uint32_t kWidth       = 5;
    constexpr uint32_t kHeight      = 3;
    constexpr uint32_t kDepth       = 4;
    constexpr uint32_t kRowBytes    = 256;
    constexpr uint32_t kRowsPadded  = 5; // 3 rows of texels and 2 of padding per slice
    constexpr uint64_t kBufferBytes = uint64_t(kRowBytes) * kRowsPadded * kDepth;

    WGPUAdapter g_adapter = nullptr;
    WGPUDevice  g_device  = nullptr;
    int         g_mapped  = 0;

    uint8_t TexelByte(uint32_t x, uint32_t y, uint32_t z, uint32_t b)
    {
        return static_cast<uint8_t>((x * 7 + y * 31 + z * 61 + b * 3) % 0x7B);
    }

    // Counts the rows of a read-back buffer laid out with `rows` rows per slice that do not hold the texels.
    int CountWrongRows(const uint8_t* data, uint32_t rows, const char* label)
    {
        int wrong = 0;
        for (uint32_t z = 0; z < kDepth; ++z)
            for (uint32_t y = 0; y < kHeight; ++y)
            {
                const uint8_t* row = data + (uint64_t(z) * rows + y) * kRowBytes;
                bool           ok  = true;
                for (uint32_t x = 0; x < kWidth; ++x)
                    for (uint32_t b = 0; b < 4; ++b)
                        ok = ok && row[x * 4 + b] == TexelByte(x, y, z, b);
                if (!ok)
                {
                    ++wrong;
                    std::printf("  %s: slice %u row %u is wrong\n", label, z, y);
                }
            }
        std::printf("%s (rowsPerImage %u): %d of %u rows wrong\n", label, rows, wrong, kDepth * kHeight);
        return wrong;
    }
} // namespace

int main(int argc, char** argv)
{
    const bool     dx12    = argc < 2 || std::strcmp(argv[1], "vulkan") != 0;
    const bool     warp    = argc > 2 && std::strcmp(argv[2], "warp") == 0;
    const uint32_t version = wgpuGetVersion();
    std::printf("wgpu-native %u.%u.%u.%u, backend %s%s\n",
                (version >> 24) & 255,
                (version >> 16) & 255,
                (version >> 8) & 255,
                version & 255,
                dx12 ? "dx12" : "vulkan",
                warp ? " (fallback adapter)" : "");

    WGPUInstanceExtras extras = {};
    extras.chain.sType        = static_cast<WGPUSType>(WGPUSType_InstanceExtras);
    extras.backends           = dx12 ? WGPUInstanceBackend_DX12 : WGPUInstanceBackend_Vulkan;
    WGPUInstanceDescriptor id = {};
    id.nextInChain            = &extras.chain;
    WGPUInstance instance     = wgpuCreateInstance(&id);

    WGPURequestAdapterCallbackInfo ac = {};
    ac.mode                           = WGPUCallbackMode_AllowProcessEvents;
    ac.callback = [](WGPURequestAdapterStatus, WGPUAdapter adapter, WGPUStringView, void*, void*) {
        g_adapter = adapter;
    };
    WGPURequestAdapterOptions ao = {};
    ao.forceFallbackAdapter      = warp;
    wgpuInstanceRequestAdapter(instance, &ao, ac);
    for (int i = 0; i < 1000 && !g_adapter; ++i)
        wgpuInstanceProcessEvents(instance);
    if (!g_adapter)
    {
        std::printf("no adapter\n");
        return 1;
    }
    WGPUAdapterInfo info = {};
    wgpuAdapterGetInfo(g_adapter, &info);
    std::printf("adapter %.*s\n", static_cast<int>(info.device.length), info.device.data);

    WGPURequestDeviceCallbackInfo dc = {};
    dc.mode                          = WGPUCallbackMode_AllowProcessEvents;
    dc.callback = [](WGPURequestDeviceStatus, WGPUDevice device, WGPUStringView, void*, void*) {
        g_device = device;
    };
    wgpuAdapterRequestDevice(g_adapter, nullptr, dc);
    for (int i = 0; i < 1000 && !g_device; ++i)
        wgpuInstanceProcessEvents(instance);
    if (!g_device)
    {
        std::printf("no device\n");
        return 1;
    }
    WGPUQueue queue = wgpuDeviceGetQueue(g_device);

    WGPUTextureDescriptor td = {};
    td.usage                 = WGPUTextureUsage_CopySrc | WGPUTextureUsage_CopyDst;
    td.dimension             = WGPUTextureDimension_3D;
    td.size                  = {kWidth, kHeight, kDepth};
    td.format                = WGPUTextureFormat_RGBA8Unorm;
    td.mipLevelCount         = 1;
    td.sampleCount           = 1;
    WGPUTexture texture      = wgpuDeviceCreateTexture(g_device, &td);

    // Source: each slice is 3 rows of distinct texel bytes followed by 2 rows of 0xEE padding.
    std::vector<uint8_t> source(kBufferBytes, 0xEE);
    for (uint32_t z = 0; z < kDepth; ++z)
        for (uint32_t y = 0; y < kHeight; ++y)
            for (uint32_t x = 0; x < kWidth; ++x)
                for (uint32_t b = 0; b < 4; ++b)
                    source[(uint64_t(z) * kRowsPadded + y) * kRowBytes + x * 4 + b] = TexelByte(x, y, z, b);
    WGPUBufferDescriptor ub = {};
    ub.usage                = WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst;
    ub.size                 = kBufferBytes;
    WGPUBuffer upload       = wgpuDeviceCreateBuffer(g_device, &ub);
    wgpuQueueWriteBuffer(queue, upload, 0, source.data(), kBufferBytes);

    WGPUBufferDescriptor rb = {};
    rb.usage                = WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;
    rb.size                 = kBufferBytes;
    WGPUBuffer padded       = wgpuDeviceCreateBuffer(g_device, &rb);
    WGPUBuffer tight        = wgpuDeviceCreateBuffer(g_device, &rb);

    // Upload with padded slices, then read back padded and tightly packed.
    WGPUCommandEncoder       encoder = wgpuDeviceCreateCommandEncoder(g_device, nullptr);
    WGPUTexelCopyBufferInfo  bi      = {};
    bi.buffer                        = upload;
    bi.layout.bytesPerRow            = kRowBytes;
    bi.layout.rowsPerImage           = kRowsPadded;
    WGPUTexelCopyTextureInfo ti      = {};
    ti.texture                       = texture;
    ti.aspect                        = WGPUTextureAspect_All;
    const WGPUExtent3D size          = {kWidth, kHeight, kDepth};
    wgpuCommandEncoderCopyBufferToTexture(encoder, &bi, &ti, &size);
    bi.buffer = padded;
    wgpuCommandEncoderCopyTextureToBuffer(encoder, &ti, &bi, &size);
    bi.buffer              = tight;
    bi.layout.rowsPerImage = kHeight;
    wgpuCommandEncoderCopyTextureToBuffer(encoder, &ti, &bi, &size);
    WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuQueueSubmit(queue, 1, &commands);

    WGPUBufferMapCallbackInfo mc = {};
    mc.mode                      = WGPUCallbackMode_AllowProcessEvents;
    mc.callback                  = [](WGPUMapAsyncStatus, WGPUStringView, void*, void*) { ++g_mapped; };
    wgpuBufferMapAsync(padded, WGPUMapMode_Read, 0, kBufferBytes, mc);
    wgpuBufferMapAsync(tight, WGPUMapMode_Read, 0, kBufferBytes, mc);
    for (int i = 0; i < 1000 && g_mapped < 2; ++i)
    {
        wgpuDevicePoll(g_device, true, nullptr);
        wgpuInstanceProcessEvents(instance);
    }
    if (g_mapped < 2)
    {
        std::printf("read-back buffers did not map\n");
        return 1;
    }
    const auto* paddedData = static_cast<const uint8_t*>(wgpuBufferGetConstMappedRange(padded, 0, kBufferBytes));
    const auto* tightData  = static_cast<const uint8_t*>(wgpuBufferGetConstMappedRange(tight, 0, kBufferBytes));
    const int   wrong      = CountWrongRows(paddedData, kRowsPadded, "padded read-back") +
                      CountWrongRows(tightData, kHeight, "tight read-back");
    std::printf("RESULT: %s\n", wrong ? "MISMATCH" : "OK");
    return wrong ? 2 : 0;
}
