#include "mainwindow.h"

#include <QMenuBar>
#include <QScrollBar>
#include <QHeaderView>
#include <QMessageBox>

#include "scores_manager.h"
#include "graphics_delegate.h"

enum col_type{ score_pos_col, score_value_col };

main_window::main_window( const QSize& images_size,
                          QStandardItemModel& model,
                          scores_manager& manager,
                          QWidget* parent ):
      QMainWindow( parent ),
      m_manager( manager )
{
    create_menus();
    create_scores_widget();
    create_view( model, images_size );

    setCentralWidget( m_game_view );
}

QTableView* main_window::get_view() const noexcept
{
    return m_game_view;
}

void main_window::victory( int score )
{
    m_manager.on_victory( score );
    QMessageBox::StandardButton reply;

    reply = QMessageBox::question( this,
                                   "Victory!", QString{ "Your score: %1. Start new game?" }.arg( score ),
                                   QMessageBox::Yes | QMessageBox::No );

    if( reply == QMessageBox::Yes )
    {
        emit restart();
    }
}

void main_window::show_scores()
{
    auto& scores = m_manager.get_scores();

    for( int row{ 0 }; row < m_scores_widget->rowCount(); ++row )
    {
       QTableWidgetItem* item{ m_scores_widget->item( row, score_value_col ) };
       item->setText( QString{ "%1" }.arg( scores[ m_scores_widget->rowCount() - row - 1 ] ) );
    }

    m_scores_widget->show();
}

void main_window::create_view( QStandardItemModel& model, const QSize& images_size )
{
    m_game_view = new QTableView( this );
    m_game_view->setModel( &model );
    m_game_view->setEditTriggers( QAbstractItemView::NoEditTriggers );
    m_game_view->setShowGrid( false );
    m_game_view->horizontalHeader()->hide();
    m_game_view->verticalHeader()->hide();
    m_game_view->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    m_game_view->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    m_game_view->setFocusPolicy( Qt::NoFocus );

    graphics_delegate* del{ new graphics_delegate( images_size, *m_game_view, this ) };
    m_game_view->setItemDelegate( del );

    m_game_view->resizeColumnsToContents();
    m_game_view->resizeRowsToContents();

    m_game_view->setFixedSize( QSize{ m_game_view->columnWidth( 0 ) * model.columnCount() ,
                                      m_game_view->rowHeight( 0 ) * model.rowCount() } );
}

void main_window::create_menus()
{
    m_restart_action = new QAction( "Restart", this);
    m_undo_action = new QAction( "Undo", this);
    m_redo_action = new QAction( "Redo", this);
    m_top_list_action = new QAction( "Scores", this);

    connect( m_restart_action, &QAction::triggered, this, &main_window::restart );
    connect( m_undo_action, &QAction::triggered, this, &main_window::undo );
    connect( m_redo_action, &QAction::triggered, this, &main_window::redo );
    connect( m_top_list_action, &QAction::triggered, this, &main_window::show_scores );

    m_menu = menuBar()->addMenu( "Menu" );
    m_menu->addAction( m_restart_action );
    m_menu->addAction( m_undo_action );
    m_menu->addAction( m_redo_action );
    m_menu->addAction( m_top_list_action );
}

void main_window::create_scores_widget()
{
    if( !m_scores_widget )
    {
        QFont font;
        font.setPointSize( 15 );
        font.setFamily( "Arial" );
        int max_scores = m_manager.max_scores_num();

        m_scores_widget = new QTableWidget{ this };
        m_scores_widget->setRowCount( max_scores );
        m_scores_widget->setColumnCount( 2 );

        m_scores_widget->setWindowFlag( Qt::Window );
        m_scores_widget->setWindowTitle( "Top scores" );
        m_scores_widget->setHorizontalHeaderLabels( QStringList{} << "Position" << "Score" );
        m_scores_widget->verticalHeader()->hide();

        m_scores_widget->resizeColumnsToContents();
        m_scores_widget->resizeRowsToContents();

        m_scores_widget->setEditTriggers( QAbstractItemView::NoEditTriggers );
        m_scores_widget->setShowGrid( false );

        m_scores_widget->setFixedWidth( m_scores_widget->horizontalHeader()->width() +
                                        m_scores_widget->columnWidth( score_pos_col ) +
                                        m_scores_widget->columnWidth( score_value_col ) );

        m_scores_widget->setFixedHeight( m_scores_widget->verticalHeader()->height() +
                                        m_scores_widget->rowHeight( 0 ) * ( max_scores + 1 ) );

        for( int row{ 0 }; row < m_scores_widget->rowCount(); ++row )
        {
            auto item_pos = new QTableWidgetItem{ QString( "#%1" ).arg( row+1 ) };
            item_pos->setFont( font );
            m_scores_widget->setItem( row, score_pos_col, item_pos );

            auto item_score = new QTableWidgetItem{ QString{} };
            item_score->setFont( font );
            m_scores_widget->setItem( row, score_value_col, item_score );
        }
    }
}
