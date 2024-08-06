#pragma once

#include <cppconn/connection.h>
#include <cppconn/resultset.h>
#include "calendar.h"

void panel(sql::Connection* con, sql::ResultSet* user, std::vector<Event*> events, bool &reload);
