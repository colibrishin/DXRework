using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class ReflectionEvaluator : EngineCommonProject
{
    public ReflectionEvaluator() 
    {
        SourceFilesExtensions.Add(".hlsl");
        SourceFilesExtensions.Add(".hlsli");
     }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<TBB>(target);
        conf.AddPublicDependency<Texture2D>(target);
        conf.AddPrivateDependency<EngineEntryPoint>(target);
    }
}