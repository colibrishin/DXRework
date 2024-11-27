using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Abstracts/CoreObjectBase/CoreObjectBase.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Components/Transform/Transform.build.cs")]

[Generate]
public class BoundingGetter : CommonProject
{
    public BoundingGetter() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<TypeLibrary>(target);
        conf.AddPublicDependency<GenericBounding>(target);
        conf.AddPrivateDependency<CoreObjectBase>(target);
        conf.AddPrivateDependency<Transform>(target);
    }
}