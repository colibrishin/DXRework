using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/DirectXTex/DirectXTex.build.cs")]

[Generate]
public class AtlasAnimationTexture : EngineCommonProject
{
    public AtlasAnimationTexture() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<TBB>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<Texture>(target);
		conf.AddPublicDependency<AtlasAnimation>(target);
    }
}