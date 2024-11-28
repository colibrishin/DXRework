using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Abstracts/CoreResource/CoreResource.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/VertexElement/VertexElement.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Serialization/Serialization.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/MathExtension/MathExtension.build.cs")]

[Generate]
public class Shape : CommonProject
{
    public Shape() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<TypeLibrary>(target);
        conf.AddPublicDependency<CoreResource>(target);
        conf.AddPrivateDependency<Mesh>(target);
        conf.AddPrivateDependency<VertexElement>(target);
        conf.AddPrivateDependency<Serialization>(target);
        conf.AddPrivateDependency<MathExtension>(target);
    }
}