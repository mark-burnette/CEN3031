bool list = false;

#include <d3d11.h>

#include <unordered_map>
#include <time.h>

#include "imgui.h"
#include "list.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// cache of book covers (id, img filename)
std::unordered_map<std::string, std::string> cache;

// credits: https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
// Simple helper function to load an image into a DX11 texture with common settings
bool LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
    // Load from disk into a raw RGBA buffer
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create texture
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = image_width;
    desc.Height = image_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ID3D11Texture2D *pTexture = NULL;
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = image_data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;
    g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

    // Create texture view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
    pTexture->Release();

    *out_width = image_width;
    *out_height = image_height;
    stbi_image_free(image_data);

    return true;
}

bool has_overdue_items(sql::Connection* con, sql::ResultSet* user)
{
	sql::PreparedStatement* pstmt = con->prepareStatement("SELECT * FROM books WHERE `checked-out-by` = ? AND CURDATE() > `expiration-date`");

	user->first();
	pstmt->setInt(1, user->getInt("id"));
	pstmt->execute();

	sql::ResultSet* res = pstmt->getResultSet();
	if (res && res->first())
		return true;

	return false;
}

int listings(sql::Connection* con, sql::ResultSet* search_results, sql::ResultSet* user)
{
	if (!list)
		return 1;

	ImGui::BeginChild("Listings", ImVec2(800, 500), 0, ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::Spacing();
	ImGui::SeparatorText("Results");

	if (search_results)
		search_results->beforeFirst();

	if (!search_results || !search_results->next())
	{
		ImGui::Text("No results.");
		ImGui::EndChild();
		return 0;
	}

	sql::PreparedStatement* pstmt = nullptr;

	try
	{
		// if not rendering window, return
		// unique id for ImGui
		int id = 0;
		search_results->beforeFirst();
		while (search_results->next())
		{
			id++;
			std::string header{ search_results->getString("Book-Title").c_str() };
			header.append(" (ISBN: ");
			header.append(search_results->getString("ISBN").c_str());
			header.append(")##");
			header.append(std::to_string(id));

			if (ImGui::CollapsingHeader(header.c_str()))
			{
				ImGui::Text("Author: ");
				ImGui::SameLine(); ImGui::Text(search_results->getString("Book-Author").c_str());

				if (search_results->getString("checked-out-by").length() > 0)
				{
					ImGui::Text("Unavailable");
					ImGui::Text("To be returned by: ");
					ImGui::SameLine(); ImGui::Text(search_results->getString("expiration-date").c_str());
				}

				std::string filename{ search_results->getString("ISBN").c_str() };
				filename.append(std::to_string(id));

				// download book cover photo
				if (cache.find(filename) == cache.end())
				{
					const char* url = search_results->getString("Image-URL-M").c_str();

					HRESULT hr = URLDownloadToFileA(NULL, url, filename.c_str(), 0, NULL);
					if (hr == S_OK)
					{
						// std::cout << "File downloaded successfully to " << filename.c_str() << std::endl; // debugging
						cache[filename] = filename.c_str();
					}
					else
						std::cerr << "Failed to download file. HRESULT: " << hr << std::endl; // TODO: change
				}

				// load the image; credits: https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
				int my_image_width = 0;
				int my_image_height = 0;
				ID3D11ShaderResourceView* my_texture = NULL;
				bool ret = LoadTextureFromFile(cache[filename].c_str(), &my_texture, &my_image_width, &my_image_height);
				IM_ASSERT(ret);

				ImGui::Image((void*)my_texture, ImVec2(my_image_width, my_image_height));

				// get checkout time 
				std::time_t time = std::time(nullptr);
				std::tm local_time;
				localtime_s(&local_time, &time);

				static char date[11] = {};
				strftime(date, 11, "%Y-%m-%d", &local_time);

				bool schedule_checkout = false;
				ImGui::SetNextItemWidth(ImGui::CalcTextSize("(YYYY-MM-DD)").x);
				ImGui::InputText("Checkout date (YYYY-MM-DD)", date, 11);

				schedule_checkout = ImGui::Button((std::string("Schedule Checkout##").append(std::to_string(id))).c_str());

				if (schedule_checkout)
				{
					if (!user)
					{
						// if not logged in, force login
                        ImGui::OpenPopup("Not logged in");
					}
					else if (has_overdue_items(con, user))
					{
						// if user has overdue items, reject checkout
						ImGui::OpenPopup("Overdue items");
					}
					else
					{
						// otherwise, process checkout
						std::string req{ "INSERT INTO requested_checkouts (ISBN, user, title, date_requested) VALUES (?, ?, ?, '" };
						req += date;
						req += "')";

						pstmt = con->prepareStatement(req.c_str());

						pstmt->setString(1, search_results->getString("ISBN"));

						user->first();

						pstmt->setInt(2, user->getInt("id"));
						pstmt->setString(3, search_results->getString("Book-Title"));
						pstmt->execute();

						delete pstmt;
						pstmt = nullptr;

						memset(date, 0, 11);
					}
				}

				if (ImGui::BeginPopup("Not logged in"))
				{
					ImGui::Text("You are not logged in.");
					ImGui::EndPopup();
				}

				if (ImGui::BeginPopup("Overdue items"))
				{
					ImGui::Text("Please return any overdue items before checking out new ones.");
					ImGui::EndPopup();
				}
			}
		}
	}
	catch (const sql::SQLException& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		delete pstmt;
		pstmt = nullptr;
	}

    ImGui::EndChild();
    return 0;
}
