using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/Boost/Boost.build.cs")]

[Generate]
public class Material : EngineCommonProject
{
    public Material() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<Shader>(target);
        conf.AddPublicDependency<AtlasAnimation>(target);
        conf.AddPublicDependency<AtlasAnimationTexture>(target);

        if (target.Raytracing == ERaytracing.On)
        {
            conf.AddPublicDependency<RaytracingShader>(target);   
        }

        conf.AddPrivateDependency<Texture>(target);
    }
}