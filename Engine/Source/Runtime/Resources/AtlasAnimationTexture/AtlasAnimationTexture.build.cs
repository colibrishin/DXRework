using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/DirectXTex/DirectXTex.build.cs")]

[Generate]
public class AtlasAnimationTexture : CommonProject
{
    public AtlasAnimationTexture() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Texture3D>(target);
        conf.AddPublicDependency<Texture2D>(target);

        conf.AddPrivateDependency<ResourceManager>(target);
        conf.AddPrivateDependency<CommandPair>(target);
        conf.AddPrivateDependency<D3D12Wrapper>(target);
        conf.AddPrivateDependency<DirectXTex>(target);
    }
}