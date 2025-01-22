using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Core/Core.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/StructuredBufferDX12/StructuredBufferDX12.build.cs")]

[Generate]
public class ShadowManager : CommonProject
{
    public ShadowManager() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<StructuredBufferDX12>(target);
    }
}