using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/Boost/Boost.build.cs")]

[Generate]
public class ShadowManager : CommonProject
{
    public ShadowManager() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<Shader>(target);
        conf.AddPublicDependency<ShadowTexture>(target);
        conf.AddPublicDependency<Texture2D>(target);
        conf.AddPublicDependency<RenderPipeline>(target);

        conf.AddPrivateDependency<ResourceManager>(target);
        conf.AddPrivateDependency<SceneManager>(target);
    }
}