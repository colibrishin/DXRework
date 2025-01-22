using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/Assimp/Assimp.build.cs")]

[Generate]
public class Shape : CommonProject
{
    public Shape() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPrivateDependency<Assimp>(target);
        conf.AddPrivateDependency<MathExtension>(target);
    }
}