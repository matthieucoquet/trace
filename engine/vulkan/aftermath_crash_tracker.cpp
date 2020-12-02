#ifdef USING_AFTERMATH
#include "aftermath_crash_tracker.hpp"
#include <GFSDK_Aftermath_GpuCrashDump.h>
#include <GFSDK_Aftermath_GpuCrashDumpDecoding.h>
#include <fmt/core.h>
#include <string>
#include <fstream>
#include <filesystem>

static void gpu_crash_dump_callback(const void* gpu_crash_dump, const uint32_t dump_size, void* user_data)
{
    auto* tracker = reinterpret_cast<Aftermath_crash_tracker*>(user_data);
    tracker->gpu_crash_dump(gpu_crash_dump, dump_size);
}

static void shader_debug_info_callback(const void* shader_debug_info, const uint32_t info_size, void* user_data)
{
    auto* tracker = reinterpret_cast<Aftermath_crash_tracker*>(user_data);
    tracker->shader_debug_info(shader_debug_info, info_size);
}

static void crash_dump_description_callback(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription add_description, void* user_data)
{
    auto* tracker = reinterpret_cast<Aftermath_crash_tracker*>(user_data);
    tracker->crash_description(add_description);
}

static void shader_lookup_callback(const GFSDK_Aftermath_ShaderHash* shader_hash, PFN_GFSDK_Aftermath_SetData set_shader_binary, void* user_data)
{
    auto* tracker = reinterpret_cast<Aftermath_crash_tracker*>(user_data);
    tracker->shader_lookup(*shader_hash, set_shader_binary);
}

void shader_debug_info_lookup_callback(const GFSDK_Aftermath_ShaderDebugInfoIdentifier* identifier, PFN_GFSDK_Aftermath_SetData set_shader_debug_info, void* user_data)
{
    auto* tracker = reinterpret_cast<Aftermath_crash_tracker*>(user_data);
    tracker->shader_debug_info_lookup(*identifier, set_shader_debug_info);
}

Aftermath_crash_tracker::Aftermath_crash_tracker()
{
    GFSDK_Aftermath_EnableGpuCrashDumps(
        GFSDK_Aftermath_Version_API,
        GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_Vulkan,
        GFSDK_Aftermath_GpuCrashDumpFeatureFlags_DeferDebugInfoCallbacks,
        gpu_crash_dump_callback,
        shader_debug_info_callback,
        crash_dump_description_callback,
        this);
    fmt::print("Aftermath initialized\n");
}

void Aftermath_crash_tracker::gpu_crash_dump(const void* gpu_crash_dump, const uint32_t dump_size)
{
    fmt::print("Gpu crash dump\n");
    std::lock_guard<std::mutex> lock(m_mutex);

    GFSDK_Aftermath_GpuCrashDump_Decoder decoder{};
    check(GFSDK_Aftermath_GpuCrashDump_CreateDecoder(GFSDK_Aftermath_Version_API, gpu_crash_dump, dump_size, &decoder));

    GFSDK_Aftermath_GpuCrashDump_BaseInfo base_info{};
    check(GFSDK_Aftermath_GpuCrashDump_GetBaseInfo(decoder, &base_info));

    uint32_t name_length = 0;
    check(GFSDK_Aftermath_GpuCrashDump_GetDescriptionSize(decoder, GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName, &name_length));

    std::string application_name(name_length, '\0');

    check(GFSDK_Aftermath_GpuCrashDump_GetDescription(decoder, GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName,
        uint32_t(application_name.size()),
        application_name.data()));
    application_name.pop_back();

    // Create a unique file name for writing the crash dump data to a file.
    // Note: due to an Nsight Aftermath bug (will be fixed in an upcoming
    // driver release) we may see redundant crash dumps. As a workaround,
    // attach a unique count to each generated file name.
    static int count = 0;
    const std::string file_name = fmt::format("{}-{}-{}.nv-gpudmp", application_name, base_info.pid, ++count);
    {
        std::ofstream file(file_name, std::ios::trunc | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }
        file.write(reinterpret_cast<const char*>(gpu_crash_dump), dump_size);
        file.close();
    }

    uint32_t json_size = 0;
    check(GFSDK_Aftermath_GpuCrashDump_GenerateJSON(
        decoder,
        GFSDK_Aftermath_GpuCrashDumpDecoderFlags_ALL_INFO,
        GFSDK_Aftermath_GpuCrashDumpFormatterFlags_NONE,
        shader_debug_info_lookup_callback,
        shader_lookup_callback,
        nullptr,
        nullptr,
        this,
        &json_size));
    std::vector<char> json(json_size);
    check(GFSDK_Aftermath_GpuCrashDump_GetJSON(decoder, uint32_t(json.size()), json.data()));

    const std::string json_filename = file_name + ".json";
    {
        std::ofstream file(json_filename, std::ios::trunc | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }
        file.write(json.data(), json.size());
        file.close();
    }
    GFSDK_Aftermath_GpuCrashDump_DestroyDecoder(decoder);
}

void Aftermath_crash_tracker::shader_debug_info(const void* shader_debug_info, const uint32_t info_size)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier = {};
    GFSDK_Aftermath_GetShaderDebugInfoIdentifier(GFSDK_Aftermath_Version_API, shader_debug_info, info_size, &identifier);

    std::vector<uint8_t> data((uint8_t*)shader_debug_info, (uint8_t*)shader_debug_info + info_size);
    m_shader_debug_info[identifier].swap(data);

    std::filesystem::path dir("shader_debug_info");
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directory(dir);
    }
    auto path = dir / fmt::format("shader-{:X}-{:X}.nvdbg", identifier.id[0], identifier.id[1]);
    std::ofstream file(path, std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    file.write(reinterpret_cast<const char*>(shader_debug_info), info_size);
    file.close();
}

void Aftermath_crash_tracker::crash_description(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription add_description)
{
    add_description(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName, "sdf_editor");
    add_description(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationVersion, "v0.1");
}

void Aftermath_crash_tracker::shader_lookup(const GFSDK_Aftermath_ShaderHash& shader_hash, PFN_GFSDK_Aftermath_SetData set_shader_binary) const
{
    std::vector<uint8_t> shader_binary;
    auto i_shader = database.binaries.find(shader_hash);
    if (i_shader == database.binaries.end()) {
        return;
    }
    set_shader_binary(i_shader->second.data(), uint32_t(i_shader->second.size()));
}

void Aftermath_crash_tracker::shader_debug_info_lookup(const GFSDK_Aftermath_ShaderDebugInfoIdentifier& identifier, PFN_GFSDK_Aftermath_SetData set_shader_debug_info) const
{
    auto i_debug_info = m_shader_debug_info.find(identifier);
    if (i_debug_info == m_shader_debug_info.end())
    {
        return;
    }
    set_shader_debug_info(i_debug_info->second.data(), uint32_t(i_debug_info->second.size()));
}

#endif