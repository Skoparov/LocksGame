#ifndef MODEL_CONTROLLER_H
#define MODEL_CONTROLLER_H

#include <mutex>
#include <deque>

#include <QTableView>
#include <QStandardItemModel>

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

struct game_settings
{
    int grid_size{ 3 };
    QSize image_size{ 100, 100 };
    size_t action_buffer{ 5 };
};

class model_controller : public QObject
{
    Q_OBJECT

    enum class data_state{ switch_horizontal, switch_vertical, lock_locked, lock_unlocked };
    using action = QPair< int, int >;

public:
    model_controller( QStandardItemModel& model,
                      const game_settings& settings,
                      QObject* parent = nullptr );

public slots:
    void start_new_game();
    void click( const QModelIndex& index );
    void undo();
    void redo();

signals:
    void data_changed( const QModelIndex&, const QModelIndex& );
    void index_changed( const QModelIndex& );
    void victory( int score );

private:
    void init( int grid_size, const QSize& image_size );
    void swap_switches( const QModelIndex& start_index );
    void update_locks();

    void swap_switch_state( const QModelIndex& index );
    void set_state( const data_state& state, const QModelIndex& index );

    void add_pixmap( const data_state& state, const QSize& size );
    QString get_image_name( const data_state& state );
    uint32_t calc_score() const noexcept;

private:
    QStandardItemModel& m_model;
    QMap< data_state, QPixmap > m_pixmaps;
    traversible_circular_buffer< action > m_actions;
    uint32_t m_total_actions{ 0 };

    std::mutex m_mutex;
};

#endif
