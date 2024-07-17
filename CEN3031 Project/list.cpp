bool list = false;

#include <d3d11.h>

#include <unordered_map>

#include "imgui.h"
#include "list.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// cache of book covers (id, img filename)
std::unordered_map<int, std::string> cache;

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

sql::ResultSet* previous_results = nullptr;

int listings(sql::ResultSet* search_results)
{
    if (!list)
        return 0;

    // TODO: need to fix this
    if (search_results && !previous_results)
        previous_results = search_results;

    ImGui::Begin("Listings");

    if (!previous_results)
    {
        ImGui::Text("No results.");
        ImGui::End();
        return 0;
    }

    int id = 0;
    while (previous_results->next())
    {
        id++;
        std::string header{ previous_results->getString("Book-Title").c_str() };
        header.append(" (ISBN: ");
		header.append(previous_results->getString("ISBN").c_str());
        header.append(")##");
        header.append(std::to_string(id));

        if (ImGui::CollapsingHeader(header.c_str()))
        {
            ImGui::Text(previous_results->getString("Book-Author").c_str());

            // download book cover photo (TODO: cache result and delete on program exit)
            if (cache.find(id) == cache.end())
            {
                const char* url = previous_results->getString("Image-URL-M").c_str();

                HRESULT hr = URLDownloadToFileA(NULL, url, std::to_string(id).c_str(), 0, NULL);
                if (hr == S_OK)
                {
                    std::cout << "File downloaded successfully to " << std::to_string(id).c_str() << std::endl;
                    cache[id] = std::to_string(id).c_str();
                }
                else
                    std::cout << "Failed to download file. HRESULT: " << hr << std::endl; // TODO: change
            }

            // load the image; credits: https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
            int my_image_width = 0;
            int my_image_height = 0;
            ID3D11ShaderResourceView* my_texture = NULL;
            bool ret = LoadTextureFromFile(std::to_string(id).c_str(), &my_texture, &my_image_width, &my_image_height);
            IM_ASSERT(ret);

            ImGui::Image((void*)my_texture, ImVec2(my_image_width, my_image_height));
            ImGui::Button("Request Checkout");
        }
    }
    previous_results->beforeFirst();

    ImGui::End();
}
