#pragma once

#include <cppconn/connection.h>
#include <cppconn/resultset.h>

void panel(sql::Connection* con, sql::ResultSet* user);
