// Compile with dxc.exe -T cs_6_6 -E cs_main -HV 2021 -Zpr -no-legacy-cbuf-layout -enable-16bit-types -Fo shader.bin shader.cs.hlsl

struct Push_Constants
{
    uint tex;
    uint unused1;
    uint unused2;
    uint unused3;
};
ConstantBuffer<Push_Constants> pc : register(b0, space0);

[numthreads(32, 32, 1)]
void cs_main(uint3 id : SV_DispatchThreadID)
{
    RWTexture2D<uint4> texture = ResourceDescriptorHeap[pc.tex];
    texture[id.xy] = uint4(id.xyz, 0);
}
