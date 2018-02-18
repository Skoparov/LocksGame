#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMenu>
#include <QAction>
#include <QTableView>
#include <QTableWidget>
#include <QMainWindow>
#include <QStandardItemModel>

class scores_manager;

class main_window : public QMainWindow
{
    Q_OBJECT

public:
    explicit main_window( QStandardItemModel& model,
                          scores_manager& manager,
                          QWidget *parent = 0 );

    QTableView* get_view() const noexcept;

public slots:
    void victory( int score );
    void show_scores();

signals:
    void restart();
    void undo();
    void redo();

private:
    scores_manager& m_manager;
    QTableView* m_game_view{ nullptr };
    QTableWidget* m_scores_widget{ nullptr };

    QMenu* m_menu{ nullptr };
    QAction* m_restart_action{ nullptr };
    QAction* m_undo_action{ nullptr };
    QAction* m_redo_action{ nullptr };
    QAction* m_top_list_action{ nullptr };
};

#endif // MAINWINDOW_H
