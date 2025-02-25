using System;
using System.Collections;
using Microsoft.Win32;
using Sharpmake;

[module: Include("%EngineDir%/Client/ClientSolution.build.cs")]
[module: Include("%EngineDir%/Engine/Monolith/Monolith.build.cs")]

[Generate]
public class MonolithClient : Monolith
{
    public MonolithClient() : base(Utils.GetDefinedTargetLaunchTypeFiltered(ELaunchType.Client)) {}
}