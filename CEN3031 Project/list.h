#pragma once

#include <unordered_map>

#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>

#include <d3d11.h>

extern ID3D11Device* g_pd3dDevice;
extern bool list;
extern std::unordered_map<int, std::string> cache;
int listings(sql::ResultSet* search_results);
