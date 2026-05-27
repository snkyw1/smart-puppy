#ifndef BOARD_H
#define BOARD_H

#include <http.h>
#include <web_socket.h>
#include <mqtt.h>
#include <udp.h>
#include <string>
#include <functional>
#include <network_interface.h>

extern "C" uint8_t lcd_get_brightness();
extern "C" void lcd_set_brightness(uint8_t brightness);

// Power save level enumeration
enum class PowerSaveLevel {
    LOW_POWER,    // Maximum power saving (lowest power consumption)
    BALANCED,     // Medium power saving (balanced)
    PERFORMANCE,  // No power saving (maximum power consumption / full performance)
};


class AudioCodec;

class Board {
private:
    Board(const Board&) = delete; // 禁用拷贝构造函数
    Board& operator=(const Board&) = delete; // 禁用赋值操作

protected: 
    std::string GenerateUuid();

    // 软件生成的设备唯一标识
    std::string uuid_;

    Board();

public:
    static Board& GetInstance() {
        static Board instance;
        return instance;
    }
    // static Board& GetInstance() {
    //     static Board* instance = static_cast<Board*>(create_board());
    //     return *instance;
    // }

    ~Board() = default;

    std::string GetUuid() { return uuid_; }

    AudioCodec* GetAudioCodec();
    NetworkInterface* GetNetwork();
    std::string GetSystemInfoJson();
    void SetPowerSaveLevel(PowerSaveLevel level);
    std::string GetBoardJson();
    std::string GetDeviceStatusJson();
};


#endif // BOARD_H
