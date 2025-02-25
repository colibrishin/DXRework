using System;
using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/**/**.build.cs")]

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
		
        conf.AddProject<Core>(target);

        if (target.Platform == Platform.win64 || target.Platform == Platform.win32)
        {
            conf.AddProject<WinAPIWrapper>(target);
        }

        conf.AddProject<RenderComponent>(target);
        conf.AddProject<ShapeRenderComponent>(target);
        conf.AddProject<RaycastExtension>(target);
        conf.AddProject<Animator>(target);
        conf.AddProject<ModelRenderer>(target);
        conf.AddProject<ParticleRenderer>(target);
        conf.AddProject<ParticleRendererExtension>(target);
        conf.AddProject<SoundPlayer>(target);
        conf.AddProject<TextRenderer>(target);

        if (target.LaunchType == ELaunchType.Client || target.LaunchType == ELaunchType.Editor)
        {   
            conf.AddProject<ParticleRendererRenderTask>(target);
            conf.AddProject<ModelRendererRenderTask>(target);
            conf.AddProject<ForwardRenderPassTask>(target);

            conf.AddProject<ProjectionFrustum>(target);
            conf.AddProject<ReflectionEvaluator>(target);
            conf.AddProject<RenderPipeline>(target);
            conf.AddProject<ShadowManager>(target);
            conf.AddProject<CameraManager>(target);
            
            if (target.RenderType == ERenderType.Deferred)
            {
                conf.AddProject<DeferredRenderPassTask>(target);
            }

            if (target.Raytracing == ERaytracing.On)
            {
                conf.AddProject<RaytracingShader>(target);
                conf.AddProject<RaytracingRenderPassTask>(target);
                conf.AddProject<RaytracingExtension>(target);
            }

            conf.AddProject<InputManager>(target);

            if (target.GraphicAPI == EGraphicAPI.D3D12)
            {
                conf.AddProject<DirectInputInterface>(target);
                conf.AddProject<D3D12GraphicInterface>(target);
            }

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
            conf.AddProject<ShapeImporter>(target);
            conf.AddProject<Texture>(target);
            conf.AddProject<Texture1D>(target);
            conf.AddProject<Texture2D>(target);
            conf.AddProject<Texture3D>(target);
            conf.AddProject<Font>(target);
            conf.AddProject<Sound>(target);

            conf.AddProject<SoundManager>(target);
            conf.AddProject<FMODSoundInterface>(target);
        }

        if (target.LaunchType == ELaunchType.Editor)
        {
            conf.AddProject<ImGuiManager>(target);
        }
        
        conf.AddProject<PhysicsManager>(target);
    }
}