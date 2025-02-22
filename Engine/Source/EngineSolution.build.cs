using System;
using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Programs/**/*.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/**/*.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Managers/**/*.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Resources/**/*.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Components/**/*.build.cs")]

[Generate]
public class EngineSolution : Solution
{
    public EngineSolution() : base(typeof(EngineTarget))
    {
        IsFileNameToLower = false;
        Name = "Engine";
        FastBuildAllProjectType = typeof(FastBuildAllOverrideProject);
        
        AddTargets(Utils.GetDefinedTarget());
    }

    [Configure()]
    public virtual void ConfigureAll(Configuration conf, EngineTarget target)
    {
        Utils.MakeConfiturationNameDefine(conf, target);

        conf.SolutionPath = Utils.GetSolutionDir() + @"\Intermediate\ProjectFiles";
        string ProjectFilesDir = Utils.GetSolutionDir() + @"\Intermediate\ProjectFiles";
        Environment.SetEnvironmentVariable("ProjectFilesDir", ProjectFilesDir);

		// Add Projects
        {
            conf.AddProject<Core>(target);
            conf.AddProject<RaycastExtension>(target);

            {
                conf.AddProject<ModelRenderer>(target);
                conf.AddProject<ParticleRenderer>(target);
                conf.AddProject<ParticleRendererExtension>(target);
                conf.AddProject<Animator>(target);
            }

            {
                conf.AddProject<ParticleRendererRenderTask>(target);
                conf.AddProject<ModelRendererRenderTask>(target);
                conf.AddProject<ForwardRenderPassTask>(target);
                
                if (target.RenderType == ERenderType.Deferred)
                {
                    conf.AddProject<DeferredRenderPassTask>(target);
                }
            }

            {
                conf.AddProject<Font>(target);
                conf.AddProject<TextRenderer>(target);
            }

            {
                if (target.LaunchType == ELaunchType.Editor)
                {
                    conf.AddProject<ImGuiManager>(target);
                }
                
                conf.AddProject<PhysicsManager>(target);
                conf.AddProject<SoundManager>(target);
                conf.AddProject<EngineEntryPoint>(target);
                conf.AddProject<ProjectionFrustum>(target);
                conf.AddProject<ReflectionEvaluator>(target);
                conf.AddProject<RenderPipeline>(target);
                conf.AddProject<ShadowManager>(target);
                conf.AddProject<SoundManager>(target);
                conf.AddProject<CameraManager>(target);
                conf.AddProject<Launch>(target);
            }

            if (target.LaunchType == ELaunchType.Client || target.LaunchType == ELaunchType.Editor)
            {
                conf.AddProject<InputManager>(target);
            }

            if (target.Raytracing == ERaytracing.On)
            {
                conf.AddProject<RaytracingShader>(target);
                conf.AddProject<RaytracingRenderPassTask>(target);
            }

            {
                conf.AddProject<AnimationTexture>(target);
                conf.AddProject<AtlasAnimation>(target);
                conf.AddProject<AtlasAnimationTexture>(target);
                conf.AddProject<BaseAnimation>(target);
                conf.AddProject<Bone>(target);
                conf.AddProject<BoneAnimation>(target);
                conf.AddProject<ComputeShader>(target);
                conf.AddProject<Material>(target);
                conf.AddProject<Mesh>(target);
                conf.AddProject<Shader>(target);
                conf.AddProject<ShadowTexture>(target);
                conf.AddProject<Shape>(target);
                conf.AddProject<Texture>(target);
                conf.AddProject<Texture1D>(target);
                conf.AddProject<Texture2D>(target);
                conf.AddProject<Texture3D>(target);
            }

            {
                conf.AddProject<Sound>(target);
                conf.AddProject<SoundPlayer>(target);
                conf.AddProject<SoundManager>(target);
                conf.AddProject<FMODSoundInterface>(target);
            }

            conf.SetStartupProject<Launch>();
        }
    }
}