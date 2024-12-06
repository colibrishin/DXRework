using System;
using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/**/*.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Managers/**/*.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Resources/**/*.build.cs")]

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
            conf.AddProject<SoundManager>(target);
            conf.AddProject<BoundingGetter>(target);
            conf.AddProject<CommandPair>(target);
            conf.AddProject<CommandPairExtension>(target);
            conf.AddProject<ConstantBufferDX12>(target);
            conf.AddProject<DescriptorHeap>(target);
            conf.AddProject<GenericBounding>(target);
            conf.AddProject<GJK>(target);
            conf.AddProject<MathExtension>(target);
            conf.AddProject<Octree>(target);
            conf.AddProject<StructuredBufferDX12>(target);
            conf.AddProject<ThrowIfFailed>(target);
            conf.AddProject<Verlet>(target);
            // conf.AddProject<Engine>(target);
            // conf.AddProject<Launch>(target);
            // conf.AddProject<Network>(target);
            // conf.AddProject<RenderCore>(target);
            // conf.AddProject<Renderer>(target);
            // conf.AddProject<RHI>(target);
            // conf.AddProject<D3D11RHI>(target);
            // conf.AddProject<Slate>(target);

            // // config
            // conf.AddProject<EngineConfig>(target);

            {
                //conf.AddProject<CommonRenderer>(target);
                conf.AddProject<D3D12Toolkit>(target);
                conf.AddProject<D3D12Wrapper>(target);
                conf.AddProject<Debugger>(target);
                conf.AddProject<EngineEntryPoint>(target);
                conf.AddProject<InputManager>(target);
                conf.AddProject<ProjectionFrustum>(target);
                // conf.AddProject<RaytracingPipeline>(target);
                conf.AddProject<ReflectionEvaluator>(target);
                conf.AddProject<Renderer>(target);
                conf.AddProject<RenderPipeline>(target);
                conf.AddProject<ResourceMananger>(target);
                conf.AddProject<SceneManager>(target);
                conf.AddProject<ShadowManager>(target);
                conf.AddProject<SoundManager>(target);
                conf.AddProject<StepTimer>(target);
                conf.AddProject<TaskScheduler>(target);
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