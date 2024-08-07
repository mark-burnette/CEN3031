#include <cppconn/prepared_statement.h>
#include "mysql_connection.h"
#include <cppconn/resultset.h>
#include <tchar.h>
#include "calendar.h"
#include "search.h"



std::map<std::string, std::vector<Event*>> calendar;

std::string sSelectedFile;
std::string sFilePath;
bool openFile()
{
	//  CREATE FILE OBJECT INSTANCE
	HRESULT f_SysHr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(f_SysHr))
		return FALSE;

	// CREATE FileOpenDialog OBJECT
	IFileOpenDialog* f_FileSystem;
	f_SysHr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&f_FileSystem));
	if (FAILED(f_SysHr)) {
		CoUninitialize();
		return FALSE;
	}

	//  SHOW OPEN FILE DIALOG WINDOW
	f_SysHr = f_FileSystem->Show(NULL);
	if (FAILED(f_SysHr)) {
		f_FileSystem->Release();
		CoUninitialize();
		return FALSE;
	}

	//  RETRIEVE FILE NAME FROM THE SELECTED ITEM
	IShellItem* f_Files;
	f_SysHr = f_FileSystem->GetResult(&f_Files);
	if (FAILED(f_SysHr)) {
		f_FileSystem->Release();
		CoUninitialize();
		return FALSE;
	}

	//  STORE AND CONVERT THE FILE NAME
	PWSTR f_Path;
	f_SysHr = f_Files->GetDisplayName(SIGDN_FILESYSPATH, &f_Path);
	if (FAILED(f_SysHr)) {
		f_Files->Release();
		f_FileSystem->Release();
		CoUninitialize();
		return FALSE;
	}

	//  FORMAT AND STORE THE FILE PATH
	std::wstring path(f_Path);
	std::string c(path.begin(), path.end());
	sFilePath = c;

	//  FORMAT STRING FOR EXECUTABLE NAME
	const size_t slash = sFilePath.find_last_of("/\\");
	sSelectedFile = sFilePath.substr(slash + 1);

	//  SUCCESS, CLEAN UP
	CoTaskMemFree(f_Path);
	f_Files->Release();
	f_FileSystem->Release();
	CoUninitialize();
	return TRUE;
}

void create_event(sql::Connection* con, bool &reload)
{
	static char name[45] = {};
	static char date[11] = {};
	static char time[9] = {};
	static char desc[300] = {};
	std::string filename;
	bool is_viewable = 0;

	if (ImGui::Button("Create Event..."))
		ImGui::OpenPopup("Create Event");

	// Always center this window when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Create Event", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::InputText("Event Name: (45 characters)", name, 45);
		ImGui::Separator();
		ImGui::SetNextItemWidth(ImGui::CalcTextSize("(YYYY-MM-DD)").x);
		ImGui::InputText("Event Date: (YYYY-MM-DD)", date, 11);
		ImGui::Separator();
		ImGui::SetNextItemWidth(ImGui::CalcTextSize("(hh:mm:ss)").x);
		ImGui::InputText("Event Time: (hh:mm:ss)", time, 9);
		ImGui::Separator();
		ImGui::Text("Description (300 characters): ");
		ImGui::InputTextMultiline("##source", desc, IM_ARRAYSIZE(desc), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16));
		ImGui::Separator();
		if (ImGui::Button("Choose Image File...")) {
			openFile();
			std::ifstream  src(sFilePath, std::ios::binary);
			std::ofstream  dst("..\\CEN3031 Project\\images\\" + sSelectedFile, std::ios::binary);

			dst << src.rdbuf();
		}
		filename = sSelectedFile;
		ImGui::Text(filename.c_str());
		
		if (ImGui::Button("Submit", ImVec2(120, 0))) { 
			sql::PreparedStatement* pstmt = nullptr;
			pstmt = con->prepareStatement("INSERT INTO events (`name`, `date`, `time`, `desc`, `filename`) VALUES (?, ?, ?, ?, ?)");
			pstmt->setString(1, name);
			pstmt->setString(2, date);
			pstmt->setString(3, time);
			pstmt->setString(4, desc);
			pstmt->setString(5, filename);
			pstmt->execute();
			delete pstmt;
			pstmt = nullptr;
			reload = true;
			ImGui::CloseCurrentPopup(); 
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();
	}
}

void event_request_checker(std::vector<Event*> totEvents, sql::Connection* con, bool &reload)
{
	if (ImGui::Button("Approve/Deny"))
		ImGui::OpenPopup("Approve/Deny");

	// Always center this window when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Approve/Deny", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("List of created events awaiting approval.");
		ImGui::Separator();
		for (auto iter = totEvents.begin(); iter != totEvents.end(); iter++) 
		{
			if((*iter)->is_viewable == false) {
				ImGui::Text((*iter)->name.c_str());
				ImGui::SameLine();
				if (ImGui::Button("Approve"))
				{
					sql::PreparedStatement* pstmt = nullptr;
					pstmt = con->prepareStatement("UPDATE events SET is_viewable = 1 WHERE name = '" + (*iter)->name + "'");
					pstmt->execute();
					delete pstmt;
					pstmt = nullptr;
					reload = true;
				}
				ImGui::SameLine();
				if (ImGui::Button("Deny"))
				{
					sql::PreparedStatement* pstmt = nullptr;
					pstmt = con->prepareStatement("DELETE FROM events WHERE `name` = '" + (*iter)->name + "'");
					pstmt->execute();
					delete pstmt;
					pstmt = nullptr;
					reload = true;
				}

				ImGui::Separator();
			}
		}

		ImGui::PopStyleVar();

		if (ImGui::Button("OK", ImVec2(120, 0))) 
		{ 
			ImGui::CloseCurrentPopup(); 
		}
		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}
}

void draw_calendar(std::vector<Event*> totEvents, sql::ResultSet* user, sql::Connection* con, bool &reload) {
	Event e;
	std::vector<Event*> temp;
	std::string text = "";
	temp.push_back(&e);

	calendar = { { "01Jan", temp }, {"02Feb", temp}
			, {"03Mar", temp}, {"04Apr", temp}, {"05May", temp}
			, {"06Jun", temp}, {"07Jul", temp}, {"08Aug", temp}
			, {"09Sep", temp}, {"10Oct", temp}, {"11Nov", temp}
			, {"12Dec", temp} };

	// Put event in month
	for (auto iter1 = calendar.begin(); iter1 != calendar.end(); iter1++) {
		for (auto iter2 = totEvents.begin(); iter2 != totEvents.end(); iter2++) {
			if ((*iter2)->is_viewable != false) 
			{
				std::string month = (*iter2)->date.substr(5, 2);
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
	}

	ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;

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
						text = (*iter2)->name + " " + (*iter2)->date + " " + (*iter2)->time;
						if (ImGui::Button(text.c_str()))
							ImGui::OpenPopup((*iter2)->name.c_str());
							// Always center this window when appearing
							ImVec2 center = ImGui::GetMainViewport()->GetCenter();
						ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

						if (ImGui::BeginPopupModal((*iter2)->name.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
						{
							ImGui::Image((void*)(*iter2)->my_texture, ImVec2((*iter2)->my_image_width, (*iter2)->my_image_height));
							ImGui::Text(("Event Name: " + (*iter2)->name).c_str());
							ImGui::Text(("Date: " + (*iter2)->date).c_str());
							ImGui::Text(("Time: " + (*iter2)->time).c_str());
							ImGui::Text(("Description: " + (*iter2)->desc).c_str());
							ImGui::Separator();

							//static int unused_i = 0;
							//ImGui::Combo("Combo", &unused_i, "Delete\0Delete harder\0");

							static bool dont_ask_me_next_time = false;
							ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
							ImGui::PopStyleVar();

							text = "RSVP to " + (*iter2)->name;

							// RSVP Button should only work for Users+
							if (ImGui::Button(text.c_str(), ImVec2(120, 0))) {
								ImGui::OpenPopup(std::to_string(id).c_str());
							}
							if (ImGui::BeginPopup(std::to_string(id).c_str())) {
								if(user == nullptr)
									ImGui::SeparatorText("You need to be logged in!");
								else { 
									static char username[45] = {};
									ImGui::InputText("Type in your name: ", username, 45);
									ImGui::Separator();
									if (ImGui::Selectable("Enter"))
									{
										sql::PreparedStatement* pstmt = nullptr;
										pstmt = con->prepareStatement("INSERT INTO rsvp (`event_name`, `username`) VALUES (?, ?)");
										pstmt->setString(1, (*iter2)->name);
										pstmt->setString(2, username);
										pstmt->execute();
										delete pstmt;
										pstmt = nullptr;
										ImGui::SeparatorText("Your reservation has been made!");
									}
								}
								ImGui::EndPopup();
							}
							ImGui::SetItemDefaultFocus();
							ImGui::SameLine();
							// Done button should always appear
							if (ImGui::Button("Done", ImVec2(120, 0))) { 
								ImGui::CloseCurrentPopup(); 
							}
							ImGui::EndPopup();
						}
						id++;
					}
				}
				if (user != nullptr)
				{
					user->first();
					if (user->getString("role") == "user")
						goto jmp;
					if (user->getString("role") == "employee" || "admin")
						create_event(con, reload);
					if (user->getString("role") == "admin")
						event_request_checker(totEvents, con, reload);
				}
				jmp:
				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();
	}
}

std::vector<Event*> getEvents(sql::Connection* con)
{
	std::vector<Event*> events;

	sql::PreparedStatement* pstmt = nullptr;
	sql::ResultSet* event_data = nullptr;

	pstmt = con->prepareStatement("SELECT * FROM events ORDER BY date, time");
	event_data = pstmt->executeQuery();

	delete pstmt;

	event_data->beforeFirst(); // point to first row

	int _id = 0;
	std::string _name;
	std::string _date;
	std::string _time;
	std::string _desc;
	std::string _filename;
	bool _is_viewable;
	int _my_image_width = 0;
	int _my_image_height = 0;
	ID3D11ShaderResourceView* _my_texture = NULL;
	while (event_data->next())
	{
		_id =event_data->getInt("id");
		_name = event_data->getString("name");
		_date = event_data->getString("date");
		_time = event_data->getString("time");
		_desc = event_data->getString("desc");
		_filename = event_data->getString("filename");
		_is_viewable = event_data->getBoolean("is_viewable");
		bool ret = LoadTextureFromFile(("..\\CEN3031 Project\\images\\" + _filename).c_str(), &_my_texture, &_my_image_width, &_my_image_height);
		IM_ASSERT(ret);

		Event* e = new Event(_id, _name, _date, _time, _desc, _filename, _is_viewable, _my_image_height, _my_image_width, _my_texture);
		events.push_back(e);
	}

	delete event_data;

	return events;
}