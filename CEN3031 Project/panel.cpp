#include "panel.h"
#include "imgui.h"
#include "list.h"
#include "search.h"

#include <cppconn/prepared_statement.h>

/* panel for users, employees, and admins
* non-logged in:
*	tab for catalogue
*	tab for events
* users:
*	tab for catalogue (including search)
*	tab for events
*	tab for checked-out books
* employees:
*	tab for creating user accounts
*	tab for creating events pending admin approval 
*	tab for approving checkouts
* admins:
*	all employee tabs + added functionality
*		create employee accounts
*	tab to approve events
*/

void panel(sql::Connection* con, sql::ResultSet* user)
{
	ImGui::Begin("User Panel", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

	// TODO: place in constructor/destructor for automatic deletion
	static sql::ResultSet* res = nullptr;
	static sql::PreparedStatement* pstmt = nullptr;
	try
	{
		if (ImGui::BeginTabBar("TabBar"))
		{
			if (!user)
			{
				// unregistered user

				if (ImGui::BeginTabItem("Catalogue"))
				{
					sql::ResultSet* search_results = search_form(con);
					listings(con, search_results, user);

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Events"))
				{
					ImGui::EndTabItem();
				}
			}
			else
			{
				user->beforeFirst();
				user->next();

				static bool tab_clicked = false; // TODO: update only on tab click
				if (user->getString("role") == "user")
				{
					if (ImGui::BeginTabItem("Catalogue"))
					{
						sql::ResultSet* search_results = search_form(con);
						listings(con, search_results, user);

						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem("My Items"))
					{
						/*
						pstmt = con->prepareStatement("SELECT * FROM books WHERE `checked-out-by` = ?");
						pstmt->setInt(1, user->getInt("id"));
						pstmt->execute();
						*/

						pstmt = con->prepareStatement("SELECT * FROM requested_checkouts WHERE user = ?");
						pstmt->setInt(1, user->getInt("id"));
						pstmt->execute();

						res = pstmt->getResultSet();

						res->beforeFirst();
						while (res->next())
						{
							ImGui::Text(res->getString("title").c_str());
						}

						delete pstmt;
						pstmt = nullptr;
						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem("Events"))
					{
						ImGui::EndTabItem();
					}
				}
				else
				{
					// if employee or admin
					if (ImGui::BeginTabItem("Accounts"))
					{
						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem("Checkouts"))
					{
						/* TODO: search for user, list all requested checkouts from only that user
						*/

						pstmt = con->prepareStatement("SELECT * FROM requested_checkouts");
						pstmt->execute();

						res = pstmt->getResultSet();

						res->beforeFirst();
						while (res->next())
						{
							ImGui::Text(res->getString("user").c_str()); // TODO: display name instead of user id
							ImGui::Text(res->getString("title").c_str());
							ImGui::SameLine(); ImGui::Text((std::string("(ISBN: ") + res->getString("ISBN").c_str() + std::string(")")).c_str());

							bool approve = ImGui::Button("Approve");
							ImGui::SameLine(); bool reject = ImGui::Button("Reject");

							if (approve)
							{
								static sql::ResultSet* checkout_res = nullptr;

								// remove from requested_checkouts (TODO: notify other users that checkout has been rejected)
								pstmt = con->prepareStatement("DELETE FROM requested_checkouts WHERE ISBN = ?");
								pstmt->setString(1, res->getString("ISBN"));
								pstmt->execute();

								delete pstmt;

								// mark book as checked out and set return date for 1 month after checkout 
								pstmt = con->prepareStatement("UPDATE books SET `checked-out-by` = ?, `date-checked-out` = CURDATE(), `expiration-date` = DATE_ADD(CURDATE(), INTERVAL 1 MONTH) WHERE ISBN = ?");
								pstmt->setInt(1, user->getInt("id"));
								pstmt->setString(2, res->getString("ISBN"));
								pstmt->execute();

								delete pstmt;
								pstmt = nullptr;
							}

							if (reject)
							{
								// remove from requested_checkouts
								pstmt = con->prepareStatement("DELETE FROM requested_checkouts WHERE ISBN = ? AND user = ?");
								pstmt->setString(1, res->getString("ISBN"));
								pstmt->setInt(2, res->getInt("user"));
								pstmt->execute();

								delete pstmt;
								pstmt = nullptr;

								// TODO: notify user of rejection
							}
						}

						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem("Events"))
					{
						ImGui::EndTabItem();
					}

					if (user->getString("role") == "admin")
					{
					}
				}
			}

			ImGui::EndTabBar();
		}
		ImGui::End();
	}
	catch (const sql::SQLException& e)
	{
		std::cout << "Error: " << e.what() << std::endl;

		delete res;
		res = nullptr;

		delete pstmt;
		pstmt = nullptr;
	}
}