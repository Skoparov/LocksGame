#include "graphics_delegate.h"

#include <QLabel>

static constexpr auto img_name_lock_locked = "lock_locked.png";
static constexpr auto img_name_lock_unlocked = "lock_unlocked.png";
static constexpr auto img_name_horizontal_vertical_anim = "horizontal_vertical.gif";
static constexpr auto img_name_vertical_horizontal_anim = "vertical_horizontal.gif";

QString get_image_name( const data_state& state )
{
    QString img_name;

    switch( state )
    {
    case data_state::lock_locked: img_name = img_name_lock_locked; break;
    case data_state::lock_unlocked: img_name = img_name_lock_unlocked; break;
    case data_state::switch_horizontal: img_name = img_name_vertical_horizontal_anim; break;
    case data_state::switch_vertical: img_name = img_name_horizontal_vertical_anim; break;
    }

    return QString( ":/graphics/%1" ).arg( img_name );
}

graphics_delegate::graphics_delegate( const QSize& image_size, QAbstractItemView& view, QObject* parent )
  : QStyledItemDelegate( parent ),
    m_view( view ),
    m_image_size( image_size )
{
    init();
}

void graphics_delegate::paint( QPainter * painter,
                           const QStyleOptionViewItem & option,
                           const QModelIndex & index ) const
{
    QStyledItemDelegate::paint( painter, option, index );

    auto curr_state = as_enum< data_state >( index.data( Qt::UserRole ).toInt() );
    QObject* index_widget{ m_view.indexWidget( index ) };
    QLabel* label{ qobject_cast< QLabel* >( index_widget ) };
    if( !label )
    {
        label = new QLabel{};
        m_view.setIndexWidget( index, label );
        label->setFixedSize( m_image_size );
    }

    if( index.row() == lock_row_pos )
    {
        QPixmap* required_pixmap{ get_pixmap( curr_state ) };
        if( label->pixmap() != required_pixmap )
        {
            label->setPixmap( *required_pixmap );
        }
    }
    else
    {
        QMovie* required_movie{ get_movie( index, curr_state ) };
        if( label->movie() != required_movie )
        {
            required_movie->jumpToFrame( 0 );
            label->setMovie( required_movie );
            required_movie->start();
        }
    }

    m_view.update( index );
}

QSize graphics_delegate::sizeHint( const QStyleOptionViewItem&, const QModelIndex& ) const
{
    return m_image_size;
}

void graphics_delegate::init()
{
    m_lock_pixmaps.insert( data_state::lock_locked,
                           QPixmap::fromImage( QImage{ get_image_name( data_state::lock_locked ) } )
                                           .scaled( m_image_size ) );

    m_lock_pixmaps.insert( data_state::lock_unlocked,
                           QPixmap::fromImage( QImage{ get_image_name( data_state::lock_unlocked ) } )
                                           .scaled( m_image_size ) );

    QAbstractItemModel* model{ m_view.model() };
    for( int row{ first_switch_row_pos }; row < model->rowCount(); ++row )
    {
        for( int col{ 0 }; col < model->columnCount(); ++col )
        {
            auto movie_vh = new QMovie{ get_image_name( data_state::switch_horizontal ), {}, this };
            auto movie_hv = new QMovie{ get_image_name( data_state::switch_vertical ), {}, this };

            movie_hv->setScaledSize( m_image_size );
            movie_vh->setScaledSize( m_image_size );

            m_switch_movies.insert( model->index( row, col ), { movie_vh, movie_hv } );
        }
    }
}

QMovie* graphics_delegate::get_movie( const QModelIndex &index, const data_state& state ) const
{
    return state == data_state::switch_horizontal?
                m_switch_movies[ index ].first : m_switch_movies[ index ].second;
}

QPixmap* graphics_delegate::get_pixmap( const data_state& state ) const
{
    return &m_lock_pixmaps[ state ];
}

