#include "../Public/ShapeImporter.h"

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <boost/make_shared.hpp>

#include "ShapeImporter.generated.h"

namespace Engine
{
	bool ShapeImporter::Import
	(
		const GenericString& name,
		const std::filesystem::path& path,
		std::vector<Strong<Resources::Mesh>>& meshes,
		Strong<Resources::Shape>& shape, 
		Strong<Resources::AnimationTexture>& animstex
	)
	{
		

		return false;
	}

    bool ShapeImporterModule::InitializeImpl()
    {
        return true;
    }

    bool ShapeImporterModule::ShutdownImpl()
    {
        return true;
    }

    bool ShapeImporterModule::DynamicLoadable()
    {
        return true;
    }
}

