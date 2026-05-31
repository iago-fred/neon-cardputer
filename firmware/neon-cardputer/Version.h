#ifndef VERSION_H
#define VERSION_H

// ============================================================
// Versão do Firmware
// ============================================================
#define FIRMWARE_VERSION_MAJOR 0
#define FIRMWARE_VERSION_MINOR 2
#define FIRMWARE_VERSION_PATCH 0

#define FIRMWARE_VERSION_STR  "0.2.0"
#define FIRMWARE_NAME         "Neon Widget"
#define FIRMWARE_AUTHOR       "Iago & Neon"

// URL do repositório para OTA
#define GITHUB_REPO_OWNER     "iago-fred"
#define GITHUB_REPO_NAME      "neon-cardputer"
#define GITHUB_API_URL        "https://api.github.com/repos/" GITHUB_REPO_OWNER "/" GITHUB_REPO_NAME "/releases/latest"
#define GITHUB_RELEASE_URL    "https://github.com/" GITHUB_REPO_OWNER "/" GITHUB_REPO_NAME "/releases"

// ============================================================
// Utilitários de versão
// ============================================================
struct FirmwareVersion {
    int major;
    int minor;
    int patch;
};

inline FirmwareVersion parseVersion(const char* str) {
    FirmwareVersion v = {0, 0, 0};
    if (!str || *str == '\0') return v;
    
    // Pula 'v' inicial se houver
    if (*str == 'v') str++;
    
    sscanf(str, "%d.%d.%d", &v.major, &v.minor, &v.patch);
    return v;
}

inline bool isNewerVersion(const FirmwareVersion& current, const FirmwareVersion& remote) {
    if (remote.major > current.major) return true;
    if (remote.major < current.major) return false;
    if (remote.minor > current.minor) return true;
    if (remote.minor < current.minor) return false;
    if (remote.patch > current.patch) return true;
    return false;
}

inline const char* getCurrentVersionStr() {
    return FIRMWARE_VERSION_STR;
}

inline FirmwareVersion getCurrentVersion() {
    FirmwareVersion v;
    v.major = FIRMWARE_VERSION_MAJOR;
    v.minor = FIRMWARE_VERSION_MINOR;
    v.patch = FIRMWARE_VERSION_PATCH;
    return v;
}

#endif // VERSION_H
