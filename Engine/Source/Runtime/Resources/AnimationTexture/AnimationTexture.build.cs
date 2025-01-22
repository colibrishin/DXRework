using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class AnimationTexture : CommonProject
{
    public AnimationTexture() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Texture3D>(target);

        conf.AddPrivateDependency<BaseAnimation>(target);
        conf.AddPrivateDependency<DirectXTK>(target);
        conf.AddPrivateDependency<DX12Agility>(target);
        conf.AddPrivateDependency<DirectXTex>(target);
        conf.AddPrivateDependency<BoneAnimation>(target);
    }
}