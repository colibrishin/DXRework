using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Abstracts/CoreResource/CoreResource.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/TypeLibrary/TypeLibrary.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/StructuredBufferDX12/StructuredBufferDX12.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Managers/ResourceManager/ResourceManager.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Serialization/Serialization.build.cs")]

[Generate]
public class Mesh : CommonProject
{
    public Mesh() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<TypeLibrary>(target);
        conf.AddPublicDependency<CoreResource>(target);
        conf.AddPublicDependency<StructuredBufferDX12>(target);
        conf.AddPrivateDependency<ResourceManager>(target);
        conf.AddPrivateDependency<Serialization>(target);
    }
}