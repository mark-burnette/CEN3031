#include "panel.h"
#include "imgui.h"
#include "list.h"
#include "search.h"
#include "calendar.h"
#include "register.h"
#include <vector>

#include <cppconn/prepared_statement.h>

/* panel for users, employees, and admins
* everybody:
*	tab for catalogue
*	tab for events
* users:
*	tab for viewing checked-out & pending resources
* employees:
*	added functionality to event tab (create events pending admin approval)
*	tab for creating user accounts
*	tab for processing checkouts/returns
* admins:
*	all employee tabs + added functionality
*		create employee accounts
*	tab to approve events
*/

void panel(sql::Connection* con, sql::ResultSet* user, std::vector<Event*> events, bool& reload)
{
	ImGui::Begin("User Panel", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

	static std::string previous_tab{}; // optimization

	static sql::ResultSet* res = nullptr;
	static sql::PreparedStatement* pstmt = nullptr;
	sql::ResultSet* user_notifs = nullptr;
	try
	{
		if (ImGui::BeginTabBar("TabBar"))
		{
			// catalogue and events can be viewed by everyone
			// TODO: change so that admins/employees cannot check out books
			if (ImGui::BeginTabItem("Books"))
			{
				sql::ResultSet* search_results = search_form(con);
				listings(con, search_results, user);

				ImGui::EndTabItem();
				previous_tab = "Books";
				search_results = nullptr;
			}

			if (ImGui::BeginTabItem("Movies"))
			{
				sql::ResultSet* search_results = search_movies(con);
				movies_listings(con, search_results, user);

				ImGui::EndTabItem();
				previous_tab = "Movies";
				search_results = nullptr;
			}

			if (ImGui::BeginTabItem("Events"))
			{
				draw_calendar(events, user, con, reload);
				ImGui::EndTabItem();

				previous_tab = "Events";
			}

			if (user)
			{
				user->first();
				ImGui::SetNextWindowSize(ImVec2(400, 200));
				ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.83f, ImGui::GetIO().DisplaySize.y * 0.2f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
				if (ImGui::Begin("Notifications", nullptr, ImGuiWindowFlags_NoCollapse)) {
					pstmt = con->prepareStatement("SELECT * FROM notifs WHERE `user_id` = ?");
					pstmt->setInt(1, user->getInt("id"));
					pstmt->execute();

					user_notifs = pstmt->getResultSet();
					delete pstmt;
					pstmt = nullptr;

					if (!user_notifs->first()) {
						ImGui::Text("No notifications.");
					}
					else {
						user_notifs->afterLast();
						while (user_notifs->previous()) {
							ImGui::TextWrapped(user_notifs->getString("text").c_str());
							if (ImGui::Button("Delete")) {
								pstmt = con->prepareStatement("DELETE FROM notifs WHERE notif_id = ? and user_id = ?");
								pstmt->setInt(1, user_notifs->getInt("notif_id"));
								pstmt->setInt(2, user_notifs->getInt("user_id"));
								pstmt->execute();

								delete pstmt;
								pstmt = nullptr;
							}
						}
					}
					ImGui::End();
				}

				if (user->getString("role") == "user")
				{
					if (ImGui::BeginTabItem("My Items"))
					{
						static sql::ResultSet* checked_out_books = nullptr;
						static sql::ResultSet* pending_books = nullptr;

						static sql::ResultSet* checked_out_movies = nullptr;
						static sql::ResultSet* pending_movies = nullptr;

						// update only when tab clicked
						if (previous_tab != "My Items")
						{
							pstmt = con->prepareStatement("SELECT * FROM books WHERE `checked-out-by` = ?");

							pstmt->setInt(1, user->getInt("id"));
							pstmt->execute();

							checked_out_books = pstmt->getResultSet();

							delete pstmt;

							pstmt = con->prepareStatement("SELECT * FROM requested_checkouts WHERE user = ?");
							pstmt->setInt(1, user->getInt("id"));
							pstmt->execute();

							pending_books = pstmt->getResultSet();

							delete pstmt;
							//pstmt = nullptr;


							pstmt = con->prepareStatement("SELECT * FROM movies WHERE `checked-out-by` = ?");

							pstmt->setInt(1, user->getInt("id"));
							pstmt->execute();

							checked_out_movies = pstmt->getResultSet();

							delete pstmt;

							pstmt = con->prepareStatement("SELECT * FROM requested_checkouts_movies WHERE user = ?");
							pstmt->setInt(1, user->getInt("id"));
							pstmt->execute();

							pending_movies = pstmt->getResultSet();

							delete pstmt;
							pstmt = nullptr;
						}

						if (ImGui::CollapsingHeader("Checked-out items##My Items/Checked-out items"))
						{
							// list checkouts
							ImGui::Text("Books:");
							checked_out_books->beforeFirst();
							while (checked_out_books->next())
							{
								ImGui::Text(checked_out_books->getString("Book-Title").c_str());
								ImGui::SameLine(); ImGui::Text(std::string(("(ISBN: ") + checked_out_books->getString("ISBN") + ")").c_str());
								ImGui::Text((std::string("Due date: ") + checked_out_books->getString("expiration-date").c_str()).c_str());
								if (!checked_out_books->isLast())
									ImGui::Separator();
							}

							ImGui::Text("\nMovies:");
							checked_out_movies->beforeFirst();
							while (checked_out_movies->next())
							{
								ImGui::Text(checked_out_movies->getString("Title").c_str());
								//ImGui::SameLine(); ImGui::Text(std::string(("(") + checked_out_movies->getString("Year") + ")").c_str());
								ImGui::Text((std::string("Due date: ") + checked_out_movies->getString("expiration-date").c_str()).c_str());
								if (!checked_out_movies->isLast())
									ImGui::Separator();
							}
						}

						if (ImGui::CollapsingHeader("Pending checkouts##My Items/Pending checkouts"))
						{
							// list pending checkouts
							ImGui::Text("Books:");
							pending_books->beforeFirst();
							while (pending_books->next())
							{
								ImGui::Text(pending_books->getString("title").c_str());
								ImGui::SameLine(); ImGui::Text(std::string(("(ISBN: ") + pending_books->getString("ISBN") + ")").c_str());
							}

							ImGui::Text("\nMovies:");
							pending_movies->beforeFirst();
							while (pending_movies->next())
							{
								ImGui::Text(pending_movies->getString("Title").c_str());
								//ImGui::SameLine(); ImGui::Text(std::string(("(") + pending_movies->getString("Year") + ")").c_str());
							}
						}

						ImGui::EndTabItem();
						previous_tab = "My Items";
					}
				}
				else
				{
					// employees can add new users but not delete them
					if (user->getString("role") == "employee")
					{
						if (ImGui::BeginTabItem("Accounts"))
						{
							ImGui::SeparatorText("Register user");

							static char username[32] = {};
							static char password[32] = {};

							register_role(con, "user", username, 32, password, 32);
							ImGui::EndTabItem();
						}
					}

					// both employees and admins can see & change status of checkouts
					if (ImGui::BeginTabItem("Checkouts"))
					{
						ImGui::SeparatorText("Search for a user");

						static char username[32] = {};
						ImGui::InputText("User", username, 32);

						static int user_id = -1;
						static int do_search = false;
						static sql::ResultSet* checked_out_books = nullptr;
						static sql::ResultSet* pending_books = nullptr;
						static sql::ResultSet* checked_out_movies = nullptr;
						static sql::ResultSet* pending_movies = nullptr;

						if (ImGui::Button("Search"))
							do_search = true;

						if (do_search)
						{
							// get user id
							pstmt = con->prepareStatement("SELECT id FROM users WHERE username = ?");

							pstmt->setString(1, username);
							pstmt->execute();

							res = pstmt->getResultSet();
							if (res && res->first())
								user_id = res->getInt("id");

							delete pstmt;
							delete res;
							res = nullptr;

							pstmt = con->prepareStatement("SELECT * FROM books WHERE `checked-out-by` = ?");

							pstmt->setInt(1, user_id);
							pstmt->execute();

							checked_out_books = pstmt->getResultSet();

							delete pstmt;

							pstmt = con->prepareStatement("SELECT * FROM requested_checkouts WHERE user = ?");
							pstmt->setInt(1, user_id);
							pstmt->execute();

							pending_books = pstmt->getResultSet();

							delete pstmt;



							pstmt = con->prepareStatement("SELECT * FROM movies WHERE `checked-out-by` = ?");

							pstmt->setInt(1, user_id);
							pstmt->execute();

							checked_out_movies = pstmt->getResultSet();

							delete pstmt;

							pstmt = con->prepareStatement("SELECT * FROM requested_checkouts_movies WHERE user = ?");
							pstmt->setInt(1, user_id);
							pstmt->execute();

							pending_movies = pstmt->getResultSet();

							delete pstmt;
							pstmt = nullptr;

							memset(&username, 0, 32);
							do_search = false;
						}

						if (ImGui::CollapsingHeader("Checked-out items##Checkouts/Checked-out items"))
						{
							if ((!checked_out_books || !checked_out_books->first()) && (!checked_out_movies || !checked_out_movies->first()))
								ImGui::Text("No results.");
							else
							{
								ImGui::Text("Books:");
								int id = 0;
								checked_out_books->beforeFirst();
								while (checked_out_books->next())
								{
									id++;

									//ImGui::Text(checked_out_books->getString("checked-out-by").c_str()); // TODO: display name instead of user id
									ImGui::Text(checked_out_books->getString("Book-Title").c_str());
									ImGui::SameLine(); ImGui::Text((std::string("(ISBN: ") + checked_out_books->getString("ISBN").c_str() + std::string(")")).c_str());

									if (ImGui::Button((std::string("Return##") + std::to_string(id)).c_str()))
									{
										// remove from checkouts
										pstmt = con->prepareStatement("UPDATE books SET `checked-out-by` = NULL, `date-checked-out` = NULL, `expiration-date` = NULL WHERE ISBN = ? AND `Book-Title` = ?");
										pstmt->setString(1, checked_out_books->getString("ISBN"));
										pstmt->setString(2, checked_out_books->getString("Book-Title"));
										pstmt->execute();

										delete pstmt;
										pstmt = nullptr;

										do_search = true;
									}
								}

								ImGui::Text("\nMovies:");
								checked_out_movies->beforeFirst();
								while (checked_out_movies->next())
								{
									id++;

									//ImGui::Text(checked_out_movies->getString("checked-out-by").c_str()); // TODO: display name instead of user id
									ImGui::Text(checked_out_movies->getString("Title").c_str());
									//ImGui::SameLine(); ImGui::Text((std::string("(ISBN: ") + checked_out_books->getString("ISBN").c_str() + std::string(")")).c_str());

									if (ImGui::Button((std::string("Return##") + std::to_string(id)).c_str()))
									{
										// remove from checkouts
										pstmt = con->prepareStatement("UPDATE movies SET `checked-out-by` = NULL, `date-checked-out` = NULL, `expiration-date` = NULL WHERE imdb_id = ? AND `Title` = ?");
										pstmt->setString(1, checked_out_movies->getString("imdb_id"));
										pstmt->setString(2, checked_out_movies->getString("Title"));
										pstmt->execute();

										delete pstmt;
										pstmt = nullptr;

										do_search = true;
									}
								}
							}
						}

						if (ImGui::CollapsingHeader("Pending checkouts##Checkouts/Pending checkouts"))
						{
							if ((!pending_books || !pending_books->first()) && (!pending_movies || !pending_movies->first()))
								ImGui::Text("No results.");
							else
							{
								int id = 0;
								ImGui::Text("Books:");
								pending_books->beforeFirst();
								while (pending_books->next())
								{
									id++;

									//ImGui::Text(pending_books->getString("user").c_str()); // TODO: display name instead of user id
									ImGui::Text(pending_books->getString("title").c_str());
									ImGui::SameLine(); ImGui::Text((std::string("(ISBN: ") + pending_books->getString("ISBN").c_str() + std::string(")")).c_str());

									bool approve = ImGui::Button((std::string("Approve##") + std::to_string(id)).c_str());
									ImGui::SameLine(); bool reject = ImGui::Button("Reject");

									if (approve)
									{
										static sql::ResultSet* checkout_res = nullptr;

										// remove from requested_checkouts (TODO: notify other users that checkout has been rejected)
										pstmt = con->prepareStatement("DELETE FROM requested_checkouts WHERE ISBN = ?");
										pstmt->setString(1, pending_books->getString("ISBN"));
										pstmt->execute();

										delete pstmt;

										// mark book as checked out and set return date for 1 month after checkout 
										pstmt = con->prepareStatement("UPDATE books SET `checked-out-by` = ?, `date-checked-out` = CURDATE(), `expiration-date` = DATE_ADD(CURDATE(), INTERVAL 1 MONTH), `last-checked-out` = ? WHERE ISBN = ?");
										pstmt->setInt(1, pending_books->getInt("user"));
										pstmt->setInt(2, pending_books->getInt("user"));
										pstmt->setString(3, pending_books->getString("ISBN"));
										pstmt->execute();

										delete pstmt;
										pstmt = nullptr;

										pstmt = con->prepareStatement("INSERT INTO notifs (`user_id`, `text`) VALUES (?, ?)");
										pstmt->setInt(1, pending_books->getInt("user"));
										std::string message = (pending_books->getString("title") + " has been checked out!");
										pstmt->setString(2, message);
										pstmt->execute();

										delete pstmt;
										pstmt = nullptr;

										do_search = true;
									}

									if (reject)
									{
										pstmt = con->prepareStatement("INSERT INTO notifs (`user_id`, `text`) VALUES (?, ?)");
										pstmt->setInt(1, pending_books->getInt("user"));
										std::string message = ("Checkout for " + pending_books->getString("title") + " was rejected.");
										pstmt->setString(2, message);
										pstmt->execute();

										delete pstmt;
										pstmt = nullptr;

										// remove from requested_checkouts
										pstmt = con->prepareStatement("DELETE FROM requested_checkouts WHERE ISBN = ? AND user = ?");
										pstmt->setString(1, pending_books->getString("ISBN"));
										pstmt->setInt(2, pending_books->getInt("user"));
										pstmt->execute();

										delete pstmt;
										pstmt = nullptr;

										do_search = true;
									}
								}


								ImGui::Text("\nMovies:");
								pending_movies->beforeFirst();
								while (pending_movies->next())
								{
									id++;

									//ImGui::Text(pending_movies->getString("user").c_str()); // TODO: display name instead of user id
									ImGui::Text(pending_movies->getString("Title").c_str());
									//ImGui::SameLine(); ImGui::Text((std::string("(ISBN: ") + pending_books->getString("ISBN").c_str() + std::string(")")).c_str());
									bool approve = ImGui::Button((std::string("Approve##") + std::to_string(id)).c_str());
									ImGui::SameLine(); bool reject = ImGui::Button("Reject");

									if (approve)
									{
										static sql::ResultSet* checkout_res = nullptr;

										// remove from requested_checkouts (TODO: notify other users that checkout has been rejected)
										pstmt = con->prepareStatement("DELETE FROM requested_checkouts_movies WHERE imdb_id = ?");
										pstmt->setString(1, pending_movies->getString("imdb_id"));
										pstmt->execute();

										delete pstmt;

										// mark book as checked out and set return date for 1 month after checkout 
										pstmt = con->prepareStatement("UPDATE movies SET `checked-out-by` = ?, `date-checked-out` = CURDATE(), `expiration-date` = DATE_ADD(CURDATE(), INTERVAL 1 MONTH), `last-checked-out` = ? WHERE imdb_id = ?");
										pstmt->setInt(1, pending_movies->getInt("user"));
										pstmt->setInt(2, pending_movies->getInt("user"));
										pstmt->setString(3, pending_movies->getString("imdb_id"));
										pstmt->execute();

										delete pstmt;
										pstmt = nullptr;

										pstmt = con->prepareStatement("INSERT INTO notifs (`user_id`, `text`) VALUES (?, ?)");
										pstmt->setInt(1, pending_movies->getInt("user"));
										std::string message = (pending_movies->getString("Title") + " has been checked out!");
										pstmt->setString(2, message);
										pstmt->execute();

										delete pstmt;
										pstmt = nullptr;

										do_search = true;
									}

									if (reject)
									{
										pstmt = con->prepareStatement("INSERT INTO notifs (`user_id`, `text`) VALUES (?, ?)");
										pstmt->setInt(1, pending_movies->getInt("user"));
										std::string message = ("Checkout for " + pending_movies->getString("Title") + " was rejected.");
										pstmt->setString(2, message);
										pstmt->execute();

										delete pstmt;
										pstmt = nullptr;

										// remove from requested_checkouts
										pstmt = con->prepareStatement("DELETE FROM requested_checkouts_movies WHERE imdb_id = ? AND user = ?");
										pstmt->setString(1, pending_movies->getString("imdb_id"));
										pstmt->setInt(2, pending_movies->getInt("user"));
										pstmt->execute();

										delete pstmt;
										pstmt = nullptr;

										do_search = true;
									}
								}
							}
						}

						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem("Inventory"))
					{
						if (ImGui::CollapsingHeader("Add new resources"))
						{
							static char _isbn[14] = {};
							static char _title[257] = {};
							static char _author[65] = {};
							static char _publisher[257] = {};
							static char _year_published[5] = {};

							ImGui::InputText("ISBN", _isbn, sizeof(_isbn));
							ImGui::InputText("Title", _title, sizeof(_title));
							ImGui::InputText("Author", _author, sizeof(_author));
							ImGui::InputText("Publisher", _publisher, sizeof(_publisher));
							ImGui::InputText("Year of Publication", _year_published, sizeof(_year_published));

							if (ImGui::Button("Add book"))
							{
								// add new entry
								pstmt = con->prepareStatement("INSERT INTO books (ISBN, `Book-Title`, `Book-Author`, Publisher, `Year-Of-Publication`) VALUES (?, ?, ?, ?, ?)");
								pstmt->setString(1, _isbn);
								pstmt->setString(2, _title);
								pstmt->setString(3, _author);
								pstmt->setString(4, _publisher);
								pstmt->setString(5, _year_published);
								pstmt->execute();

								delete pstmt;
								pstmt = nullptr;

								memset(&_isbn, 0, sizeof(_isbn));
								memset(&_title, 0, sizeof(_title));
								memset(&_author, 0, sizeof(_author));
								memset(&_publisher, 0, sizeof(_publisher));
								memset(&_year_published, 0, sizeof(_year_published));
							}

							static char _imdb_id[14] = {};
							static char _movie_title[257] = {};
							static char _year[5] = {};

							ImGui::InputText("imdb_id", _imdb_id, sizeof(_imdb_id));
							ImGui::InputText("Title##idk", _movie_title, sizeof(_movie_title));
							ImGui::InputText("Year", _year, sizeof(_year));

							if (ImGui::Button("Add movie"))
							{
								// add new entry
								pstmt = con->prepareStatement("INSERT INTO movies (imdb_id, `Title`, `Year`) VALUES (?, ?, ?)");
								pstmt->setString(1, _imdb_id);
								pstmt->setString(2, _movie_title);
								pstmt->setString(3, _year);

								pstmt->execute();

								delete pstmt;
								pstmt = nullptr;

								memset(&_imdb_id, 0, sizeof(_imdb_id));
								memset(&_movie_title, 0, sizeof(_movie_title));
								memset(&_year, 0, sizeof(_year));
							}
						}
						if (ImGui::CollapsingHeader("Modify existing resources"))
						{
							static char title[64] = {};

							ImGui::SeparatorText("Search for a book:");
							ImGui::InputText("Title##Modify existing resources/Title", title, sizeof(title));

							static sql::ResultSet* search_results = nullptr;
							if (ImGui::Button("Search"))
							{
								pstmt = con->prepareStatement("SELECT * FROM books WHERE `Book-Title` LIKE ?");
								std::string _title{ "%" };
								_title.append(title);
								_title.append("%");
								pstmt->setString(1, _title.c_str());
								pstmt->execute();

								search_results = pstmt->getResultSet();

								delete pstmt;
								pstmt = nullptr;

								memset(&title, 0, sizeof(title));
							}

							ImGui::BeginChild("Inventory/Listings", ImVec2(800, 500), 0, ImGuiWindowFlags_HorizontalScrollbar);
							ImGui::SeparatorText("Results:");

							if (search_results)
							{
								int id = 0;
								search_results->beforeFirst();
								while (search_results->next())
								{
									id++;

									std::string header{ search_results->getString("Book-Title").c_str() };
									header.append("##");
									header.append("Inventory");
									header.append(std::to_string(id));

									if (ImGui::CollapsingHeader(header.c_str()))
									{
										// initialize fields
										static char _isbn[14] = {};
										static char _title[257] = {};
										static char _author[65] = {};
										static char _publisher[257] = {};
										static char _year_published[5] = {};

										if (_isbn[0] == '\0')
										{
											std::string isbn{ search_results->getString("ISBN").c_str() };
											std::string title{ search_results->getString("Book-Title").c_str() };
											std::string author{ search_results->getString("Book-Author").c_str() };
											std::string publisher{ search_results->getString("Publisher").c_str() };
											std::string year_published{ search_results->getString("Year-Of-Publication").c_str() };

											memcpy(_isbn, isbn.c_str(), isbn.length());
											memcpy(_title, title.c_str(), title.length());
											memcpy(_author, author.c_str(), author.length());
											memcpy(_publisher, publisher.c_str(), publisher.length());
											memcpy(_year_published, year_published.c_str(), year_published.length());
										}

										ImGui::InputText("ISBN", _isbn, sizeof(_isbn));
										ImGui::InputText("Title", _title, sizeof(_title));
										ImGui::InputText("Author", _author, sizeof(_author));
										ImGui::InputText("Publisher", _publisher, sizeof(_publisher));
										ImGui::InputText("Year of Publication", _year_published, sizeof(_year_published));

										if (ImGui::Button("Modify"))
										{
											// update entry
											pstmt = con->prepareStatement("UPDATE books SET ISBN = ?, `Book-Title` = ?, `Book-Author` = ?, Publisher = ?, `Year-Of-Publication` = ? WHERE id = ?");
											pstmt->setString(1, _isbn);
											pstmt->setString(2, _title);
											pstmt->setString(3, _author);
											pstmt->setString(4, _publisher);
											pstmt->setString(5, _year_published);
											pstmt->setInt(6, search_results->getInt("id"));
											pstmt->execute();

											delete pstmt;
											pstmt = nullptr;
										}
									}
								}
							}

							ImGui::EndChild();
						}
						ImGui::EndTabItem();
					}

					ImGui::Spacing();

					if (user->getString("role") == "admin")
					{
						// allow can choose role of new user
						if (ImGui::BeginTabItem("Accounts"))
						{
							static char username[32] = {};
							static char password[32] = {};

							static std::string role{ "user" };
							ImGui::SeparatorText((std::string("Register new ") + role).c_str());

							static int role_idx = 0;
							if (ImGui::Combo("Role", &role_idx, "User\0Employee\0Administrator\0"))
							{
								switch (role_idx)
								{
								case 0: role = "user"; break;
								case 1: role = "employee"; break;
								case 2: role = "admin"; break;
								}
							}

							register_role(con, role, username, 32, password, 32);

							ImGui::Spacing();

							// user search function
							ImGui::SeparatorText("Search for a user:");

							static char search_username[32] = {};
							ImGui::InputText("Username##SearchUser", search_username, 32);

							static bool search = false;
							static sql::ResultSet* search_results = nullptr;
							if (ImGui::Button("Search"))
								search = true;

							if (search)
							{
								pstmt = con->prepareStatement("SELECT id, username, role FROM users WHERE username=?");
								pstmt->setString(1, search_username);
								pstmt->execute();

								search_results = pstmt->getResultSet();
								search = false;
							}

							ImGui::Spacing();

							if (search_results)
							{
								if (search_results->first())
									ImGui::Text("Results:");
								else
									ImGui::Text("No results.");

								if (ImGui::BeginTable("Table of Accounts", 3))
								{
									ImGui::TableSetupColumn("id");
									ImGui::TableSetupColumn("username");
									ImGui::TableSetupColumn("role");
									ImGui::TableHeadersRow();

									// fill rows
									int row = 0;
									static bool selected = false;

									static std::vector<int> selected_users{};

									search_results->beforeFirst();
									while (search_results && search_results->next())
									{
										ImGui::TableNextRow();

										ImGui::TableSetColumnIndex(0);
										if (ImGui::Selectable(std::to_string(search_results->getInt("id")).c_str(), selected, ImGuiSelectableFlags_SpanAllColumns))
										{
											selected = !selected;
											selected_users.push_back(search_results->getInt("id"));
										}

										ImGui::TableSetColumnIndex(1);
										ImGui::Selectable(search_results->getString("username").c_str(), selected, ImGuiSelectableFlags_SpanAllColumns);
										ImGui::TableSetColumnIndex(2);
										ImGui::Selectable(search_results->getString("role").c_str(), selected, ImGuiSelectableFlags_SpanAllColumns);
									}
									ImGui::EndTable();

									if (ImGui::Button("Delete selected"))
									{
										for (const auto& user : selected_users)
										{
											pstmt = con->prepareStatement("DELETE FROM users WHERE id = ?");
											pstmt->setInt(1, user);
											pstmt->execute();
										}

										selected_users.clear();
										search = true;
									}
								}
							}
							ImGui::EndTabItem();
						}
					}
				}
			}
			ImGui::EndTabBar();
		}
		ImGui::End();
	}
	catch (const sql::SQLException& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;

		delete res;
		res = nullptr;

		delete pstmt;
		pstmt = nullptr;
	}
}