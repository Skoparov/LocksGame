#ifndef MOVIE_DELEGATE_HPP
#define MOVIE_DELEGATE_HPP

#include <QMap>
#include <QMovie>
#include <QAbstractItemView>
#include <QStyledItemDelegate>

#include "common.h"

// Paints animations and images instead of data_state values

class graphics_delegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    graphics_delegate( const QSize& image_size, QAbstractItemView& view, QObject* parent = nullptr );

    void paint( QPainter* painter,
                const QStyleOptionViewItem& option,
                const QModelIndex& index ) const override;

    QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const override;

signals:
    void animation_completed();

private:
    void init();
    QMovie* get_movie( const QModelIndex& index, const data_state& state ) const;
    QPixmap* get_pixmap( const data_state& state ) const;

private:
    QAbstractItemView& m_view;

    QSize m_image_size;
    mutable QMap< data_state, QPixmap > m_lock_pixmaps;
    mutable QMap< QModelIndex, QPair< QMovie*, QMovie* > > m_switch_movies;
};

#endif
