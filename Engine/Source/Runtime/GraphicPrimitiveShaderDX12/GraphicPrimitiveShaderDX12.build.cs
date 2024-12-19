using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/Boost/Boost.build.cs")]

[Generate]
public class GraphicPrimitiveShaderDX12 : CommonProject
{
    public GraphicPrimitiveShaderDX12() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<RenderPipeline>(target);
        conf.AddPublicDependency<Shader>(target);
        
        conf.AddPrivateDependency<ShaderRenderPrerequisiteTaskDX12>(target);
        conf.AddPrivateDependency<DX12Agility>(target);
        conf.AddPrivateDependency<D3D12Wrapper>(target);
    }
}