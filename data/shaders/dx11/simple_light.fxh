cbuffer PerApplication : register( b0 ) {
    matrix projectionMatrix;
}

cbuffer PerFrame : register( b1 ) {
    matrix viewMatrix;
}

struct VertexShaderInput {
    float3 Pos : POSITION;
    float3 Normal : NORMAL;
};

struct VertexShaderOutput {
    float4 Pos : SV_POSITION;
    float3 Normal : NORMAL;
};

VertexShaderOutput VS(VertexShaderInput input) {
    VertexShaderOutput output;
    matrix mvp = mul(projectionMatrix, viewMatrix);
    output.Pos = mul(mvp, float4(input.Pos, 1));
    output.Normal = input.Normal;
    return output;
}

float4 PS(VertexShaderOutput input) : SV_Target {
    // float3 N = normalize(input.Normal);
    // float3 L = normalize(float3(0.3f, 0.8f, 0.2f));

    // float ndotl = saturate(dot(N, L));
    // float ambient = 0.45f;
    // float3 color = (ambient + (1.0f - ambient) * ndotl).xxx;

    // return float4(color, 1.0f);
    
    float3 n = normalize(input.Normal);
    float3 c = n * 0.5f + 0.5f;
    return float4(c, 1);
}