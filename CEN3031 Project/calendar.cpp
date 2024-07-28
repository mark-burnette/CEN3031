#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include "mysql_connection.h"
#include "imgui.h"
#include "calendar.h"
#include "search.h"

std::map<std::string, std::vector<Event*>> calendar;

void draw_calendar(std::vector<Event*> totEvents) {
	Event e;
	std::vector<Event*> temp;
	temp.push_back(&e);

	calendar = { { "01Jan", temp }, {"02Feb", temp}
			, {"03Mar", temp}, {"04Apr", temp}, {"05May", temp}
			, {"06Jun", temp}, {"07Jul", temp}, {"08Aug", temp}
			, {"09Sep", temp}, {"10Oct", temp}, {"11Nov", temp}
			, {"12Dec", temp} };

	// Put event in month
	for (auto iter1 = calendar.begin(); iter1 != calendar.end(); iter1++) {
		for (auto iter2 = totEvents.begin(); iter2 != totEvents.end(); iter2++) {
			std::string month = (*iter2)->date.substr(0, 2);
			if (month == (iter1->first.substr(0, 2))) {
				if (iter1->second == temp) {
					std::vector<Event*> vec;
					vec.push_back(*iter2);
					calendar[iter1->first] = vec;
				}
				else {
					iter1->second.push_back(*iter2);
				}
			}
		}
	}

	// Create window
	ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
	ImGui::Begin("Event Calendar");

	if (ImGui::BeginTabBar("Events", tab_bar_flags))
	{
		for (auto iter1 = calendar.begin(); iter1 != calendar.end(); iter1++) {
			std::string month = iter1->first.substr(2, iter1->first.size() - 1);
			if (ImGui::BeginTabItem(month.c_str()))
			{
				if (iter1->second == temp) {
					ImGui::Text("No events this month...");
				}
				else {
					int id = 0;
					for (auto iter2 = iter1->second.begin(); iter2 != iter1->second.end(); iter2++) {
						ImGui::Text((*iter2)->date.c_str());
						ImGui::Text((*iter2)->name.c_str());
						ImGui::Text((*iter2)->desc.c_str());
						// RSVP Actions
						if (ImGui::Button(("RSVP to " + (*iter2)->name).c_str())) {
							ImGui::OpenPopup(std::to_string(id).c_str());
						}
						if (ImGui::BeginPopup(std::to_string(id).c_str())) {
							ImGui::SeparatorText("Your reservation has been made!");
							ImGui::Selectable("Okay");
							ImGui::EndPopup();
						}
						// TODO: Save rsvp info and prevent duplicates
						id++;
					}
				}
				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();
	}
}
std::vector<Event*> getEvents()
{
	std::vector<Event*> events;

	sql::Driver* driver = nullptr;
	sql::Connection* con = nullptr;
	sql::PreparedStatement* pstmt = nullptr;
	sql::ResultSet* event_data = nullptr;

	driver = get_driver_instance();
	con = driver->connect(server, db_username, db_password);

	con->setSchema("test_db");

	pstmt = con->prepareStatement("SELECT * FROM events");
	event_data = pstmt->executeQuery();

	delete pstmt;
	delete con;

	event_data->beforeFirst(); // point to first row

	int _id = 0;
	std::string _name;
	std::string _date;
	std::string _desc;
	while (event_data->next())
	{
		_id =event_data->getInt("id");
		_name = event_data->getString("name");
		_date = event_data->getString("date");
		_desc = event_data->getString("desc");

		Event* e = new Event(_id, _name, _date, _desc);
		events.push_back(e);
	}

	return events;
}