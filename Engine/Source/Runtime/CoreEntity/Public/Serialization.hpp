#pragma once
#include "Entity.h"
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

namespace Engine
{
	class Serializer
	{
	public:
		// Serialize the object. Use when the object is nested, and if nested objects are required to be serialized.
		template <typename T>
		static bool Serialize(const std::string& filename, const boost::shared_ptr<T>& object)
		{
			object->OnSerialized();
			std::string fixed_name = filename;

			constexpr char illegal_chars[] =
			{
				'#',
				'%',
				'&',
				'{',
				'}',
				'\\',
				'<',
				'>',
				'?',
				'/',
				' ',
				'$',
				'!',
				'\'',
				'\"',
				':',
				'@',
				'+',
				'|',
				'`',
				'='
			};

			for (const auto& illegal : illegal_chars)
			{
				while (const auto pos = fixed_name.find(illegal))
				{
					if (pos == std::string::npos)
					{
						break;
					}
					fixed_name.replace(pos, 1, "_");
				}
			}

			//int                   i               = 0;
			//std::string           tagged_filename = fixed_name + "_%d";
			//char                  buffer[1024]    = {};
			//std::filesystem::path final_path      = fixed_name;
			std::string extension = ".meta";

			//while (std::filesystem::exists(final_path.string() + extension))
			//{
			//  sprintf_s(buffer, 1024, tagged_filename.c_str(), i++);
			//  final_path = buffer;
			//}

			const std::filesystem::path folder = object->GetPrettyTypeName();

			if (!exists(folder))
			{
				create_directory(folder);
			}

			std::filesystem::path final_filename = fixed_name + extension;

			std::filesystem::path final_path = folder / final_filename;
			object->m_meta_path_             = final_path;
			object->m_meta_str_              = final_path.string();

			if (exists(final_path))
			{
				std::filesystem::remove(final_path);
			}

			const auto entity = boost::static_pointer_cast<Abstracts::Entity>(object);

			std::fstream                    stream(final_path, std::ios::out | std::ios::binary);
			boost::archive::binary_oarchive archive(stream);
			archive << entity;
			return true;
		}

		template <typename T>
		static boost::shared_ptr<T> Deserialize(const std::string& filename)
		{
			boost::shared_ptr<Abstracts::Entity> object;
			std::fstream                        stream(filename, std::ios::in | std::ios::binary);

			if (!stream.is_open())
			{
				throw std::runtime_error("Failed to open file for deserialization");
			}

			boost::archive::binary_iarchive archive(stream);
			archive >> object;
			object->OnDeserialized();
			return boost::static_pointer_cast<T>(object);
		}
	};
} // namespace Engine