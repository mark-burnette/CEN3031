#pragma once

//#include <unordered_map>
#include <windows.h>
#include <sqlext.h>
#include <cppconn/driver.h>
#include <iostream>
#include <vector>

class Event {
public:
	int id = -1;
	std::string name = "";
	std::string date= "";
	std::string desc = "";
	Event() = default;
	Event(int &_id, std::string &_name, std::string &_date, std::string &_desc) {
		id = _id;
		name = _name;
		date = _date;
		desc = _desc;
	}
};
extern std::map<std::string, std::vector<Event*>> calendar;

void draw_calendar(std::vector<Event*> totEvents);	// Create event calendar
std::vector<Event*> getEvents();	// Get event info from database
