using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class BoneAnimation : CommonProject
{
    public BoneAnimation() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<BaseAnimation>(target);
        conf.AddPrivateDependency<ResourceManager>(target);
        conf.AddPrivateDependency<DirectXTK>(target);
        conf.AddPrivateDependency<Bone>(target);
    }
}