#include <stdlib.h>
#include <iostream>

#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>

#include <openssl/evp.h>
#include <iomanip>
#include <string>
#include <string.h>
#include <sstream>

#include "imgui.h"

// TODO: make/move global vars
const std::string server = "tcp://127.0.0.1:3306";
const std::string db_username = "root";
const std::string db_password = "ADMIN";

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

int register_user()
{
	ImGui::Begin("Registration Form");

    static char username[32] = {};
    static char password[32] = {};
    std::string password_hash{};

	ImGui::InputText("Username", username, sizeof(username));
	ImGui::InputText("Password", password, sizeof(password), ImGuiInputTextFlags_Password);

    if (ImGui::Button("Register"))
    {
        // registration attempted, process
        if (strlen(username) > 0 && strlen(password) > 0)
            password_hash = hash_password(password);
    }
	ImGui::End();

    // if not yet attempted login, don't bother with SQL
    if (password_hash.length() <= 0)
        return 0;

    sql::Driver* driver = nullptr;
    sql::Connection* con = nullptr;
    sql::Statement* stmt = nullptr;
    sql::PreparedStatement* pstmt = nullptr;

    try
    {
        driver = get_driver_instance();
        con = driver->connect(server, db_username, db_password);

        con->setSchema("test_db");

        pstmt = con->prepareStatement("INSERT INTO users(username, password_hash, role) VALUES (?, ?, ?)");
        pstmt->setString(1, username);
        pstmt->setString(2, password_hash);
        pstmt->setString(3, "user");
        pstmt->execute();

        memset(username, 0, sizeof(username));
        memset(password, 0, sizeof(password));
        password_hash.clear();
    }
    catch (sql::SQLException e)
    {
        std::cout << "Error: " << e.what() << std::endl;

        delete pstmt;
        delete con;
        delete stmt;

        return 1;
    }

	delete pstmt;
	delete con;
	delete stmt;

    return 0;
}