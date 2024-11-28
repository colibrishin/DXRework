using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Abstracts/CoreComponent/CoreComponent.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/DirectXTK/DirectXTK.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/DirectXMath/DirectXMath.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Delegation/Delegation.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/GenericBounding/GenericBounding.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Serialization/Serialization.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/RaycastExtension/RaycastExtension.build.cs")]

[Generate]
public class Collider : CommonProject
{
    public Collider() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPrivateDependency<TypeLibrary>(target);
        conf.AddPrivateDependency<GenericBouding>(target);
        conf.AddPrivateDependency<CoreComponent>(target);
        conf.AddPrivateDependency<Serialization>(target);
        conf.AddPrivateDependency<Delegation>(target);
        conf.AddPrivateDependency<Transform>(target);
        conf.AddPrivateDependency<DirectXMath>(target);
        conf.AddPrivateDependency<DirectXTK>(target);
        conf.AddPrivateDependency<VertexElement>(target);
        conf.AddPrivateDependency<RaycastExtension>(target);
        conf.AddPrivateDependency<RenderComponent>(target);
    }
}