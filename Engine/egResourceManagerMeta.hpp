#pragma once
#include "egAnimationsTexture.h"
#include "egBaseAnimation.h"
#include "egBone.h"
#include "egBoneAnimation.h"
#include "egComputeShader.h"
#include "egFont.h"
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
        const auto title = std::string("Load ") + typeid(T).name();
        if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
          static char buf[256] = {0};

          ImGui::InputText("Filename", buf, IM_ARRAYSIZE(buf));

          if (ImGui::Button("Load"))
          {
            const auto scene = Serializer::Deserialize<T>(buf);
            GetResourceManager().AddResource<T>(scene);

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
      if (ImGui::MenuItem(typeid(T).name()))
      {
        ResourceManager::m_b_imgui_load_dialog_[boost::mpl::find<LoadableResourceTypes, T>::type::pos::value] = true;
      }
    }
  };
}
