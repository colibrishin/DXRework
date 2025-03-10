#include "NetworkType.h"

Engine::RawNetMessage::RawNetMessage( const void* data, const size_t size )
{
    m_raw_data_.resize( size );
    SIMDExtension::_mm256_memcpy( m_raw_data_.data(), data, size );
}

const unsigned char* Engine::RawNetMessage::data() const
{
    return m_raw_data_.data();
}

const unsigned char* Engine::RawNetMessage::data_without_header() const
{
    return m_raw_data_.data() + sizeof( NetMessageHeaderType );
}

size_t Engine::RawNetMessage::size() const
{
    return m_raw_data_.size();
}

size_t Engine::RawNetMessage::size_without_header() const
{
    return m_raw_data_.size() - sizeof( NetMessageHeaderType );
}

bool Engine::RawNetMessage::operator==( const RawNetMessage& other ) const noexcept
{
    static std::hash<RawNetMessage> hasher{};
    return hasher( other );
}
