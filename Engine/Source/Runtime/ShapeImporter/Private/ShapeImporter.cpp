#include "../Public/ShapeImporter.h"

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <boost/make_shared.hpp>

#include "Source/Runtime/Core/VertexElement/Public/VertexElement.hpp"
#include "Source/Runtime/Resources/Mesh/Public/Mesh.h"

namespace Engine
{
	bool ShapeImporter::Import
	(
		const GenericString& name,
		const boost::filesystem::path& path,
		std::vector<Strong<Resources::Mesh>>& meshes,
		Strong<Resources::Shape>& shape, 
		Strong<Resources::AnimationTexture>& animstex
	)
	{
		

		return false;
	}
}

