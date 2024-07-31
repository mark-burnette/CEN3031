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
