using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/Boost/Boost.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/TBB/TBB.build.cs")]

[Generate]
public class CommonRenderer : CommonProject
{
    public CommonRenderer() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<TBB>(target);
        conf.AddPublicDependency<DescriptorHeap>(target);
        conf.AddPublicDependency<ModelRenderer>(target);
        conf.AddPublicDependency<ParticleRenderer>(target);
        conf.AddPublicDependency<Material>(target);

        conf.AddPrivateDependency<Animator>(target);
        conf.AddPrivateDependency<BoneAnimation>(target);
        conf.AddPrivateDependency<BaseAnimation>(target);
        conf.AddPrivateDependency<AtlasAnimation>(target);
    }
}