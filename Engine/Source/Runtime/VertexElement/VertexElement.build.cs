using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/DirectXTK/DirectXTK.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/DirectXMath/DirectXMath.build.cs")]

[Generate]
public class VertexElement : CommonProject
{
    public VertexElement() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<TypeLibrary>(target);
        conf.AddPublicDependency<DirectXTK>(target);
        conf.AddPublicDependency<DirectXMath>(target);
    }
}