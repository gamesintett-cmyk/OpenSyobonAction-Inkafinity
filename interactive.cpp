#include "interactive.h"
#include "main.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET socket_t;
#define CLOSESOCKET closesocket
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int socket_t;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define CLOSESOCKET close
#endif

struct InkEvent {
    std::string action;
    std::string enemy;
    std::string userName;
    int quantity = 1;
    int type = 0;
    int seconds = 0;
};

static std::atomic<bool> g_running(false);
static std::thread g_thread;
static std::mutex g_mutex;
static std::queue<InkEvent> g_events;
static std::string g_nextViewerName;
static std::string g_enemyNames[amax];
static int g_lastEventTicks = 0;
static std::string g_lastStatus = "Inkafinity HTTP :5755";
static int g_speedBoostFrames = 0;
static int g_freezeFrames = 0;
static int g_reverseFrames = 0;

static std::string urlDecode(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size()) {
            char hex[3] = {s[i + 1], s[i + 2], 0};
            out.push_back((char)strtol(hex, nullptr, 16));
            i += 2;
        } else if (s[i] == '+') out.push_back(' ');
        else out.push_back(s[i]);
    }
    return out;
}

static std::string getParam(const std::string& query, const std::string& key, const std::string& def = "") {
    size_t start = 0;
    while (start <= query.size()) {
        size_t end = query.find('&', start);
        std::string part = query.substr(start, end == std::string::npos ? std::string::npos : end - start);
        size_t eq = part.find('=');
        std::string k = urlDecode(eq == std::string::npos ? part : part.substr(0, eq));
        if (k == key) return urlDecode(eq == std::string::npos ? "" : part.substr(eq + 1));
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return def;
}

static int getIntParam(const std::string& q, const std::string& key, int def) {
    std::string v = getParam(q, key, "");
    if (v.empty()) return def;
    int n = atoi(v.c_str());
    return n;
}

static void pushEvent(const InkEvent& ev) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_events.size() < 100) g_events.push(ev);
}

static void sendText(socket_t client, const std::string& body) {
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n"
        << "Content-Type: text/plain; charset=utf-8\r\n"
        << "Access-Control-Allow-Origin: *\r\n"
        << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        << "Access-Control-Allow-Headers: Content-Type\r\n"
        << "Content-Length: " << body.size() << "\r\n\r\n"
        << body;
    std::string resp = oss.str();
    send(client, resp.c_str(), (int)resp.size(), 0);
}

static void handleClient(socket_t client) {
    char buffer[4096];
    int n = recv(client, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) { CLOSESOCKET(client); return; }
    buffer[n] = 0;
    std::string req(buffer);
    std::string first = req.substr(0, req.find("\r\n"));
    std::istringstream iss(first);
    std::string method, target;
    iss >> method >> target;

    if (method == "OPTIONS") { sendText(client, "OK"); CLOSESOCKET(client); return; }

    size_t qpos = target.find('?');
    std::string path = qpos == std::string::npos ? target : target.substr(0, qpos);
    std::string query = qpos == std::string::npos ? "" : target.substr(qpos + 1);

    InkEvent ev;
    if (path == "/spawn" || path == "/webhook") {
        ev.action = "spawn";
        ev.enemy = getParam(query, "enemy", getParam(query, "id", "0"));
        ev.type = getIntParam(query, "type", getIntParam(query, "id", 0));
        ev.quantity = std::max(1, std::min(20, getIntParam(query, "quantity", getIntParam(query, "value", 1))));
        ev.userName = getParam(query, "userName", getParam(query, "username", getParam(query, "nickname", "viewer")));
        pushEvent(ev);
        sendText(client, "OK spawn queued");
    } else if (path == "/effect") {
        ev.action = getParam(query, "type", "");
        ev.seconds = std::max(1, std::min(60, getIntParam(query, "seconds", 5)));
        ev.userName = getParam(query, "userName", getParam(query, "username", getParam(query, "nickname", "viewer")));
        pushEvent(ev);
        sendText(client, "OK effect queued");
    } else if (path == "/kill") {
        ev.action = "kill";
        ev.userName = getParam(query, "userName", getParam(query, "username", "viewer"));
        pushEvent(ev);
        sendText(client, "OK kill queued");
    } else if (path == "/clear") {
        ev.action = "clear";
        pushEvent(ev);
        sendText(client, "OK clear queued");
    } else if (path == "/status") {
        sendText(client, "OK OpenSyobonAction Inkafinity server running on :5755");
    } else {
        sendText(client, "OpenSyobonAction Inkafinity endpoints: /spawn, /effect, /kill, /clear, /status");
    }
    CLOSESOCKET(client);
}

static void serverLoop(int port) {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
#endif
    socket_t serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == INVALID_SOCKET) return;
    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons((unsigned short)port);
    if (bind(serverFd, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        CLOSESOCKET(serverFd);
        return;
    }
    listen(serverFd, 16);
    while (g_running.load()) {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(serverFd, &set);
        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 200000;
        int rv = select((int)serverFd + 1, &set, nullptr, nullptr, &tv);
        if (rv > 0 && FD_ISSET(serverFd, &set)) {
            socket_t client = accept(serverFd, nullptr, nullptr);
            if (client != INVALID_SOCKET) handleClient(client);
        }
    }
    CLOSESOCKET(serverFd);
#ifdef _WIN32
    WSACleanup();
#endif
}

void Inkafinity_StartServer(int port) {
    if (g_running.load()) return;
    g_running = true;
    g_thread = std::thread(serverLoop, port);
}

void Inkafinity_StopServer() {
    g_running = false;
    if (g_thread.joinable()) g_thread.join();
}

static int enemyTypeFromName(const std::string& enemy, int fallback) {
    std::string e = enemy;
    std::transform(e.begin(), e.end(), e.begin(), [](unsigned char c){ return std::tolower(c); });
    if (e == "cat" || e == "syobon" || e == "goomba") return 0;
    if (e == "jump" || e == "jumper") return 3;
    if (e == "mushroom" || e == "kinoko") return 100;
    if (e == "poison" || e == "dokukinoko") return 102;
    if (e == "star" || e == "badstar") return 110;
    if (!e.empty() && std::isdigit((unsigned char)e[0])) return atoi(e.c_str());
    return fallback;
}

void Inkafinity_SetNextViewerName(const std::string& name) {
    g_nextViewerName = name.substr(0, 32);
}

std::string Inkafinity_TakeNextViewerName() {
    std::string n = g_nextViewerName;
    g_nextViewerName.clear();
    return n;
}

void Inkafinity_SetEnemyViewerName(int slot, const std::string& name) {
    if (slot >= 0 && slot < amax) g_enemyNames[slot] = name.substr(0, 32);
}

const char* Inkafinity_GetEnemyViewerName(int slot) {
    if (slot < 0 || slot >= amax) return "";
    return g_enemyNames[slot].c_str();
}

void Inkafinity_ProcessEvents() {
    if (g_speedBoostFrames > 0) { g_speedBoostFrames--; if (mactsok < 2) mactsok = 2; }
    if (g_freezeFrames > 0) { g_freezeFrames--; mc = 0; }
    if (g_reverseFrames > 0) { g_reverseFrames--; }

    std::queue<InkEvent> local;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        std::swap(local, g_events);
    }
    while (!local.empty()) {
        InkEvent ev = local.front();
        local.pop();
        g_lastEventTicks = (int)GetNowCount();
        if (ev.action == "spawn") {
            int etype = enemyTypeFromName(ev.enemy, ev.type);
            if (etype < 0) etype = 0;
            if (etype > 159) etype = 0;
            for (int i = 0; i < ev.quantity; ++i) {
                int xoff = 7000 + i * 2200;
                int yoff = -2500 - (i % 3) * 800;
                Inkafinity_SetNextViewerName(ev.userName);
                ayobi(ma + fx + xoff, mb + fy + yoff, 0, 0, 0, etype, 0);
            }
            g_lastStatus = "Spawn x" + std::to_string(ev.quantity) + " by " + ev.userName;
        } else if (ev.action == "kill") {
            mhp = 0;
            g_lastStatus = "Kill by " + ev.userName;
        } else if (ev.action == "clear") {
            for (int i = 0; i < amax; ++i) { aa[i] = -80000000; g_enemyNames[i].clear(); }
            g_lastStatus = "Enemies cleared";
        } else if (ev.action == "speed") {
            g_speedBoostFrames = ev.seconds * 30;
            g_lastStatus = "Speed effect by " + ev.userName;
        } else if (ev.action == "freeze") {
            g_freezeFrames = ev.seconds * 30;
            g_lastStatus = "Freeze effect by " + ev.userName;
        } else if (ev.action == "reverse") {
            g_reverseFrames = ev.seconds * 30;
            g_lastStatus = "Reverse queued by " + ev.userName;
        }
    }
}

void Inkafinity_DrawHud() {
    if (mainZ == 1) {
        DrawString(8, 8, "Inkafinity HTTP :5755", GetColor(255, 255, 255));
        if ((int)GetNowCount() - g_lastEventTicks < 4000) {
            DrawString(8, 28, g_lastStatus.c_str(), GetColor(255, 255, 0));
        }
    }
}
