#pragma once
#ifdef USING_AFTERMATH
#include "vk_common.hpp"
#include <GFSDK_Aftermath_GpuCrashDumpDecoding.h>
#include <map>
#include <vector>
#include <fmt/core.h>

// Helper for comparing GFSDK_Aftermath_ShaderHash.
inline bool operator<(const GFSDK_Aftermath_ShaderHash& lhs, const GFSDK_Aftermath_ShaderHash& rhs)
{
    return lhs.hash < rhs.hash;
}

inline void check(GFSDK_Aftermath_Result result)
{
    if (!GFSDK_Aftermath_SUCCEED(result))
    {
        fmt::print("Aftermath Error {:#X}", result);
        exit(1);
    }
}

class Aftermath_database
{
public:
    void add_binary(std::vector<uint8_t> data);
    std::map<GFSDK_Aftermath_ShaderHash, std::vector<uint8_t>> binaries;
};

#endif