#pragma once
#ifdef USING_AFTERMATH
#include <mutex>
#include "aftermath_database.hpp"
#include <GFSDK_Aftermath_GpuCrashDump.h>

// Helper for comparing GFSDK_Aftermath_ShaderDebugInfoIdentifier.
inline bool operator<(const GFSDK_Aftermath_ShaderDebugInfoIdentifier& lhs, const GFSDK_Aftermath_ShaderDebugInfoIdentifier& rhs)
{
    if (lhs.id[0] == rhs.id[0])
    {
        return lhs.id[1] < rhs.id[1];
    }
    return lhs.id[0] < rhs.id[0];
}

class Aftermath_crash_tracker
{
public:
    Aftermath_database database{};

    Aftermath_crash_tracker();

    void gpu_crash_dump(const void* gpu_crash_dump, const uint32_t dump_size);
    void shader_debug_info(const void* shader_debug_info, const uint32_t info_size);
    void crash_description(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription add_description);
    void shader_lookup(const GFSDK_Aftermath_ShaderHash& shader_hash, PFN_GFSDK_Aftermath_SetData set_shader_binary) const;
    void shader_debug_info_lookup(const GFSDK_Aftermath_ShaderDebugInfoIdentifier& identifier, PFN_GFSDK_Aftermath_SetData set_shader_debug_info) const;
private:
    mutable std::mutex m_mutex;
    std::map<GFSDK_Aftermath_ShaderDebugInfoIdentifier, std::vector<uint8_t>> m_shader_debug_info;
};

#endif