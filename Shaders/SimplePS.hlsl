#include "Common.hlsli" // 쉐이더에서도 include 사용 가능

struct PixelShaderOutput
{
    float4 pixelColor : SV_Target0;
};

PixelShaderOutput main(PSInput input)
{
    PixelShaderOutput output;
    output.pixelColor = float4(0, 0, 0, 1);
    
    return output;
}
