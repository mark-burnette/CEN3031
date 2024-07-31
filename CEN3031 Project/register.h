#pragma once

#include "mysql_connection.h"
#include <cppconn/connection.h>

extern std::string hash_password(const std::string& password);
extern void register_role(sql::Connection* con, const std::string& role, char* username, size_t username_length, char* password, size_t password_length);
extern sql::ResultSet* login_register(sql::Connection* con);
extern sql::ResultSet* user;
