using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Abstracts/CoreResource/CoreResource.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/VertexElement/VertexElement.build.cs")]

[Generate]
public class AnimationTexture : CommonProject
{
    public AnimationTexture() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<TypeLibrary>(target);
        conf.AddPublicDependency<CoreResource>(target);
        conf.AddPrivateDependency<VertexElement>(target);
    }
}