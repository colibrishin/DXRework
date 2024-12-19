#pragma once

class IModule 
{
public:
    virtual ~IModule() = default;
    virtual void Initialize() {}
    virtual void Destroy() {}
};