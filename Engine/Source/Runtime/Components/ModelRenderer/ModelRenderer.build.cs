using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class ModelRenderer : CommonProject
{
    public ModelRenderer() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<TBB>(target);
        conf.AddPublicDependency<RenderComponent>(target);
        conf.AddPublicDependency<DirectXTK>(target);
    }
}