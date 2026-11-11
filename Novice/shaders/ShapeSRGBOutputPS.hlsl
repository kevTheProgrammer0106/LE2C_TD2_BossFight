#include "Shape.hlsli"

float3 ApplySRGBGamma(float3 linearColor)
{
    float3 low = 12.92f * linearColor;
    float3 high = 1.055f * pow(abs(linearColor), 1.0f / 2.4f) - 0.055f;
    return float3(linearColor.r < 0.0031308f ? low.r : high.r,
                  linearColor.g < 0.0031308f ? low.g : high.g,
                  linearColor.b < 0.0031308f ? low.b : high.b);
}

float4 main(VSOutput input) : SV_TARGET {
    float4 output = input.color;
    output.xyz = ApplySRGBGamma(output.xyz);
    return output;
}
