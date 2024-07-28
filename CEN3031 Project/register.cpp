#include <stdlib.h>
#include <iostream>

#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>

#include <openssl/evp.h>
#include <iomanip>
#include <string>
#include <string.h>
#include <sstream>

#include "register.h"
#include "panel.h"

#include "imgui.h"

sql::ResultSet* user = nullptr;

std::string hash_password(const std::string& password)
{
    // credit: https://wiki.openssl.org/index.php/EVP_Message_Digests
    try
    {
        // set up EVP message digest context
        EVP_MD_CTX* mdctx;
        if ((mdctx = EVP_MD_CTX_new()) == NULL)
            throw std::string("EVP_MD_CTX_new() failed.");
        
        const EVP_MD* md = EVP_sha256();
        unsigned char md_value[EVP_MAX_MD_SIZE];
        unsigned int md_len;

        if (1 != EVP_DigestInit_ex(mdctx, md, nullptr))
            throw std::string("EVP_DigestInit_ex() failed.");

        if (1 != EVP_DigestUpdate(mdctx, password.c_str(), password.length()))
            throw std::string("EVP_DigestUpdate() failed.");

        if (1 != EVP_DigestFinal_ex(mdctx, md_value, &md_len))
            throw std::string("EVP_DigestFinal_ex() failed.");
        
        // context no longer needed
        EVP_MD_CTX_free(mdctx);

        // put digest into string
        std::stringstream ss;
        for (unsigned int i = 0; i < md_len; i++)
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(md_value[i]);

        return ss.str();
    }
    catch (const std::string& e)
    {
        std::cerr << "Error: " << e << std::endl;
        return "";
    }
}

sql::ResultSet* login_register(sql::Connection* con)
{
    // lock window to top right corner
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos;
    ImVec2 work_size = viewport->WorkSize;

    float PAD = 10.0f;
    ImVec2 window_pos = ImVec2(work_pos.x + work_size.x - PAD, work_pos.y + PAD);
    ImVec2 window_pos_pivot = ImVec2(1.0f, 0.0f); // pivot point for top-right corner
	ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);

    static std::string label{ "Login Form##Login/Registration Form" };

    // create window
    static char username[32] = {};
    static char password[32] = {};
    std::string password_hash{};

    if (!user)
    {
		ImGui::Begin(label.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

		ImGui::InputText("Username", username, sizeof(username));
		ImGui::InputText("Password", password, sizeof(password), ImGuiInputTextFlags_Password);

        if (label == "Login Form##Login/Registration Form")
        {
            if (ImGui::Button("Login"))
            {
                // login attempted, process
                if (strlen(username) > 0 && strlen(password) > 0)
                    password_hash = hash_password(password);
            }

            ImGui::Spacing();
            ImGui::Text("New user?");
            ImGui::SameLine(); if (ImGui::Button("Sign up")) { label = "Registration Form##Login/Registration Form"; };
        }
        else
        {
            if (ImGui::Button("Register"))
            {
                // registration attempted, process
                if (strlen(username) > 0 && strlen(password) > 0)
                    password_hash = hash_password(password);
            }

            ImGui::Spacing();
            ImGui::Text("Already have an account?");
            ImGui::SameLine(); if (ImGui::Button("Sign in")) { label = "Login Form##Login/Registration Form"; };
        }
    }
    else
    {
		// TODO: add full name to registration
        label.assign("Logged in##Login/Registration Form");
		ImGui::Begin(label.c_str(), nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

		user->beforeFirst();
        if (ImGui::Button("Logout", ImVec2(0, 0.0f)))
        {
            label = "Login Form##Login/Registration Form";
            delete user;
            user = nullptr;
        }

        ImGui::End();
        return user;
    }
	ImGui::End();

    // if not yet attempted login/registration, don't bother with SQL
    if (password_hash.length() <= 0)
        return nullptr;

    sql::PreparedStatement* pstmt = nullptr;

    try
    {
        if (label == "Login Form##Login/Registration Form")
        {
            pstmt = con->prepareStatement("SELECT * FROM users WHERE username = ? AND password_hash = ?");
			pstmt->setString(1, username);
			pstmt->setString(2, password_hash);
            pstmt->execute();

			sql::ResultSet* res = pstmt->getResultSet();

            if (!res->next())
                res = nullptr;

			memset(username, 0, sizeof(username));
			memset(password, 0, sizeof(password));
			password_hash.clear();

			delete pstmt;
			pstmt = nullptr;
            return res;
        }
        else
        {
			pstmt = con->prepareStatement("INSERT INTO users (username, password_hash, role) VALUES (?, ?, ?)");
			pstmt->setString(1, username);
			pstmt->setString(2, password_hash);
			pstmt->setString(3, "user");
			pstmt->execute();

			memset(username, 0, sizeof(username));
			memset(password, 0, sizeof(password));
			password_hash.clear();
        }
    }
    catch (sql::SQLException e)
    {
        std::cout << "Error: " << e.what() << std::endl;
    }

	delete pstmt;
	pstmt = nullptr;
    return nullptr;
}