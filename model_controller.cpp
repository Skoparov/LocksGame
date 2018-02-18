#include "model_controller.h"

#include <random>
#include <thread>
#include <type_traits>

#include <QPair>
#include <QQueue>

static constexpr auto img_name_switch_vertical = "vertical_256_256";
static constexpr auto img_name_switch_horizontal = "horizontal_256_256";
static constexpr auto img_name_lock_locked = "lock_locked_256_256";
static constexpr auto img_name_lock_unlocked = "lock_unlocked_256_256";
static constexpr uint32_t pause_between_switches_msec{ 150 };

enum{ lock_row, first_switch_row };

template< typename enum_type,
          typename int_type = typename std::underlying_type< enum_type >::type >
constexpr int_type as_int( const enum_type& value )
{
    static_assert( std::is_integral< int_type >::value, "Underlying type should be integral" );
    return static_cast< int_type >( value );
}

template<  typename enum_type, typename int_type >
enum_type as_enum( int_type value )
{
    static_assert( std::is_integral< int_type >::value, "Underlying type should be integral" );
    static_assert( std::is_enum< enum_type >::value, "Destination type should be enum" );
    return static_cast< enum_type >( value );
}

model_controller::model_controller( QStandardItemModel& model,
                                    const game_settings& settings ,
                                    QObject* parent ) :
    QObject( parent ),
    m_model( model ),
    m_actions( settings.action_buffer )
{
    if( settings.grid_size <= 0 )
    {
        throw std::invalid_argument{ "Grid size should be positive" };
    }

    if( settings.image_size.width() <= 0 || settings.image_size.height() <= 0)
    {
        throw std::invalid_argument{ "Image size should be positive" };
    }

    init( settings.grid_size, settings.image_size );
    start_new_game();
}

void model_controller::start_new_game()
{
    std::mt19937 rng{ std::random_device{}() };
    std::uniform_int_distribution< std::mt19937::result_type > dist{ 0, 1 };

    for( int row{ 0 }; row < m_model.rowCount(); ++row )
    {
        for( int col{ 0 }; col < m_model.columnCount(); ++col )
        {
            QModelIndex index{ m_model.index( row, col ) };
            m_model.setItem( index.row(), index.column(), new QStandardItem{} );

            if( row >= first_switch_row )
            {
                data_state state{ dist( rng )? data_state::switch_horizontal : data_state::switch_vertical };
                set_state( state, index );
            }
            else
            {
                set_state( data_state::lock_locked, index );
            }
        }
    }

    update_locks();
}

void model_controller::click( const QModelIndex& index )
{
    ++m_total_actions;
    m_actions.push( action{ index.row(), index.column() } );

    swap_switches( index );
}

void model_controller::undo()
{
    if( m_actions.has_prev() )
    {
        --m_total_actions;

        const action& prev = m_actions.prev();
        QModelIndex index{ m_model.index( prev.first, prev.second ) };
        swap_switches( index );
    }
}

void model_controller::redo()
{
    if( m_actions.has_next() )
    {
        ++m_total_actions;

        const action& prev = m_actions.next();
        QModelIndex index{ m_model.index( prev.first, prev.second ) };
        swap_switches( index );
    }
}

enum class move_direction{ left, right, top, bottom };

void maybe_add_child( const QModelIndex& index,
                      QStandardItemModel& model,
                      const move_direction& direction,
                      QQueue< QPair< QModelIndex, move_direction > >& queue )
{
    if( direction == move_direction::left && index.column() - 1 >= 0 )
    {
        queue.push_back( { model.index( index.row(), index.column() - 1 ),
                           direction } );
    }
    else if( direction == move_direction::right &&
             index.column() + 1 < model.columnCount() )
    {
        queue.push_back( { model.index( index.row(), index.column() + 1 ),
                           direction } );
    }
    else if( direction == move_direction::top && index.row() - 1 > lock_row )
    {
        queue.push_back( { model.index( index.row() - 1, index.column() ),
                           direction } );
    }
    else if( direction == move_direction::bottom &&
             index.row() + 1 < model.rowCount() )
    {
        queue.push_back( { model.index( index.row() + 1, index.column() ),
                           direction } );
    }
}

void model_controller::swap_switches( const QModelIndex& start_index )
{
    if( start_index.row() >= first_switch_row )
    {
        std::lock_guard< std::mutex > l{ m_mutex };
        swap_switch_state( start_index );

        QQueue< QPair< QModelIndex, move_direction > > queue;
        maybe_add_child( start_index, m_model, move_direction::left, queue );
        maybe_add_child( start_index, m_model, move_direction::right, queue );
        maybe_add_child( start_index, m_model, move_direction::top, queue );
        maybe_add_child( start_index, m_model, move_direction::bottom, queue );

        static constexpr uint8_t group_size{ 4 };
        uint8_t switches_swapped{ group_size };

        while( !queue.empty() )
        {
            if( switches_swapped == group_size )
            {
                std::this_thread::sleep_for( std::chrono::milliseconds{ pause_between_switches_msec } );
                switches_swapped = 0;
            }

            auto& index_and_direction = queue.front();
            swap_switch_state( index_and_direction.first );
            maybe_add_child( index_and_direction.first, m_model, index_and_direction.second, queue );

            queue.pop_front();
            ++switches_swapped;
        }

        update_locks();

        if( switches_swapped == group_size )
        {
            std::this_thread::sleep_for( std::chrono::milliseconds{ pause_between_switches_msec } );
        }
    }
}

void model_controller::set_state( const model_controller::data_state& state, const QModelIndex& index )
{
    QStandardItem* item{ m_model.item( index.row(), index.column() ) };
    item->setData( m_pixmaps[ state ], Qt::DecorationRole );
    item->setData( as_int( state ), Qt::UserRole );
    emit index_changed( index );
}

void model_controller::init( int grid_size , const QSize& image_size )
{
    add_pixmap( data_state::switch_vertical, image_size );
    add_pixmap( data_state::switch_horizontal, image_size );
    add_pixmap( data_state::lock_locked, image_size );
    add_pixmap( data_state::lock_unlocked, image_size );

    m_model.setRowCount( grid_size + 1 );
    m_model.setColumnCount( grid_size );
}

void model_controller::swap_switch_state( const QModelIndex& index )
{
    auto curr_state = as_enum< data_state >( index.data( Qt::UserRole ).toInt() );
    data_state new_state{ curr_state == data_state::switch_horizontal?
                    data_state::switch_vertical : data_state::switch_horizontal };

    set_state( new_state, index );
}

void model_controller::add_pixmap( const data_state& state, const QSize& size )
{
    m_pixmaps.insert( state,
                      QPixmap::fromImage( QImage{ get_image_name( state ) } )
                      .scaled( size ) );
}

QString model_controller::get_image_name( const data_state& state )
{
    QString img_name;

    switch( state )
    {
    case data_state::switch_horizontal: img_name = img_name_switch_horizontal; break;
    case data_state::switch_vertical: img_name = img_name_switch_vertical; break;
    case data_state::lock_locked: img_name = img_name_lock_locked; break;
    case data_state::lock_unlocked: img_name = img_name_lock_unlocked; break;
    }

    return QString( ":/graphics/%1.png" ).arg( img_name );
}

uint32_t model_controller::calc_score() const noexcept
{
    return double{ 1 } / std::max( m_total_actions, uint32_t{ 1 } ) * 100;
}

void model_controller::update_locks()
{
    uint16_t locks_unlocked{ 0 };

    for( int col{ 0 }; col < m_model.columnCount(); ++col )
    {
        bool has_vertical_switches{ false };
        for( int row{ first_switch_row }; row < m_model.rowCount(); ++ row )
        {
            QModelIndex index{ m_model.index( row, col ) };
            auto switch_state = as_enum< data_state >( index.data( Qt::UserRole ).toInt() );

            if( switch_state == data_state::switch_vertical )
            {
                has_vertical_switches = true;
                break;
            }
        }

        QModelIndex index{ m_model.index( lock_row, col ) };
        auto lock_state = as_enum< data_state >( index.data( Qt::UserRole ).toInt() );
        if( has_vertical_switches && lock_state == data_state::lock_unlocked )
        {
            set_state( data_state::lock_locked, index );
        }
        else if( !has_vertical_switches && lock_state == data_state::lock_locked )
        {
            set_state( data_state::lock_unlocked, index );
        }

        if( !has_vertical_switches )
        {
            ++locks_unlocked;
        }
    }

    if( locks_unlocked == m_model.columnCount() )
    {
        emit victory( calc_score() );
    }
}
