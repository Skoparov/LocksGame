#ifndef MODEL_CONTROLLER_H
#define MODEL_CONTROLLER_H

#include <QQueue>
#include <QStandardItemModel>

#include "common.h"
#include "traversible_circular_buffer.h"

// Manages switches' and locks' states

class model_controller : public QObject
{
    Q_OBJECT

    using action = QPair< int, int >;
    enum class move_direction{ left, right, top, bottom };

public:
    model_controller( QStandardItemModel& model,
                      size_t grid_size,
                      size_t action_buffer_size,
                      QObject* parent = nullptr );

    QStandardItemModel& get_model() const noexcept;

public slots:
    void start_new_game();
    void on_click( const QModelIndex& index );
    void undo();
    void redo();

    void swap_animation_complete();

signals:
    void index_changed( const QModelIndex& );
    void victory( int score );

private:
    void update_locks();
    uint32_t calc_score() const noexcept;
    void swap_switch_state( const QModelIndex& index );
    void start_swap_switch_states( const QModelIndex& start_index );
    void set_state( const data_state& state, const QModelIndex& index );
    void maybe_add_child( const QModelIndex& index, const move_direction& direction );

private:
    QStandardItemModel& m_model;

    // For score calculation
    uint32_t m_total_actions{ 1 };

    // Action buffer for undo/redo
    traversible_circular_buffer< action > m_actions;

    // Data required to switch switches (ugh) sequentially
    QPair< int, int > m_current_root{};
    uint16_t m_swaps_to_be_completed{ 0 };
    uint16_t m_last_distance_from_root{ 0 };
    QQueue< QPair< QModelIndex, move_direction > > m_swap_queue;
};

#endif
