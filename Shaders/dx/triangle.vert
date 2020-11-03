struct VSInput
{
    float3 Position : POSITION;
    float3 Color : COLOR;
};

struct VSOutput {
	float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

VSOutput main(VSInput input)
{
    VSOutput result;
    result.Position = float4(input.Position, 1.0);
    result.Color = float4(input.Color, 1.0);
    return result;
}
