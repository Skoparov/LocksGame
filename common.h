#ifndef COMMON_H
#define COMMON_H

#include <type_traits>

enum class data_state{ switch_horizontal, switch_vertical, lock_locked, lock_unlocked };
enum rows_pos{ lock_row_pos, first_switch_row_pos };

template<  typename enum_type, typename int_type >
enum_type as_enum( int_type value )
{
    static_assert( std::is_integral< int_type >::value, "Underlying type should be integral" );
    static_assert( std::is_enum< enum_type >::value, "Destination type should be enum" );

    return static_cast< enum_type >( value );
}

template< typename enum_type,
          typename int_type = typename std::underlying_type< enum_type >::type >
constexpr int_type as_int( const enum_type& value )
{
    static_assert( std::is_integral< int_type >::value, "Underlying type should be integral" );

    return static_cast< int_type >( value );
}

#endif
