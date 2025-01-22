using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class ConstantBufferRenderPrerequisiteTaskDX12 : CommonProject
{
    public ConstantBufferRenderPrerequisiteTaskDX12() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Renderer>(target);
        conf.AddPublicDependency<D3D12Wrapper>(target);
    }
}