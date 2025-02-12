using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class ModelRenderer : EngineCommonProject
{
    public ModelRenderer() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<TBB>(target);
        conf.AddPublicDependency<ShapeRenderComponent>(target);
        
        conf.AddPrivateDependency<DirectXTK>(target);
    }
}