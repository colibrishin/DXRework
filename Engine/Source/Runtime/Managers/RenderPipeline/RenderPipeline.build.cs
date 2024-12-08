using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/DX12Agility/DX12Agility.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/DirectXTK/DirectXTK.build.cs")]


[Generate]
public class RenderPipeline : CommonProject
{
    public RenderPipeline() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<CommandPair>(target);
        conf.AddPublicDependency<DescriptorHeap>(target);
        conf.AddPrivateDependency<Shader>(target);
        conf.AddPrivateDependency<DX12Agility>(target);
        conf.AddPrivateDependency<DirectXTK>(target);
        conf.AddPublicDependency<D3D12Wrapper>(target);
        conf.AddPrivateDependency<ThrowIfFailed>(target);
    }
}