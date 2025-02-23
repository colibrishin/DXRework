using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/Boost/Boost.build.cs")]

[Generate]
public class ShadowManager : EngineCommonProject
{
    public ShadowManager() 
    {
        ResourceFilesExtensions.Add(".hlsl");
        ResourceFilesExtensions.Add(".hlsli");
    }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<Texture2D>(target);
        conf.AddPublicDependency<RenderPipeline>(target);

        conf.AddPrivateDependency<Shader>(target);
        conf.AddPrivateDependency<ShadowTexture>(target);
        conf.AddPrivateDependency<ShadowRenderTarget>(target);
        conf.AddPrivateDependency<ForwardRenderPassTask>(target);
        
        conf.TargetCopyFiles.Add
        (
            @"cascade_shadow_stage1.hlsl"
        );
    }
}