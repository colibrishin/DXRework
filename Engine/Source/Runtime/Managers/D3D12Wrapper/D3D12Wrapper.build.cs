using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Abstracts/CoreSingleton/CoreSingleton.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Allocator/Allocator.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/ThrowIfFailed/ThrowIfFailed.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/ThirdParty/DX12Agility/DX12Agility.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/ThirdParty/DirectXTK/DirectXTK.build.cs")]

[Generate]
public class D3D12Wrapper : CommonProject
{
    public D3D12Wrapper() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<TypeLibrary>(target);
        conf.AddPrivateDependency<ThrowIfFailed>(target);
        conf.AddPrivateDependency<CoreSingleton>(target);
        conf.AddPrivateDependency<Allocator>(target);
        conf.AddPrivateDependency<DX12Agility>(target);
        conf.AddPrivateDependency<DirectXTK>(target);
    }
}