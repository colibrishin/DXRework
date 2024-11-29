using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/ThirdParty/Boost/Boost.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Managers/D3D12Wrapper/D3D12Wrapper.build.cs")]

[Generate]
public class DescriptorHeap : CommonProject
{
    public DescriptorHeap() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Allocator>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<ThrowIfFailed>(target);
        conf.AddPrivateDependency<D3D12Wrapper>(target);
    }
}