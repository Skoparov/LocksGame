#include <iostream>

#include <QThread>
#include <QApplication>
#include <QMainWindow>

#include "mainwindow.h"
#include "model_controller.h"
#include "scores_manager.h"

game_settings get_game_settings( int argc, char** argv )
{
    enum args_pos{ grid_size_pos = 1, image_size_pos };

    game_settings settings;

    if( argc >= grid_size_pos + 1 )
    {
        settings.grid_size = std::stoi( argv[ grid_size_pos ] );

        if( argc >=  image_size_pos + 1 )
        {
            int image_size{ std::stoi( argv[ image_size_pos ] ) };
            settings.image_size.setHeight( image_size );
            settings.image_size.setWidth( image_size );
        }
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
        game_settings settings{ get_game_settings( argc, argv ) };

        QStandardItemModel model;
        model_controller controller{ model, settings };
        controller.moveToThread( &thread );

        scores_manager manager{ "scores", 10 };

        main_window w{ model, manager };

        QObject::connect( w.get_view(),
                          SIGNAL( clicked( const QModelIndex& ) ),
                          &controller,
                          SLOT( click( const QModelIndex& ) ), Qt::QueuedConnection );

        QObject::connect( &controller,
                          SIGNAL( data_changed( const QModelIndex&, const QModelIndex& ) ),
                          w.get_view(),
                          SLOT( dataChanged( const QModelIndex&, const QModelIndex& ) ), Qt::QueuedConnection );

        QObject::connect( &controller,
                          SIGNAL( index_changed( const QModelIndex& ) ),
                          w.get_view(),
                          SLOT( update( const QModelIndex& ) ), Qt::QueuedConnection );

        QObject::connect( &controller,
                          SIGNAL( victory( int ) ),
                          &w,
                          SLOT( victory( int ) ), Qt::QueuedConnection );

        QObject::connect( &w,
                          SIGNAL( restart() ),
                          &controller,
                          SLOT( start_new_game() ), Qt::QueuedConnection );

        QObject::connect( &w,
                          SIGNAL( undo() ),
                          &controller,
                          SLOT( undo() ), Qt::QueuedConnection );

        QObject::connect( &w,
                          SIGNAL( redo() ),
                          &controller,
                          SLOT( redo() ), Qt::QueuedConnection );

        thread.start();

        w.show();
        return_code = a.exec();
    }
    catch( const std::exception& e )
    {
        std::cerr << e.what() << std::endl;
        return_code = -1;
    }

    if( thread.isRunning() )
    {
        thread.quit();
        thread.wait();
    }

    return return_code;
 }
