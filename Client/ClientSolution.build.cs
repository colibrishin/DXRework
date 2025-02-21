using System;
using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/EngineSolution.build.cs")]

[Generate]
public class ClientProject : CommonProject
{
    public ClientProject() : base(false)
    {
        Name = "Client";

        ResourceFilesExtensions.Add(".ini");
        ResourceFilesExtensions.Add(".hlsl");
        ResourceFilesExtensions.Add(".jpg");
        ResourceFilesExtensions.Add(".png");

        AddTargets(Utils.GetDefinedTarget());
    }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.SolutionFolder = @"Client";

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<TextRenderer>(target);
        conf.AddPublicDependency<Shape>(target);
        conf.AddPublicDependency<Texture2D>(target);
        conf.AddPublicDependency<ModelRenderer>(target);
        conf.AddPublicDependency<ShadowTexture>(target);
        conf.AddPublicDependency<ComputeShader>(target);
        conf.AddPublicDependency<RenderComponent>(target);
        
        conf.AddPrivateDependency<BoneAnimation>(target);
        conf.AddPrivateDependency<Animator>(target);
        conf.AddPrivateDependency<AnimationTexture>(target);
        conf.AddPrivateDependency<InputManager>(target);
        conf.AddPrivateDependency<PhysicsManager>(target);
        conf.AddPrivateDependency<ShadowManager>(target);
        conf.AddPrivateDependency<ForwardRenderPassTask>(target);

        conf.TargetCopyFiles.Add
        (
            @"cs_intensity_test.hlsl", 
            @"intensity_test.hlsl"
        );
    }
}

[Generate]
public class ClientSolution : EngineSolution
{
    public ClientSolution()
    {
        IsFileNameToLower = false;
        Name = "Client";
        FastBuildAllProjectType = typeof(FastBuildAllOverrideProject);
    }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.SolutionPath = Utils.GetSolutionDir() + @"\Intermediate\ProjectFiles";
        string ProjectFilesDir = Utils.GetSolutionDir() + @"\Intermediate\ProjectFiles";
        Environment.SetEnvironmentVariable("ProjectFilesDir", ProjectFilesDir);

        conf.AddProject<ClientProject>(target);
    }
}