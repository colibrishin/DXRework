using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/Boost/Boost.build.cs")]

[Generate]
public class ComputePrimitiveShaderDX12 : CommonProject
{
    public ComputePrimitiveShaderDX12() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<TBB>(target);
        conf.AddPublicDependency<ComputeShader>(target);
        conf.AddPrivateDependency<PrimitivePipelineDX12>(target);
        conf.AddPrivateDependency<ComputeDispatchTaskDX12>(target);

        conf.AddPrivateDependency<RenderPipeline>(target);
        conf.AddPrivateDependency<Material>(target);
        conf.AddPrivateDependency<Texture>(target);
        conf.AddPrivateDependency<Shape>(target);
        conf.AddPrivateDependency<ModelRenderer>(target);
        conf.AddPrivateDependency<BaseAnimation>(target);
        conf.AddPrivateDependency<BoneAnimation>(target);
        conf.AddPrivateDependency<Animator>(target);
    }
}