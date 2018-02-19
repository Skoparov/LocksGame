#include <iostream>

#include <QThread>
#include <QApplication>
#include <QMainWindow>

#include "mainwindow.h"
#include "model_controller.h"
#include "scores_manager.h"

struct game_settings
{
    size_t grid_size{ 3 };
    QSize image_size{ 100, 100 };
    size_t action_buffer_size{ 5 };
    size_t max_score_records{ 10 };
    QString scores_file_name{ "scores" };
};

game_settings get_settings( int argc, char** argv )
{
    enum args_pos{ grid_size_pos = 1,
                   image_size_pos,
                   action_buffer_size_pos,
                   max_scores_records_pos,
                   scores_file_name_pos };

    game_settings settings;

    if( argc >= grid_size_pos + 1 )
    {
        settings.grid_size = std::stoi( argv[ grid_size_pos ] );
    }

    if( argc >=  image_size_pos + 1 )
    {
        int image_size{ std::stoi( argv[ image_size_pos ] ) };
        settings.image_size.setHeight( image_size );
        settings.image_size.setWidth( image_size );
    }

    if( argc >=  action_buffer_size_pos + 1 )
    {
        settings.action_buffer_size = std::stoi( argv[ action_buffer_size_pos ] );
    }

    if( argc >=  max_scores_records_pos + 1 )
    {
        settings.max_score_records = std::stoi( argv[ max_scores_records_pos ] );
    }

    if( argc >=  scores_file_name_pos + 1 )
    {
        settings.scores_file_name = argv[ scores_file_name_pos ];
    }

    return settings;
}

int main(int argc, char *argv[])
{
    int return_code{ 0 };
    QApplication a{ argc, argv };

    QThread thread;

    try
    {
        game_settings settings{ get_settings( argc, argv ) };

        QStandardItemModel model;
        model_controller controller{ model, settings.grid_size, settings.action_buffer_size };
        controller.moveToThread( &thread );

        scores_manager manager{ settings.scores_file_name, settings.max_score_records };
        main_window w{ settings.image_size, model, manager };

        QObject::connect( w.get_view(),
                          SIGNAL( clicked( const QModelIndex& ) ),
                          &controller,
                          SLOT( on_click( const QModelIndex& ) ) );

        QObject::connect( &controller,
                          SIGNAL( index_changed( const QModelIndex& ) ),
                          w.get_view(),
                          SLOT( update( const QModelIndex& ) ) );

        QObject::connect( &controller,
                          SIGNAL( victory( int ) ),
                          &w,
                          SLOT( victory( int ) ) );

        QObject::connect( &w,
                          SIGNAL( restart() ),
                          &controller,
                          SLOT( start_new_game() ) );

        QObject::connect( &w,
                          SIGNAL( undo() ),
                          &controller,
                          SLOT( undo() ) );

        QObject::connect( &w,
                          SIGNAL( redo() ),
                          &controller,
                          SLOT( redo() ) );

        thread.start();
        w.show();
        return_code = a.exec();

        if( thread.isRunning() )
        {
            thread.quit();
            thread.wait();
        }
    }
    catch( const std::exception& e )
    {
        std::cerr << e.what() << std::endl;
        return_code = -1;
    }

    return return_code;
 }
