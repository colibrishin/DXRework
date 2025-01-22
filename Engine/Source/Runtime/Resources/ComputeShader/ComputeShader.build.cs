using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class ComputeShader : CommonProject
{
    public ComputeShader() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Shader>(target);

        conf.AddPrivateDependency<RenderPipeline>(target);
        conf.AddPrivateDependency<Debugger>(target);
    }
}