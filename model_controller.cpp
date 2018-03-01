#include "model_controller.h"

#include <random>
#include <thread>

model_controller::model_controller( QStandardItemModel& model,
                                    size_t grid_size,
                                    size_t action_buffer_size,
                                    QObject* parent ) :
    QObject( parent ),
    m_model( model ),
    m_actions( action_buffer_size )
{
    if( grid_size <= 0 )
    {
        throw std::invalid_argument{ "Grid size should be positive" };
    }

    m_model.setRowCount( static_cast< int >( grid_size ) + 1 );
    m_model.setColumnCount( static_cast< int >( grid_size ) );

    start_new_game();
}

QStandardItemModel& model_controller::get_model() const noexcept
{
    return m_model;
}

void model_controller::start_new_game()
{
    m_total_actions = 0;

    static std::mt19937 rng{ std::random_device{}() };
    static std::uniform_int_distribution< std::mt19937::result_type > dist{ 0, 1 };

    // Fill model with random values
    for( int row{ 0 }; row < m_model.rowCount(); ++row )
    {
        for( int col{ 0 }; col < m_model.columnCount(); ++col )
        {
            QModelIndex index{ m_model.index( row, col ) };

            if( row >= first_switch_row_pos )
            {
                data_state state{ dist( rng )?
                                data_state::switch_vertical : data_state::switch_horizontal };

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

void model_controller::on_click( const QModelIndex& index )
{
    if( index.row() >= first_switch_row_pos && !m_swaps_to_be_completed )
    {
        ++m_total_actions;
        m_actions.push( action{ index.row(), index.column() } );

        start_swap_switch_states( index );
    }
}

void model_controller::undo()
{
    if( m_actions.has_prev() && !m_swaps_to_be_completed )
    {
        --m_total_actions;

        const action& prev = m_actions.prev();
        QModelIndex index{ m_model.index( prev.first, prev.second ) };
        start_swap_switch_states( index );
    }
}

void model_controller::redo()
{
    if( m_actions.has_next() && !m_swaps_to_be_completed )
    {
        ++m_total_actions;

        const action& prev = m_actions.next();
        QModelIndex index{ m_model.index( prev.first, prev.second ) };
        start_swap_switch_states( index );
    }
}

void model_controller::maybe_add_child( const QModelIndex& index, const move_direction& direction )
{
    if( direction == move_direction::left && index.column() - 1 >= 0 )
    {
        m_swap_queue.push_back( { m_model.index( index.row(), index.column() - 1 ),
                           direction } );
    }
    else if( direction == move_direction::right &&
             index.column() + 1 < m_model.columnCount() )
    {
        m_swap_queue.push_back( { m_model.index( index.row(), index.column() + 1 ),
                           direction } );
    }
    else if( direction == move_direction::top && index.row() - 1 > lock_row_pos )
    {
        m_swap_queue.push_back( { m_model.index( index.row() - 1, index.column() ),
                           direction } );
    }
    else if( direction == move_direction::bottom &&
             index.row() + 1 < m_model.rowCount() )
    {
        m_swap_queue.push_back( { m_model.index( index.row() + 1, index.column() ),
                           direction } );
    }
}

uint16_t get_distance_from_root( const QPair< int, int >& root, const QModelIndex& curr )
{
    return root.first == curr.row()?
                std::abs( root.second - curr.column() ) :
                std::abs( root.first - curr.row() );
}

void model_controller::start_swap_switch_states( const QModelIndex& start_index )
{
    maybe_add_child( start_index, move_direction::left );
    maybe_add_child( start_index, move_direction::right );
    maybe_add_child( start_index, move_direction::top );
    maybe_add_child( start_index, move_direction::bottom );

    m_last_distance_from_root = 1;
    m_swaps_to_be_completed = 1;
    m_current_root = { start_index.row(), start_index.column() };

    swap_switch_state( start_index );
}

void model_controller::swap_animation_complete()
{
    if( m_swaps_to_be_completed )
    {
        --m_swaps_to_be_completed;
        if( !m_swaps_to_be_completed )
        {
            while( !m_swap_queue.empty() )
            {
                auto& index_and_direction = m_swap_queue.front();

                // Every time we increase the dist from root, wait for animation to finish
                int curr_distance{ get_distance_from_root( m_current_root, index_and_direction.first ) };
                if( curr_distance > m_last_distance_from_root )
                {
                    m_last_distance_from_root = curr_distance;
                    break;
                }

                ++m_swaps_to_be_completed;

                swap_switch_state( index_and_direction.first );
                maybe_add_child( index_and_direction.first, index_and_direction.second );

                m_swap_queue.pop_front();
            }

            update_locks();
        }
    }
}

void model_controller::set_state( const data_state& state, const QModelIndex& index )
{
    m_model.setData( index, as_int( state ), Qt::UserRole );
    emit index_changed( index );
}

void model_controller::swap_switch_state( const QModelIndex& index )
{
    auto curr_state = as_enum< data_state >( index.data( Qt::UserRole ).toInt() );
    data_state new_state{ curr_state == data_state::switch_horizontal?
                    data_state::switch_vertical : data_state::switch_horizontal };

    set_state( new_state, index );
}

uint32_t model_controller::calc_score() const noexcept
{
    return double( m_model.columnCount() * 100 ) / m_total_actions;
}

void model_controller::update_locks()
{
    uint16_t unlocked_locks_num{ 0 };

    for( int col{ 0 }; col < m_model.columnCount(); ++col )
    {
        bool has_vertical_switches{ false };
        for( int row{ first_switch_row_pos }; row < m_model.rowCount(); ++row )
        {
            QModelIndex index{ m_model.index( row, col ) };
            auto switch_state = as_enum< data_state >( index.data( Qt::UserRole ).toInt() );

            if( switch_state == data_state::switch_vertical )
            {
                has_vertical_switches = true;
                break;
            }
        }

        QModelIndex index{ m_model.index( lock_row_pos, col ) };
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
            ++unlocked_locks_num;
        }
    }

    if( unlocked_locks_num == m_model.columnCount() )
    {
        emit victory( calc_score() );
    }
}
