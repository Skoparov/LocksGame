#include "model_controller.h"

#include <random>
#include <thread>

#include <QQueue>

model_controller::model_controller(QStandardItemModel& model,
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

    m_model.setRowCount( grid_size + 1 );
    m_model.setColumnCount( grid_size );

    start_new_game();
}

void model_controller::start_new_game()
{
    std::mt19937 rng{ std::random_device{}() };
    std::uniform_int_distribution< std::mt19937::result_type > dist{ 0, 1 };

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
    ++m_total_actions;
    m_actions.push( action{ index.row(), index.column() } );

    swap_switch_group( index );
}

void model_controller::undo()
{
    if( m_actions.has_prev() )
    {
        --m_total_actions;

        const action& prev = m_actions.prev();
        QModelIndex index{ m_model.index( prev.first, prev.second ) };
        swap_switch_group( index );
    }
}

void model_controller::redo()
{
    if( m_actions.has_next() )
    {
        ++m_total_actions;

        const action& prev = m_actions.next();
        QModelIndex index{ m_model.index( prev.first, prev.second ) };
        swap_switch_group( index );
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
    else if( direction == move_direction::top && index.row() - 1 > lock_row_pos )
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

int get_distance_from_root( const QModelIndex& root, const QModelIndex& curr )
{
    return root.row() == curr.row()?
                std::abs( root.column() - curr.column() ) :
                std::abs( root.row() - curr.row() );
}

void model_controller::swap_switch_group( const QModelIndex& start_index )
{
    if( start_index.row() >= first_switch_row_pos )
    {
        std::lock_guard< std::mutex > l{ m_mutex };

        swap_switch_state( start_index );

        // We go from root in all 4 directions, switching all the switches
        // with the similar distance from root at a time
        QQueue< QPair< QModelIndex, move_direction > > queue;
        maybe_add_child( start_index, m_model, move_direction::left, queue );
        maybe_add_child( start_index, m_model, move_direction::right, queue );
        maybe_add_child( start_index, m_model, move_direction::top, queue );
        maybe_add_child( start_index, m_model, move_direction::bottom, queue );

        static uint32_t anim_duration{ 400 };

        int distance_from_root{ 0 };
        while( !queue.empty() )
        {
            auto& index_and_direction = queue.front();

            // Every time we increase the dist from root, wait for animation to finish
            int curr_distance( get_distance_from_root( start_index, index_and_direction.first ) );
            if( curr_distance > distance_from_root )
            {
                std::this_thread::sleep_for( std::chrono::milliseconds{ anim_duration } );
                distance_from_root = curr_distance;
            }

            swap_switch_state( index_and_direction.first );
            maybe_add_child( index_and_direction.first, m_model, index_and_direction.second, queue );

            queue.pop_front();
        }

        update_locks();
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
