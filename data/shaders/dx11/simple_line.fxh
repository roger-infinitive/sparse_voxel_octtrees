cbuffer PerApplication : register( b0 ) {
    matrix projectionMatrix;
}

cbuffer PerFrame : register( b1 ) {
    matrix viewMatrix;
}

struct VertexShaderInput {
    float3 Pos : POSITION;
};

struct VertexShaderOutput {
    float4 Pos : SV_POSITION;
    float4 Color : TINT;
};

VertexShaderOutput VS(VertexShaderInput input) {
    VertexShaderOutput output;
    matrix mvp = mul(projectionMatrix, viewMatrix);
    output.Pos = mul(mvp, float4(input.Pos, 1));
    return output;
}

float4 PS(VertexShaderOutput input) : SV_Target {
    return float4(1, 0, 0, 1);
}