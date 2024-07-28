#include <iostream>

#include <windows.h>
#include <urlmon.h>

#include "imgui.h"
#include "list.h"

sql::ResultSet* search_form(sql::Connection* con)
{
    static char title[64] = {};

    ImGui::Text("Search for a book:");
    ImGui::InputText("Title", title, sizeof(title));

    sql::Statement* stmt = nullptr;
    sql::PreparedStatement* pstmt = nullptr;
    sql::ResultSet* search_results = nullptr;

    if (!ImGui::Button("Search"))
    {
        // search not yet attempted
        return nullptr;
    }

    try
    {
        pstmt = con->prepareStatement("SELECT * FROM books WHERE `Book-Title` LIKE ?");
        std::string _title{"%"};
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
        return nullptr;
    }

    delete pstmt;
    return search_results;
}
