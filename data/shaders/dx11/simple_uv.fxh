// IMPORTANT(roger): we changed this to handle premultiplied input. we need to do the same for opengl.

cbuffer PerApplication : register(b0) {
    matrix projectionMatrix;
}

cbuffer PerFrame : register(b1) {
    matrix viewMatrix;
}

Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

struct VertexShaderInput
{
    float4 Pos : POSITION;
    float2 TexCoord : TEXCOORD0;
    float4 Tint : TINT; 
};

struct VertexShaderOutput
{
    float4 Pos : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    float4 Tint : TINT;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VertexShaderOutput VS( VertexShaderInput input )
{
    VertexShaderOutput output;

    matrix mvp = mul(projectionMatrix, viewMatrix);
    output.Pos = mul(mvp, float4(input.Pos.xy, 0, 1));
    output.TexCoord = input.TexCoord;
    
    // premultiply vertex color
    output.Tint.rgb = input.Tint.rgb * input.Tint.a; 
    output.Tint.a   = input.Tint.a; 
    
    return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS( VertexShaderOutput input ) : SV_Target
{
    float4 texel = Texture.Sample(Sampler, input.TexCoord);
    clip(texel.a - 0.01);
    return texel * input.Tint;
}