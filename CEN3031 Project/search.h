#pragma once

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>

sql::ResultSet* search_form(sql::Connection* con);
