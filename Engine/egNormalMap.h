#pragma once
#include "egTexture.h"

namespace Engine::Resources
{
    class NormalMap : public Texture
    {
    public:
        INTERNAL_RES_CHECK_CONSTEXPR(RES_T_NORMAL)

        explicit NormalMap(std::filesystem::path path);

        ~NormalMap() override;

        void     Initialize() override;
        void     PreUpdate(const float& dt) override;
        void     Update(const float& dt) override;
        void     PreRender(const float& dt) override;
        void     Render(const float& dt) override;
        void     PostRender(const float& dt) override;

        void          Load_INTERNAL() override;
        void          Unload_INTERNAL() override;
        eResourceType GetResourceType() const override;

        RESOURCE_SELF_INFER_GETTER(NormalMap)
        RESOURCE_SELF_INFER_CREATE(NormalMap)

    protected:
        NormalMap();

    private:
        SERIALIZER_ACCESS
    };
} // namespace Engine::Resources

BOOST_CLASS_EXPORT_KEY(Engine::Resources::NormalMap)
