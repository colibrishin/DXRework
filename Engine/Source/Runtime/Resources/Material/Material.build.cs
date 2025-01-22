using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/Boost/Boost.build.cs")]

[Generate]
public class Material : CommonProject
{
    public Material() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<CommandPair>(target);
        conf.AddPublicDependency<DescriptorHeap>(target);
        conf.AddPublicDependency<D3D12Wrapper>(target);

        conf.AddPrivateDependency<DirectXTK>(target);
        conf.AddPrivateDependency<RenderPipeline>(target);
        conf.AddPrivateDependency<ResourceManager>(target);
        conf.AddPrivateDependency<Shape>(target);
        conf.AddPrivateDependency<Mesh>(target);
        conf.AddPrivateDependency<Shader>(target);
        conf.AddPrivateDependency<Texture>(target);
        conf.AddPrivateDependency<AnimationTexture>(target);
        conf.AddPrivateDependency<AtlasAnimationTexture>(target);
        conf.AddPrivateDependency<ThrowIfFailed>(target);
    }
}