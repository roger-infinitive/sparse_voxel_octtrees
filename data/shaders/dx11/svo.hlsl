StructuredBuffer<uint> Mask       : register(t0);
StructuredBuffer<uint> FirstChild : register(t1);

RWTexture2D<float4> OutColor : register(u0);

[numthreads(8,8,1)]
void CS(uint3 tid : sv_dispatchthreadid) {
    uint m = Mask[0] & 0xFF;

    uint band = (tid.x * 8) / 1920; // TODO(roger): width is hardcoded.
    uint bit = 1u << band;

    float on = (m & bit) != 0  ? 1.0f : 0.1f;
    OutColor[tid.xy] = float4(on, on, on, 1);
}