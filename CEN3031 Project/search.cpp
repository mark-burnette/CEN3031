#include <iostream>

#include <windows.h>
#include <urlmon.h>

#include "imgui.h"
#include "list.h"

sql::ResultSet* search_form(sql::Connection* con)
{
    static char title[256] = {};

    ImGui::SeparatorText("Search for a book:");
    ImGui::InputText("Title", title, sizeof(title));

    sql::PreparedStatement* pstmt = nullptr;
    static sql::ResultSet* search_results = nullptr;

    if (ImGui::Button("Search"))
    {
        try
        {
            pstmt = con->prepareStatement("SELECT * FROM books WHERE `Book-Title` LIKE ?");
            std::string _title{ "%" };
            _title.append(title);
            _title.append("%");
            pstmt->setString(1, _title.c_str());
            pstmt->execute();

            search_results = pstmt->getResultSet();

            list = true;

            memset(title, 0, sizeof(title));
        }
        catch (sql::SQLException e)
        {
            std::cout << "Error: " << e.what() << std::endl;

            delete pstmt;
            pstmt = nullptr;

            return nullptr;
        }
    }

    delete pstmt;
    pstmt = nullptr;

    return search_results;
}

sql::ResultSet* search_movies(sql::Connection* con)
{
    static char title[256] = {};

    // title
    ImGui::SeparatorText("Search for a movie:");
    ImGui::InputText("Title", title, sizeof(title));

    // genre(s)
    static std::vector<int> selected_genres;
    const char* genres[] = { "Action", "Adventure", "Animation", "Biography", "Comedy", "Crime", "Documentary", "Drama", "Family", "Fantasy", "History", "Horror", "Music", "Musical", "Mystery", "News", "Reality-TV", "Romance", "Sci-Fi", "Sport", "Thriller", "War", "Western" };
    const int num_genres = IM_ARRAYSIZE(genres);

    if (ImGui::BeginListBox("Genres"))
    {
        for (int i = 0; i < num_genres; ++i)
        {
            auto curr_genre = std::find(selected_genres.begin(), selected_genres.end(), i);
            bool isSelected = (curr_genre != selected_genres.end());
            if (ImGui::Selectable(genres[i], isSelected))
            {
                if (isSelected)
                    selected_genres.erase(curr_genre);
                else
                    selected_genres.push_back(i);
            }
        }
        ImGui::EndListBox();
    }

    sql::PreparedStatement* pstmt = nullptr;
    static sql::ResultSet* search_results = nullptr;

    if (ImGui::Button("Search"))
    {
        try
        {
            // consolidate genres
            std::sort(selected_genres.begin(), selected_genres.end()); // they're sorted in the database, so they need to be sorted here

            std::string genres_str{};
            for (int i : selected_genres)
            {
				genres_str += "%"; // SQL wildcard; matches zero or more characters
				genres_str += genres[i];
				genres_str += "%";
			}

            pstmt = con->prepareStatement("SELECT * FROM movies WHERE `Title` LIKE ? AND Genre LIKE ?");
            std::string _title{ "%" };
            _title.append(title);
            _title.append("%");
            pstmt->setString(1, _title.c_str());

            pstmt->setString(2, genres_str.c_str());

            pstmt->execute();
            search_results = pstmt->getResultSet();

            list = true;

            memset(title, 0, sizeof(title));
        }
        catch (sql::SQLException e)
        {
            std::cout << "Error: " << e.what() << std::endl;

            delete pstmt;
            pstmt = nullptr;

            return nullptr;
        }
    }

    delete pstmt;
    pstmt = nullptr;

    return search_results;
}