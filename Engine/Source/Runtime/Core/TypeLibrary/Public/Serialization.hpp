#pragma once

namespace boost::serialization 
{
	template <typename Archive>
	void serialize(Archive& ar, Engine::BufferUAVDescription& x, const unsigned int version)
	{
		ar& FirstElement;
		ar& NumElements;
		ar& StructureByteStride;
		ar& CounterOffsetInBytes;
		ar& Flags;
	}
}