#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <filesystem>
#include <chrono>
#include <random>
#include <curl/curl.h>
#include <unordered_map>
#include <cstdlib>  // for system()


std::unordered_map<std::string, int> accessCountMap;
std::unordered_map<std::string, std::string> lastAccessMap;


namespace fs = std::filesystem;


static const std::string BASE64_ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void sendDiscordWebhook(const std::string& message);

std::string base64Encode(const std::string& input) {
    std::string encoded;
    int padding = 0;
    int inputLength = input.size();
    unsigned char byte1, byte2, byte3;
    unsigned char encoded1, encoded2, encoded3, encoded4;

    for (int i = 0; i < inputLength; i += 3) {
        byte1 = input[i];
        byte2 = (i + 1 < inputLength) ? input[i + 1] : 0;
        byte3 = (i + 2 < inputLength) ? input[i + 2] : 0;

        encoded1 = (byte1 >> 2);
        encoded2 = ((byte1 & 0x03) << 4) | (byte2 >> 4);
        encoded3 = ((byte2 & 0x0F) << 2) | (byte3 >> 6);
        encoded4 = byte3 & 0x3F;

        encoded.push_back(BASE64_ALPHABET[encoded1]);
        encoded.push_back(BASE64_ALPHABET[encoded2]);

        if (i + 1 < inputLength) encoded.push_back(BASE64_ALPHABET[encoded3]);
        else encoded.push_back('='), padding++;

        if (i + 2 < inputLength) encoded.push_back(BASE64_ALPHABET[encoded4]);
        else encoded.push_back('='), padding++;
    }

    return encoded;
}

#include <sstream>

std::string escapeJson(const std::string& input) {
    std::ostringstream escaped;
    for (char c : input) {
        switch (c) {
        case '"': escaped << "\\\""; break;
        case '\\': escaped << "\\\\"; break;
        case '\n': escaped << "\\n"; break;
        case '\r': escaped << "\\r"; break;
        case '\t': escaped << "\\t"; break;
        default: escaped << c;
        }
    }
    return escaped.str();
}


void sendDiscordWebhook(const std::string& title, const std::vector<std::pair<std::string, std::string>>& fields) {
    CURL* curl;
    CURLcode res;

    const std::string webhookURL = "https://discord.com/api/webhooks/1365946350398738463/nhS5k0DIJCSGXALY8gpr1pf9N685tkZn39OX-zBkpJcfUC_btH0hZhWvTgGpaGhMPxz4";

    curl = curl_easy_init();
    if (curl) {
        std::ostringstream json;
        json << "{ \"embeds\": [{"
            << "\"title\": \"" << escapeJson(title) << "\","
            << "\"color\": 16711680,"
            << "\"fields\": [";

        for (size_t i = 0; i < fields.size(); ++i) {
            json << "{"
                << "\"name\": \"" << escapeJson(fields[i].first) << "\","
                << "\"value\": \"" << escapeJson(fields[i].second) << "\","
                << "\"inline\": false"
                << "}";

            if (i != fields.size() - 1) json << ",";
        }

        json << "]}]}";

        std::string payload = json.str();

        curl_easy_setopt(curl, CURLOPT_URL, webhookURL.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Failed to send webhook embed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

// Wrapper to support single-message webhook sending
void sendDiscordWebhook(const std::string& message) {
    sendDiscordWebhook(message, {});
}


void showAlert(const std::string& message) {
    MessageBoxA(NULL, message.c_str(), "ShadowSentinel Alert", MB_OK | MB_ICONWARNING);
}
std::string getCurrentUsername() {
    char username[256];
    DWORD size = sizeof(username);
    if (GetUserNameA(username, &size)) {
        return std::string(username);
    }
    return "Unknown";
}

std::string getLastAccessTime(const std::string& filePath) {
    HANDLE file = CreateFileA(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return "Unable to get time";
    }

    FILETIME lastAccessTime;
    if (GetFileTime(file, NULL, &lastAccessTime, NULL)) {
        SYSTEMTIME st;
        FileTimeToSystemTime(&lastAccessTime, &st);
        char buffer[100];
        sprintf_s(buffer, "%04d-%02d-%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        CloseHandle(file);
        return std::string(buffer);
    }

    CloseHandle(file);
    return "Unable to get time";
}

std::string getProcessName() {
    char processPath[MAX_PATH];
    if (GetModuleFileNameA(NULL, processPath, MAX_PATH)) {
        return std::string(processPath);
    }
    return "Unknown";
}


void logEvent(const std::string& message, bool alert = false) {
    fs::create_directories("logs");
    std::ofstream log("logs/events.log", std::ios::app);

    SYSTEMTIME st;
    GetLocalTime(&st);
    char timestamp[100];
    sprintf_s(timestamp, "%04d-%02d-%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    // Get the username and last access time
    std::string username = getCurrentUsername();
    std::string lastAccessTime = getLastAccessTime(message);

    // Get the current process ID and executable name
    DWORD pid = GetCurrentProcessId();
    std::string processName = getProcessName();

    // Create the full log message
    std::string fullMessage = std::string("[") + timestamp + "] [ACCESS] File: " + message +
        " | User: " + username +
        " | Last Accessed: " + lastAccessTime +
        " | PID: " + std::to_string(pid) +
        " | Process: " + processName;

    std::string encodedMessage = base64Encode(fullMessage);
    log << encodedMessage << std::endl;

    if (alert) {
        showAlert("Suspicious activity detected: " + message);
        sendDiscordWebhook("Suspicious activity detected: " + message);

        // Optionally send PID/Process in alert
        std::vector<std::pair<std::string, std::string>> fields = {
            {"Username", username},
            {"File Accessed", message},
            {"Last Access Time", lastAccessTime},
            {"Process ID", std::to_string(pid)},
            {"Process Name", processName},
            {"Location", "C:\\Honeytokens\\" + message}
        };

        sendDiscordWebhook("Suspicious File Access Detected", fields);
    }
}



void misleadingFeedback(const std::string& fileName) {
    std::vector<std::string> errors = {
        "Access Denied", "File Corrupted", "System Error: 0x80070005", "Permission Violation"
    };
    int index = rand() % errors.size();
    std::cout << "[!] " << errors[index] << ": " << fileName << std::endl;
}

void lockFile(const std::string& filepath) {
    DWORD dwAttrs = GetFileAttributesA(filepath.c_str());
    if (dwAttrs != INVALID_FILE_ATTRIBUTES) {
        dwAttrs |= FILE_ATTRIBUTE_READONLY;
        SetFileAttributesA(filepath.c_str(), dwAttrs);
    }
}

std::string getRandomFakeData() {
    std::vector<std::string> fakeData = {
        "SSN: 321-65-9876\nName: Alexandaer Johnson\nDOB: 04/12/1975",
        "Bank Name: TrustyBank\nAccount: 9988776655\nRouting: 123456789",
        "Email: kime424@gmail.com\nPassword: ilovepizza2025",
        "API_KEY=AIzaSyD-FAKE-KEY-EXAMPLE",
        "{\"username\":\"admin\",\"password\":\"Iloveme38485hj5\"}",
        "DROP DATABASE production; -- SQL Injection Test",
        "<xml><user>jane</user><pass>qwerty</pass></xml>",
        "Card #: 4111 1111 1111 1111\nExp: 12/25\nCVV: 999",
        "Token: abcdef1234567890fake",

    };
    return fakeData[rand() % fakeData.size()];
}

std::string generateFakeJsonCredentials() {
    std::stringstream fakeJson;
    fakeJson << "{\n"
        << "  \"username\": \"admin\",\n"
        << "  \"password\": \"password1234!\",\n"
        << "  \"email\": \"admin@fakeemail.com\",\n"
        << "  \"api_token\": \"ghp_ABC1234FakeToken56789!\",\n"
        << "  \"role\": \"administrator\",\n"
        << "  \"last_login\": \"2025-04-30T14:35:00Z\"\n"
        << "}";
    return fakeJson.str();
}

std::string generateFakeBankInfo() {
    std::stringstream fakeBankInfo;
    fakeBankInfo << "Account Holder: John Doe\n"
        << "Bank: Fake Trusty Bank\n"
        << "Account Number: 1234567890\n"
        << "Routing Number: 987654321\n"
        << "SWIFT Code: FTBBUS33\n"
        << "IBAN: US12345678901234567890\n"
        << "Balance: $12,345.67\n"
        << "Last Transaction: $500.00 - Transfer to Jane Doe\n";
    return fakeBankInfo.str();
}


std::string generateFakeInternalMemo() {
    std::stringstream fakeMemo;
    fakeMemo << "To: All Employees\n"
        << "From: CEO\n"
        << "Subject: Q3 Restructuring Plan\n\n"
        << "Dear Team,\n\n"
        << "As part of our ongoing restructuring efforts, we will be moving forward with some significant changes. "
        << "These changes will affect various departments, and all team leads are required to submit their revised plans "
        << "by the end of the month. Please keep this information confidential until the official announcement.\n\n"
        << "Best regards,\n"
        << "CEO, FakeCorp";
    return fakeMemo.str();
}

std::string generateFakeCreditCardInfo() {
    std::stringstream fakeCardInfo;
    fakeCardInfo << "Cardholder Name: Jane Doe\n"
        << "Card Number: 4111 1111 1111 1111\n"
        << "Expiration Date: 12/25\n"
        << "CVV: 123\n"
        << "Billing Address: 123 Fake Street, Faketown, FA 12345\n";
    return fakeCardInfo.str();
}

std::string generateFakeSystemConfig() {
    std::stringstream fakeConfig;
    fakeConfig << "[Database]\n"
        << "host=localhost\n"
        << "port=3306\n"
        << "user=admin\n"
        << "password=admin123\n"
        << "dbname=production_db\n\n"
        << "[API]\n"
        << "key=384h4n8n3sA9j4Pi\n"
        << "endpoint=https://api.Capitalone.com\n";
    return fakeConfig.str();
}


void generateFakeFiles(const std::string& folder) {
    fs::create_directories(folder);

    if (fs::is_empty(folder)) {
        std::vector<std::string> filenames;
        std::vector<std::string> baseNames = {
            "confidential_report", "bank_info", "logins_backup", "crypto_keys",
            "q2_financials", "passwords", "server_credentials", "admin_config",
            "sensitive_data", "client_records", "meeting_notes", "internal_report",
            "company_data", "invoice_details", "financials", "project_plans",
            "legal_documents", "backup_schedule", "key_backup", "user_settings",
            "network_logs", "admin_credentials", "database_dump", "login_activity", "client_feedback"
        };

        std::vector<std::string> extensions = {
            ".docx", ".xlsx", ".txt", ".json", ".pdf", ".xml", ".csv", ".db", ".sql"
        };

        for (size_t i = 0; i < 25; ++i) {
            std::string name = baseNames[i] + extensions[rand() % extensions.size()];
            filenames.push_back(name);
        }

        for (const auto& name : filenames) {
            std::ofstream file(folder + "\\" + name);
            if (file.is_open()) {
                if (name.ends_with(".json")) {
                    file << generateFakeJsonCredentials();  // JSON file with credentials
                }
                else if (name.ends_with("file.txt")) {
                    file << generateFakeBankInfo();  // Bank info in .txt file
                }

                else if (name.ends_with(".xml") || name.ends_with(".docx") || name.ends_with(".pdf")) {
                    file << generateFakeInternalMemo();  // Fake internal memo in .txt file
                }
                else if (name.ends_with(".csv")) {
                    file << generateFakeCreditCardInfo();  // Fake credit card info in .csv file
                }
                else {
                    file << generateFakeSystemConfig();  // Fake system config data
                }
                file.close();
            }
        }
    }
}


void watchHoneytokenDir(const std::string& path, int& countdownTime) {
    HANDLE dir = CreateFileA(
        path.c_str(), FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS, NULL
    );

    if (dir == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open directory handle." << std::endl;
        return;
    }

    char buffer[1024];
    DWORD bytesReturned;
    int fileAccessCount = 0;  // Track number of access events

    while (true) {
        if (ReadDirectoryChangesW(dir, buffer, sizeof(buffer), TRUE,
            FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_FILE_NAME,
            &bytesReturned, NULL, NULL)) {

            FILE_NOTIFY_INFORMATION* info = (FILE_NOTIFY_INFORMATION*)buffer;
            std::wstring filename(info->FileName, info->FileNameLength / sizeof(WCHAR));
            std::string accessedFile(filename.begin(), filename.end());

            std::string actionStr;
            switch (info->Action) {
            case FILE_ACTION_ADDED:
                actionStr = "File Created";
                break;
            case FILE_ACTION_REMOVED:
                actionStr = "File Deleted";
                break;
            case FILE_ACTION_MODIFIED:
                actionStr = "File Modified";
                break;
            case FILE_ACTION_RENAMED_OLD_NAME:
                actionStr = "File Renamed (From)";
                break;
            case FILE_ACTION_RENAMED_NEW_NAME:
                actionStr = "File Renamed (To)";
                break;
            default:
                actionStr = "Unknown Action";
                break;
            }

            // Increment access counter
            fileAccessCount++;

            // Log the access event
            logEvent(actionStr + ": " + accessedFile, true);


            misleadingFeedback(accessedFile);  // Display a misleading message
            lockFile(path + "\\" + accessedFile);  // Lock the file

            std::vector<std::pair<std::string, std::string>> fields = {
    {"Username", getCurrentUsername()},
    {"File Accessed", accessedFile},
    {"Last Access Time", getLastAccessTime(accessedFile)},
    {"Access Count", std::to_string(fileAccessCount)},
    {"Suspicion Level", "HIGH"},
    {"Location", "C:\\Honeytokens\\" + accessedFile}
            };

            sendDiscordWebhook("Suspicious File Access Detected", fields);

            // Simulate different types of access (open, delete, rename, etc.)
            if (accessedFile == "confidential_report.docx") {
                logEvent("Attempt to access confidential report.", true);
                misleadingFeedback(accessedFile);
            }
            else if (accessedFile == "bank_info.csv") {
                logEvent("Sensitive bank information accessed.", true);
                misleadingFeedback(accessedFile);
            }
            else if (accessedFile == "logins_backup.txt") {
                logEvent("Backup of login credentials accessed.", true);
                misleadingFeedback(accessedFile);
            }
            else if (accessedFile == "crypto_keys.db") {
                logEvent("Crypto keys accessed.", true);
                misleadingFeedback(accessedFile);
            }
            else if (accessedFile == "q2_financials.xlsx") {
                logEvent("Q2 financials spreadsheet accessed.", true);
                misleadingFeedback(accessedFile);
            }

            // Output the current access count
            std::cout << "File accessed " << fileAccessCount << " times." << std::endl;

            // Example condition to stop monitoring after 10 accesses (or "clicks")
            if (fileAccessCount >= 10) {
                std::cout << "[!] Max access count reached. Stopping the simulation." << std::endl;
                break;
            }
        }
        Sleep(500); // Wait for 500 ms before checking again
    }

    CloseHandle(dir);
}



int main() {
    srand((unsigned int)time(NULL));

    const std::string honeytokenDir = "C:\\Honeytokens";
    int countdownTime = 60;

    generateFakeFiles(honeytokenDir);

    std::thread fakeAdminThread([]() {
        MessageBoxA(NULL, "System Health: OK\nUnauthorized Accesses: 2\n\n[View Access Logs]  [Scan System]  [Lock Down]", "ShadowSentinel Admin Console", MB_OK | MB_ICONINFORMATION);
        });
    fakeAdminThread.detach();

    std::thread watcherThread(watchHoneytokenDir, honeytokenDir, std::ref(countdownTime));
    watcherThread.join();

    return 0;
}
