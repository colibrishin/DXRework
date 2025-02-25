using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class RaytracingExtension : EngineCommonProject
{
    public RaytracingExtension() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);

        conf.AddPrivateDependency<RenderPipeline>(target);
        conf.AddPrivateDependency<RaytracingRenderPassTask>(target);
        if (target.RenderType == ERenderType.Deferred)
        {
            conf.AddPrivateDependency<DeferredRenderPassTask>(target);
        }
        conf.AddPrivateDependency<ForwardRenderPassTask>(target);
    }
}