#pragma once

#include "mysql_connection.h"
#include <cppconn/driver.h>

sql::ResultSet* login_register(sql::Connection* con);
extern sql::ResultSet* user;
