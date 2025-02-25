using System;
using System.Collections;
using Microsoft.Win32;
using Sharpmake;

[module: Include("%EngineDir%/Client/ClientSolution.build.cs")]
[module: Include("%EngineDir%/Engine/Monolith/Monolith.build.cs")]

[Generate]
public class MonolithServer : Monolith
{
    public MonolithServer() : base(Utils.GetDefinedTargetLaunchTypeFiltered(ELaunchType.Server)) {}
}