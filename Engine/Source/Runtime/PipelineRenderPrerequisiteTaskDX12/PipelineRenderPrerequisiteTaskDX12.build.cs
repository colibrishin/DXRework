using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class PipelineRenderPrerequisiteTaskDX12 : CommonProject
{
    public PipelineRenderPrerequisiteTaskDX12() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<RenderPipeline>(target);
        conf.AddPublicDependency<D3D12Wrapper>(target);
        conf.AddPrivateDependency<Shader>(target);
    }
}