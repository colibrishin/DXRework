using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class D3D12GraphicInterface : EngineCommonProject
{
    public D3D12GraphicInterface() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<TBB>(target);
        conf.AddPublicDependency<DX12Agility>(target);
        conf.AddPublicDependency<Texture>(target);
        conf.AddPublicDependency<Shape>(target);
        conf.AddPublicDependency<Mesh>(target);
        conf.AddPublicDependency<Shader>(target);
        conf.AddPublicDependency<ComputeShader>(target);

        conf.AddPrivateDependency<DirectXTex>(target);
        conf.AddPrivateDependency<Font>(target);
        conf.AddPrivateDependency<WinAPIWrapper>(target);
        conf.AddPrivateDependency<RenderPipeline>(target);
    }
}