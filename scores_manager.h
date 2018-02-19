#ifndef SCORES_MANAGER_H
#define SCORES_MANAGER_H

#include <deque>

#include <QString>

// Stores and saves top scores
class scores_manager
{
public:
    scores_manager( const QString& file_name, size_t max_records );
    void on_victory( int score );

    size_t max_scores_num() const noexcept;
    const std::deque< int >& get_scores() const noexcept;

private:
    void read_from_file();
    void save_to_file();

private:
    QString m_file_name;
    size_t m_max_recors{ 0 };
    std::deque< int > m_scores;
};

#endif
