#include <iostream>

#include <windows.h>
#include <urlmon.h>
#include <sstream>
#include <vector>

#include "imgui.h"
#include "list.h"

sql::ResultSet* search_form(sql::Connection* con)
{
    static char title[256] = {};

    ImGui::SeparatorText("Search for a book:");
    ImGui::InputText("Title", title, sizeof(title));

    // filter description
    static char desc[1024] = {};
    ImGui::InputText("Description", desc, sizeof(desc));

    // filter location
    static char location[2] = {};
	ImGui::InputText("Location (1, 2, or 3)", location, 2);

    // filter last checked out by
    static char last_checked_out[32] = {};
	ImGui::InputText("Last checked out by", last_checked_out, 32);

    // filter # of copies
    static char num_copies[3] = "0";
	ImGui::InputText("# of copies >=", num_copies, 3);

    // filter genre(s)
    const char* genres[] = { "", "Art", "Biography", "Business", "Children's", "Classics", "Contemporary", "Cookbooks", "Crime", "Fantasy", "Fiction", "Graphic Novels", "Historical Fiction", "History", "Horror", "Memoir", "Music", "Mystery", "Nonfiction", "Philosophy", "Poetry", "Religion", "Romance", "Science", "Science Fiction", "Self Help", "Young Adult" };
    const int num_genres = IM_ARRAYSIZE(genres);

	static int genre_current_idx = 0;

	if (ImGui::BeginCombo("Genre", genres[genre_current_idx]))
	{
		for (int n = 0; n < IM_ARRAYSIZE(genres); n++)
		{
			const bool is_selected = (genre_current_idx == n);
			if (ImGui::Selectable(genres[n], is_selected))
				genre_current_idx = n;

			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	// filter availability
	static char date[11] = {};
    if (date[0] == '\0')
    {
        std::time_t time = std::time(nullptr);
        std::tm local_time;
        localtime_s(&local_time, &time);
        strftime(date, 11, "%Y-%m-%d", &local_time);
    }

	ImGui::SetNextItemWidth(ImGui::CalcTextSize("(YYYY-MM-DD)").x);
	ImGui::InputText("Available by (YYYY-MM-DD)", date, 11);

    sql::PreparedStatement* pstmt = nullptr;
    static sql::ResultSet* search_results = nullptr;

    if (ImGui::Button("Search"))
    {
        try
        {
            std::string query = "SELECT * FROM books WHERE `Book-Title` LIKE ?";
            std::vector<std::string> params{};

            // title
            std::string _title{ "%" };
            _title.append(title);
            _title.append("%");
            params.push_back(_title);

            // description
            std::stringstream desc_stream{ desc };

            while (desc_stream.good())
            {
                std::string tmp{};
                desc_stream >> tmp;

                query += " AND Summary LIKE ?";
                params.push_back(std::string("%") + tmp + std::string("%"));
            }

            // genres
            if (genre_current_idx != 0)
            {
                query += " AND Genre LIKE ?";
                params.push_back(genres[genre_current_idx]);
            }

            // expiration date
            query += " AND (`expiration-date` IS NULL OR `expiration-date` < ?)";
            params.push_back(date);

            // # of copies
            query += " AND num_copies >= ?";
            params.push_back(num_copies);

            // location
            if (location[0] != '\0')
            {
				query += " AND location = ?";
				params.push_back(std::string("Location ") + location);
			}

            // last checked out by
            if (last_checked_out[0] != '\0')
            {
                sql::PreparedStatement* tmp_pstmt = con->prepareStatement("SELECT id FROM users WHERE username = ?");
                tmp_pstmt->setString(1, last_checked_out);
                tmp_pstmt->execute();

                sql::ResultSet* tmp_res = tmp_pstmt->getResultSet();

                tmp_res->first();

                query += " AND `last-checked-out` = ?";
                params.push_back(std::to_string(tmp_res->getInt("id")));

                delete tmp_pstmt;
                delete tmp_res;
            }

            pstmt = con->prepareStatement(query.c_str());

            for (int i = 0; i < params.size(); i++)
                pstmt->setString(i + 1, params[i].c_str());

            pstmt->execute();
            search_results = pstmt->getResultSet();

            list = true;
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

    // filter description
    static char desc[1024] = {};
    ImGui::InputText("Description", desc, sizeof(desc));

    // filter location
    static char location[2] = {};
	ImGui::InputText("Location (1, 2, or 3)", location, 2);

    // filter last checked out by
    static char last_checked_out[32] = {};
	ImGui::InputText("Last checked out by", last_checked_out, 32);

    // filter # of copies
    static char num_copies[3] = "0";
	ImGui::InputText("# of copies >=", num_copies, 3);

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

	// filter availability
	static char date[11] = {};
    if (date[0] == '\0')
    {
        std::time_t time = std::time(nullptr);
        std::tm local_time;
        localtime_s(&local_time, &time);
        strftime(date, 11, "%Y-%m-%d", &local_time);
    }
    
	ImGui::SetNextItemWidth(ImGui::CalcTextSize("(YYYY-MM-DD)").x);
	ImGui::InputText("Available by (YYYY-MM-DD)", date, 11);

    sql::PreparedStatement* pstmt = nullptr;
    static sql::ResultSet* search_results = nullptr;

    if (ImGui::Button("Search"))
    {
        try
        {
            std::string query = "SELECT * FROM movies WHERE `Title` LIKE ?";
            std::vector<std::string> params{};

            // title
            std::string _title{ "%" };
            _title.append(title);
            _title.append("%");
            params.push_back(_title);

            // description
            std::stringstream desc_stream{ desc };

            while (desc_stream.good())
            {
                std::string tmp{};
                desc_stream >> tmp;

                query += " AND Summary LIKE ?";
                params.push_back(std::string("%") + tmp + std::string("%"));
            }

            // genres
            std::sort(selected_genres.begin(), selected_genres.end());

            std::string genres_str{};
            if (selected_genres.size() > 0)
            {
                query += " AND Genre LIKE ?";
                for (int i : selected_genres)
                {
                    genres_str += "%"; // SQL wildcard; matches zero or more characters
                    genres_str += genres[i];
                    genres_str += "%";
                }
                params.push_back(genres_str);
            }

            // expiration date
            query += " AND (`expiration-date` IS NULL OR `expiration-date` < ?)";
            params.push_back(date);

            // # of copies
            query += " AND num_copies >= ?";
            params.push_back(num_copies);

            // location
            if (location[0] != '\0')
            {
				query += " AND location = ?";
				params.push_back(std::string("Location ") + location);
			}

            // last checked out by
            if (last_checked_out[0] != '\0')
            {
                sql::PreparedStatement* tmp_pstmt = con->prepareStatement("SELECT id FROM users WHERE username = ?");
                tmp_pstmt->setString(1, last_checked_out);
                tmp_pstmt->execute();

                sql::ResultSet* tmp_res = tmp_pstmt->getResultSet();

                tmp_res->first();

                query += " AND `last-checked-out` = ?";
                params.push_back(std::to_string(tmp_res->getInt("id")));

                delete tmp_pstmt;
                delete tmp_res;
            }

            pstmt = con->prepareStatement(query.c_str());

            for (int i = 0; i < params.size(); i++)
                pstmt->setString(i + 1, params[i].c_str());

            pstmt->execute();
            search_results = pstmt->getResultSet();

            list = true;
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