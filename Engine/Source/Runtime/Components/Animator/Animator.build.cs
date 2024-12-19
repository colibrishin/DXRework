using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class Animator : CommonProject
{
    public Animator() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<TBB>(target);
        conf.AddPublicDependency<DirectXTK>(target);        
        conf.AddPublicDependency<BaseAnimation>(target);

        conf.AddPrivateDependency<AtlasAnimation>(target);
        conf.AddPrivateDependency<BoneAnimation>(target);
        conf.AddPrivateDependency<Material>(target);
        conf.AddPrivateDependency<RenderComponent>(target);
        conf.AddPrivateDependency<ModelRenderer>(target);
    }
}