#include "demo.hpp"

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>

#ifdef USING_AFTERMATH
#include <GFSDK_Aftermath_GpuCrashDump.h>
#include <fmt/core.h>
#include <fstream>

static void gpu_crash_dump_callback(const void* gpu_crash_dump, const uint32_t dump_size, void* user_data)
{
    fmt::print("gpu_crash_dump_callback\n");

    // Make sure only one thread at a time...
    std::mutex* mutex = reinterpret_cast<std::mutex*>(user_data);
    std::lock_guard<std::mutex> lock(*mutex);

    auto path = "dump.nv-gpudmp";
    std::ofstream file(path, std::ios::trunc | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
    file.write(reinterpret_cast<const char *>(gpu_crash_dump), dump_size);
    file.close();

}

static void shader_debug_info_callback(const void* shader_debug_info, const uint32_t info_size, void* user_data)
{
    fmt::print("shader_debug_info_callback\n");
    //GpuCrashTracker* pGpuCrashTracker = reinterpret_cast<GpuCrashTracker*>(pUserData);
    //pGpuCrashTracker->OnShaderDebugInfo(pShaderDebugInfo, shaderDebugInfoSize);
}

static void crash_dump_description_callback(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription add_description, void* user_data)
{
    fmt::print("crash_dump_description_callback\n");
    //GpuCrashTracker* pGpuCrashTracker = reinterpret_cast<GpuCrashTracker*>(pUserData);
    //pGpuCrashTracker->OnDescription(addDescription);
}
#endif

int main() {
#ifdef USING_AFTERMATH
    std::mutex mutex;
    GFSDK_Aftermath_EnableGpuCrashDumps(
        GFSDK_Aftermath_Version_API,
        GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_Vulkan,
        GFSDK_Aftermath_GpuCrashDumpFeatureFlags_Default,
        gpu_crash_dump_callback,                               // Register callback for GPU crash dumps.
        shader_debug_info_callback,                            // Register callback for shader debug information.
        crash_dump_description_callback,                       // Register callback for GPU crash dump description.
        &mutex);
#endif
    demo::Demo demo{};
    try {
        demo.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
