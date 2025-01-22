using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/DirectXTK/DirectXTK.build.cs")]

[Generate]
public class StructuredBufferDX12 : CommonProject
{
    public StructuredBufferDX12() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPrivateDependency<DirectXTK>(target);
    }
}