#ifndef TRAVERSIBLE_CIRCULAR_BUFFER_H
#define TRAVERSIBLE_CIRCULAR_BUFFER_H

#include <deque>

#include <QSize>

template< typename type >
class traversible_circular_buffer
{
public:
    explicit traversible_circular_buffer( size_t size ) noexcept : m_max_size( size ){}

    void push( const type& data )
    {
        if( m_pos != m_data.size() )
        {
            m_data.resize( m_pos );
        }

        m_data.push_back( data );
        if( m_data.size() > m_max_size )
        {
            m_data.pop_front();
        }

        m_pos = m_data.size();
    }

    const type& next()
    {
        if( !has_next() )
        {
            throw std::out_of_range{ "No next action" };
        }

        return m_data[ m_pos++ ];
    }

    const type& prev()
    {
        if( !has_prev() )
        {
            throw std::out_of_range{ "No prev action" };
        }

        return m_data[ --m_pos ];
    }

    bool has_next() const noexcept{ return m_pos < m_data.size(); }
    bool has_prev() const noexcept{ return m_pos > 0; }
    size_t max_size() const noexcept{ return m_max_size; }

private:
    size_t m_pos{ 0 };
    size_t m_max_size{ 0 };
    std::deque< type > m_data;
};

#endif
