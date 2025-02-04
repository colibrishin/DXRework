using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/DirectXMath/DirectXMath.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/Boost/Boost.build.cs")]

[Generate]
public class RaytracingShader : CommonProject
{
    public RaytracingShader()
    {
        SourceFilesExtensions.Add(".hlsl");
        SourceFilesExtensions.Add(".hlsli");
    }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<Shader>(target);
        conf.AddPublicDependency<DirectXMath>(target);
        
        conf.TargetCopyFiles.Add
        (
            @"raytracing.hlsl"
        );
    }
}