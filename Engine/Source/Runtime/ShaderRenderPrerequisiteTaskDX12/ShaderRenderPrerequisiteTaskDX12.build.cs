using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class ShaderRenderPrerequisiteTaskDX12 : CommonProject
{
    public ShaderRenderPrerequisiteTaskDX12() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<D3D12Wrapper>(target);
    }
}