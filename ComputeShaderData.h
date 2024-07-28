#pragma once
#define RootSig "RootConstants(num32BitConstants =2, b0), DescriptorTable(UAV(u0)), DescriptorTable(UAV(u1)),"\
                "StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_POINT, addressU=TEXTURE_ADDRESS_CLAMP, addressV"\
                        "=TEXTURE_ADDRESS_CLAMP, borderColor=STATIC_BORDER_COLOR_OPAQUE_BLACK )"

#ifdef __cplusplus
using uint = unsigned int;
#endif
struct Constants {
    uint r;
    uint it;
};