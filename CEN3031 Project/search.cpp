#include <iostream>

#include <windows.h>
#include <urlmon.h>

#include "imgui.h"
#include "list.h"
#include "search.h"

// TODO: use global var so that there's only one return of result set
sql::ResultSet* search_form()
{
    ImGui::Begin("Search");

    static char title[64] = {};

    ImGui::InputText("Title", title, sizeof(title));

    if (!ImGui::Button("Search"))
    {
        // search not yet attempted
        return nullptr;
    }
    ImGui::End();

    sql::Driver* driver = nullptr;
    sql::Connection* con = nullptr;
    sql::Statement* stmt = nullptr;
    sql::PreparedStatement* pstmt = nullptr;
    sql::ResultSet* search_results = nullptr;

    try
    {
        // TODO: connect to sql database once in main and pass these to functions as parameters
        driver = get_driver_instance();
        con = driver->connect(server, db_username, db_password);

        con->setSchema("test_db");

        // TODO: more than ISBN
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
        delete con;
        delete stmt;

        return nullptr;
    }

    delete pstmt;
    delete con;
    delete stmt;

    return search_results;
}
