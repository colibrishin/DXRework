#pragma once
#include "egAnimationsTexture.h"
#include "egBaseAnimation.h"
#include "egBone.h"
#include "egBoneAnimation.h"
#include "egComputeShader.h"
#include "egFont.h"
#include "egHelper.hpp"
#include "egMaterial.h"
#include "egMesh.h"
#include "egResourceManager.hpp"
#include "egShader.hpp"
#include "egShadowTexture.h"
#include "egShape.h"
#include "egSound.h"
#include "egTexture1D.h"
#include "egTexture2D.h"
#include "egTexture3D.h"

namespace Engine::Manager
{
  struct MetaResourceLoadDialog
  {
    template <typename T>
    void operator()(boost::type<T>) const
    {
      bool& flag = ResourceManager::m_b_imgui_load_dialog_[boost::mpl::find<LoadableResourceTypes, T>::type::pos::value];
      if (flag)
      {
        const auto title = std::string("Load ") + pretty_name<T>::get();
        if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
          static char buf[256] = {0};

          ImGui::InputText("Filename", buf, IM_ARRAYSIZE(buf));

          if (ImGui::Button("Load"))
          {
            const auto resource = Serializer::Deserialize<Abstract::Entity>(buf)->GetSharedPtr<T>();
            GetResourceManager().AddResource<T>(resource);

            flag = false;
            ImGui::CloseCurrentPopup();
          }

          ImGui::SameLine();

          if (ImGui::Button("Cancel"))
          {
            flag = false;
            ImGui::CloseCurrentPopup();
          }

          ImGui::End();
        }
      }
    }
  };

  struct MetaLoadMenu
  {
    template <typename T>
    void operator()(boost::type<T>) const
    {
      if (ImGui::MenuItem(pretty_name<T>::get().c_str()))
      {
        ResourceManager::m_b_imgui_load_dialog_[boost::mpl::find<LoadableResourceTypes, T>::type::pos::value] = true;
      }
    }
  };
}
