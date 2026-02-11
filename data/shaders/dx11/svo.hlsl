StructuredBuffer<uint> Mask : register(t0);

RWTexture2D<float3> OutColor : register(u0);

[numthreads(8,8,1)]
void CS(uint3 tid : sv_dispatchthreadid) {
    OutColor[tid.xy] = float4(1, 0, 0, 1);
}