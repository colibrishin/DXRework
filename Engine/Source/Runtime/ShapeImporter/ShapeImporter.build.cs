using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/DirectXMath/DirectXMath.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/Assimp/Assimp.build.cs")]

[Generate]
public class ShapeImporter : CommonProject
{
    public ShapeImporter() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<DirectXMath>(target);
        conf.AddPublicDependency<Mesh>(target);
        conf.AddPublicDependency<Shape>(target);
        conf.AddPrivateDependency<Assimp>(target);
        conf.AddPrivateDependency<Boost>(target);
    }
}