using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class Shader : CommonProject
{
    public Shader() 
    {
        SourceFilesExtensions.Add(".hlsl");
        SourceFilesExtensions.Add(".hlsli");
    }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<RenderPipeline>(target);

        conf.TargetCopyFiles.Add
        (
            @"atlas.hlsl", 
            @"billboard.hlsl", 
            @"cascade_shadow_stage1.hlsl",
            @"color.hlsl",
            @"common.hlsli",
            @"default.hlsl",
            @"normal.hlsl",
            @"raytracing.hlsl",
            @"refraction.hlsl",
            @"skybox.hlsl",
            @"specular.hlsl",
            @"specular_normal.hlsl",
            @"specular_tex.hlsl",
            @"type.hlsli",
            @"utility.hlsli",
            @"vs_default.hlsl"
        );
    }
}