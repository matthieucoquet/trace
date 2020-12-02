#ifdef USING_AFTERMATH
#include "aftermath_database.hpp"

void Aftermath_database::add_binary(std::vector<uint8_t> data)
{
    const GFSDK_Aftermath_SpirvCode shader{ data.data(), uint32_t(data.size()) };
    GFSDK_Aftermath_ShaderHash hash;
    GFSDK_Aftermath_GetShaderHashSpirv(GFSDK_Aftermath_Version_API, &shader, &hash);
    binaries[hash].swap(data);
}

#endif