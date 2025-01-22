#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <filesystem>
#include <string>

namespace boost::serialization
{
	template <class Archive>
	void serialize(Archive& ar, std::filesystem::path& p, const unsigned int version)
	{
		std::wstring s;
		if (Archive::is_saving::value)
			s = p.generic_wstring();
		ar& boost::serialization::make_nvp("wstring", s);
		if (Archive::is_loading::value)
			p = s;
	}
}