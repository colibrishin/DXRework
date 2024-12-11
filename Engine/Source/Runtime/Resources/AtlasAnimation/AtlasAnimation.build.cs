using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/Pugixml/Pugixml.build.cs")]

[Generate]
public class AtlasAnimation : CommonProject
{
    public AtlasAnimation() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<BaseAnimation>(target);
        conf.AddPrivateDependency<Pugixml>(target);
        conf.AddPrivateDependency<ResourceManager>(target);
    }
}