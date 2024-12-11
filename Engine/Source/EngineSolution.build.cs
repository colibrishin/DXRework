using System;
using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/**/*.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Managers/**/*.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Resources/**/*.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Components/**/*.build.cs")]

public class FastBuildAllOverrideProject : FastBuildAllProject 
{
    public FastBuildAllOverrideProject() : base(typeof(EngineTarget))
    {
    }

    [Configure()]
    public virtual void ConfigureAll(Configuration conf, EngineTarget target) 
    {
        Utils.MakeConfiturationNameDefine(conf, target);
    }
}

[Generate]
public class EngineSolution : Solution
{
    public EngineSolution() : base(typeof(EngineTarget))
    {
        IsFileNameToLower = false;
        Name = "Engine";
        FastBuildAllProjectType = typeof(FastBuildAllOverrideProject);

        AddTargets(new EngineTarget(
            ELaunchType.Editor | ELaunchType.Client | ELaunchType.Server,
            Platform.win64,
            DevEnv.vs2022,
            Optimization.Debug | Optimization.Release,
            OutputType.Lib,
            Blob.NoBlob,
            BuildSystem.FastBuild
        ));
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
            // // ThirdParty
            // conf.AddProject<FBX>(target);

            // dll
            conf.AddProject<Core>(target);
            conf.AddProject<GJK>(target);
            conf.AddProject<RaycastExtension>(target);
            conf.AddProject<ParticleRendererExtension>(target);
            conf.AddProject<Verlet>(target);

            {
                conf.AddProject<CommandPair>(target);
                conf.AddProject<CommandPairExtension>(target);
                conf.AddProject<DescriptorHeap>(target);
                conf.AddProject<ThrowIfFailed>(target);
            }
            

            {
                conf.AddProject<RenderComponent>(target);
                conf.AddProject<ModelRenderer>(target);
                conf.AddProject<ParticleRenderer>(target);
                conf.AddProject<Animator>(target);
            }

            {
                conf.AddProject<ParticleRendererRenderTask>(target);
                conf.AddProject<ModelRendererRenderTask>(target);
                conf.AddProject<RenderPassTaskDX12>(target);
                conf.AddProject<ShadowRenderPrerequisiteTaskDX12>(target);
                conf.AddProject<ViewportRenderPrerequisiteTaskDX12>(target);
                conf.AddProject<ShaderRenderPrerequisiteTaskDX12>(target);
                conf.AddProject<GraphicPrimitiveShaderDX12>(target);
                conf.AddProject<PipelineRenderPrerequisiteTaskDX12>(target);
                conf.AddProject<StructuredBufferRenderPrerequisiteTaskDX12>(target);
                conf.AddProject<ConstantBufferRenderPrerequisiteTaskDX12>(target);
                conf.AddProject<PrimitivePipelineDX12>(target);
            }

            {
                conf.AddProject<SoundManager>(target);
                conf.AddProject<D3D12Toolkit>(target);
                conf.AddProject<D3D12Wrapper>(target);
                conf.AddProject<Debugger>(target);
                conf.AddProject<EngineEntryPoint>(target);
                conf.AddProject<InputManager>(target);
                conf.AddProject<CameraManager>(target);
                conf.AddProject<ProjectionFrustum>(target);
                //conf.AddProject<RaytracingPipeline>(target);
                conf.AddProject<ReflectionEvaluator>(target);
                conf.AddProject<Renderer>(target);
                conf.AddProject<RenderPipeline>(target);
                conf.AddProject<ResourceManager>(target);
                conf.AddProject<SceneManager>(target);
                conf.AddProject<SoundManager>(target);
                conf.AddProject<StepTimer>(target);
                conf.AddProject<WinAPIWrapper>(target);
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

            // exe
            conf.SetStartupProject<WinAPIWrapper>();
        }
    }
}