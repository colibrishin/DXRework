using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class Shader : EngineCommonProject
{
    public Shader() 
    {
        ResourceFilesExtensions.Add(".hlsl");
        ResourceFilesExtensions.Add(".hlsli");
    }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPrivateDependency<RenderPipeline>(target);

        conf.TargetCopyFiles.Add
        (
            @"atlas.hlsl", 
            @"billboard.hlsl",
            @"atlas_billboard.hlsl",
            @"color.hlsl",
            @"common.hlsli",
            @"default.hlsl",
            @"normal.hlsl",
            @"refraction.hlsl",
            @"skybox.hlsl",
            @"specular.hlsl",
            @"specular_normal.hlsl",
            @"specular_tex.hlsl",
            @"type.hlsli",
            @"utility.hlsli",
            @"vs_default.hlsl",
            @"pbr.hlsli"
        );
    }
}