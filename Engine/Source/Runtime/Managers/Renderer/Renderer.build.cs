using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class Renderer : CommonProject
{
    public Renderer() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<TBB>(target);
        conf.AddPublicDependency<CommandPair>(target);
        conf.AddPublicDependency<CommandPairExtension>(target);
        conf.AddPublicDependency<DescriptorHeap>(target);
        conf.AddPublicDependency<D3D12Wrapper>(target);
        conf.AddPublicDependency<Texture2D>(target);

        conf.AddPrivateDependency<DirectXTK>(target);
        conf.AddPrivateDependency<Texture>(target);
        conf.AddPrivateDependency<RenderComponent>(target);
        conf.AddPrivateDependency<Shader>(target);
        conf.AddPrivateDependency<ShadowTexture>(target);
        conf.AddPrivateDependency<RenderPipeline>(target);
        conf.AddPrivateDependency<SceneManager>(target);
        conf.AddPrivateDependency<Debugger>(target);
        conf.AddPrivateDependency<ProjectionFrustum>(target);
        conf.AddPrivateDependency<ReflectionEvaluator>(target);
        conf.AddPrivateDependency<Material>(target);
    }
}