#include "scores_manager.h"

#include <QFile>
#include <QDataStream>

scores_manager::scores_manager( const QString& file_name, size_t max_records ):
    m_file_name( file_name ),
    m_max_recors( max_records )
{
    read_from_file();
}

void scores_manager::on_victory( int score )
{
    m_scores.push_back( score );
    std::sort( m_scores.begin(), m_scores.end() );

    if( m_scores.size() > m_max_recors )
    {
        m_scores.pop_front();
    }

    save_to_file();
}

size_t scores_manager::max_scores_num() const noexcept
{
    return m_max_recors;
}

const std::deque< int >& scores_manager::get_scores() const noexcept
{
    return m_scores;
}

void scores_manager::save_to_file()
{
    QFile file{ m_file_name };
    file.open( QFile::ReadWrite );
    if( !file.isOpen() )
    {
        throw std::ios_base::failure{ "Failed to open file" };
    }

    QDataStream out{ &file };
    for( int score : m_scores )
    {
        out << score;
    }
}

void scores_manager::read_from_file()
{
    m_scores.clear();

    QFile file{ m_file_name };
    file.open( QFile::ReadOnly );
    if( file.isOpen() )
    {
        QDataStream out{ &file };
        for( size_t score_num{ 0 }; score_num < m_max_recors; ++score_num )
        {
            int score{ 0 };
            out >> score;
            m_scores.push_back( score );
        }
    }

    std::sort( m_scores.begin(), m_scores.end() );
}
