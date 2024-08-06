#pragma once

//#include <unordered_map>
#include <windows.h>
#include <sqlext.h>
#include <cppconn/driver.h>
#include <cppconn/resultset.h>
#include <iostream>
#include <fstream>
#include <string>
#include <shobjidl.h> 
#include <vector>
#include <d3d11.h>
#include "imgui.h"
#include "stb_image.h"
#include "list.h"

class Event {
public:
	int id = -1;
	std::string name = "";
	std::string date = "";
	std::string time = "";
	std::string desc = "";
	std::string filename = "";
	bool is_viewable = 0;
	int my_image_width = 0;
	int my_image_height = 0;
	ID3D11ShaderResourceView* my_texture = NULL;
	Event() = default;
	Event(int &_id, std::string &_name, std::string &_date, std::string& _time, std::string &_desc, std::string& _filename, bool &_is_viewable,int &_my_image_height, int& _my_image_width, ID3D11ShaderResourceView* _my_texture) {
		id = _id;
		name = _name;
		date = _date;
		time = _time;
		desc = _desc;
		filename = _filename;
		is_viewable = _is_viewable;
		my_image_height = _my_image_height;
		my_image_width = _my_image_width;
		my_texture = _my_texture;
	}
};
extern std::map<std::string, std::vector<Event*>> calendar;

void draw_eventCreator(bool& trigger_eventGet); // Create Event Creator
void draw_calendar(std::vector<Event*> totEvents, sql::ResultSet* user, sql::Connection* con, bool &reload);	// Create event calendar
std::vector<Event*> getEvents(sql::Connection* con);	// Get event info from database
