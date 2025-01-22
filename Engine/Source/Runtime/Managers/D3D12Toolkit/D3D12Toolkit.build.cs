using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Core/Core.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/ThrowIfFailed/ThrowIfFailed.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/ThirdParty/DX12Agility/DX12Agility.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/ThirdParty/DirectXTK/DirectXTK.build.cs")]

[Generate]
public class D3D12Toolkit : CommonProject
{
    public D3D12Toolkit() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<Core>(target);
        conf.AddPrivateDependency<ThrowIfFailed>(target);
        conf.AddPrivateDependency<DX12Agility>(target);
        conf.AddPrivateDependency<DirectXTK>(target);
    }
}