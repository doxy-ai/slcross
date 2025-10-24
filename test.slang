struct VSInput {
    float3 position : POSITION;    // object-space position
};

struct VSOutput {
    float4 position : SV_Position; // clip-space position
};

[shader("vertex")]
VSOutput vertex(VSInput in) {
    VSOutput out;
    out.position = float4(in.position, 1.0);
    return out;
}

[shader("fragment")]
float4 fragment(VSOutput in) : SV_Target {
    return float4(1.0, 0.5, 0.0, 1.0);
}