using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/DX12Agility/DX12Agility.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/DirectXTK/DirectXTK.build.cs")]

[Generate]
public class D3D12Wrapper : CommonProject
{
    public D3D12Wrapper() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<TBB>(target);

        conf.AddPublicDependency<CommandPair>(target);
        conf.AddPublicDependency<ThrowIfFailed>(target);
        conf.AddPublicDependency<DescriptorHeap>(target);

        conf.AddPrivateDependency<DX12Agility>(target);
        conf.AddPrivateDependency<DirectXTK>(target);
        conf.AddPrivateDependency<WinAPIWrapper>(target);
    }
}