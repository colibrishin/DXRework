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
        conf.AddPublicDependency<RenderPipeline>(target);
        conf.AddPrivateDependency<RenderPassTaskDX12>(target);
        conf.AddPrivateDependency<GraphicPrimitiveShaderDX12>(target);
        conf.AddPrivateDependency<Shader>(target);
        conf.AddPublicDependency<D3D12Wrapper>(target);
    }
}