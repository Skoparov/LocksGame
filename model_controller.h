#ifndef MODEL_CONTROLLER_H
#define MODEL_CONTROLLER_H

#include <mutex>

#include <QStandardItemModel>

#include "common.h"
#include "traversible_circular_buffer.h"

class model_controller : public QObject
{
    Q_OBJECT

    using action = QPair< int, int >;

public:
    model_controller( QStandardItemModel& model,
                      size_t grid_size,
                      size_t action_buffer_size,
                      QObject* parent = nullptr );

public slots:
    void start_new_game();
    void on_click( const QModelIndex& index );
    void undo();
    void redo();

signals:
    void index_changed( const QModelIndex& );
    void victory( int score );

private:
    void update_locks();
    void swap_switch_group( const QModelIndex& start_index );
    void swap_switch_state( const QModelIndex& index );
    void set_state( const data_state& state, const QModelIndex& index );

    uint32_t calc_score() const noexcept;

private:
    QStandardItemModel& m_model;
    uint32_t m_total_actions{ 1 };
    traversible_circular_buffer< action > m_actions;

    std::mutex m_mutex;
};

#endif
