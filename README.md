
# ShadowSentinel Honeypot Tool

**ShadowSentinel** is a high-interaction Windows honeypot tool designed to deceive attackers by simulating sensitive data, fake administrative tools, and misleading system feedback. It uses decoy files, fake admin consoles, and real-time logging of suspicious activities, all while sending alerts to Discord for monitoring.

---

##  Features: 

### 1. **Fake Admin Console**
   - Simulates an administrative control panel to engage attackers.
   - Displays fake system status and logs.
  

### 2. **Fake Sensitive Files**
   - Generates and hides fake files that contain mock credentials, bank information, personal details, and system configurations.
   - These files are designed to look legitimate and entice attackers to interact with them.
   - Examples of fake files include:
     - `confidential_report.docx`
     - `bank_info.csv`
     - `logins_backup.txt`

### 3. **File Access Logging**
   - Tracks every file access attempt, logging the time, user, and action.
   - Logs are base64-encoded and stored in a secure location.
   - Monitors and logs file creation, modification, and deletion events.

### 4. **Misleading Feedback**
   - Displays misleading system messages (e.g., "Access Denied", "File Corrupted") when an attacker tries to interact with a fake file or command.
   - Can lock files after access attempts to simulate real ransomware behavior.

### 5. **Real-Time Alerts via Discord Webhook**
   - Sends real-time alerts to a Discord webhook whenever suspicious activity is detected.
   - Alerts include details like the file accessed, username, process ID, and the time of the event.
   - Provides detailed alerts including the suspicion level and the type of access.

### 6. **File Locking**
   - Implements file locking by setting files as read-only after access
   - Traps attackers into interacting with locked files, giving more time for analysis.

---
 Installation: 

### Prerequisites:
1. C++ Compiler (e.g., Visual Studio or MinGW) for building the tool.
2. CURL: library for sending Discord webhook alerts.
3. Windows OS (Tested on Windows 10/11).

 Steps:
1. **Clone the Repository:**
   ```bash
   git clone https://github.com/yourusername/ShadowSentinel.git
   cd ShadowSentinel
