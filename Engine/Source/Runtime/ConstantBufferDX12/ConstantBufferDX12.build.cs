using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/Boost/Boost.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Managers/D3D12Wrapper/D3D12Wrapper.build.cs")]

[Generate]
public class ConstantBufferDX12 : CommonProject
{
    public ConstantBufferDX12() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<ThrowIfFailed>(target);
        conf.AddPrivateDependency<D3D12Wrapper>(target);
    }
}