#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <esp_timer.h>
#include "esp_mmap_assets.h"

// Main event bits
#define MAIN_EVENT_SCHEDULE             (1 << 0)
#define MAIN_EVENT_SEND_AUDIO           (1 << 1)
#define MAIN_EVENT_WAKE_WORD_DETECTED   (1 << 2)
#define MAIN_EVENT_ERROR                (1 << 3)
#define MAIN_EVENT_ACTIVATION_DONE      (1 << 4)
#define MAIN_EVENT_NETWORK_CONNECTED    (1 << 5)
#define MAIN_EVENT_NETWORK_DISCONNECTED (1 << 6)
#define MAIN_EVENT_TOGGLE_CHAT          (1 << 7)
#define MAIN_EVENT_START_LISTENING      (1 << 8)
#define MAIN_EVENT_STOP_LISTENING       (1 << 9)
#define MAIN_EVENT_STATE_CHANGED        (1 << 10)
#define MAIN_EVENT_CLOCK_TICK           (1 << 11)
#define MAIN_EVENT_CHARGE_STATE_CHANGE  (1 << 12)

#ifdef __cplusplus
#include <string>
#include <mutex>
#include <deque>
#include <memory>
#include <functional>

#include "protocol.h"
#include "ota.h"
#include "audio_service.h"
#include "device_state_machine.h"

enum AecMode {
    kAecOff,
    kAecOnDeviceSide,
    kAecOnServerSide,
};

class Application {
public:
    static Application& GetInstance() {
        static Application instance;
        return instance;
    }
    // Delete copy constructor and assignment operator
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    /**
     * Initialize the application
     * This sets up display, audio, network callbacks, etc.
     * Network connection starts asynchronously.
     */
    void Initialize();

    /**
     * Run the main event loop
     * This function runs in the main task and never returns.
     * It handles all events including network, state changes, and user interactions.
     */
    void Run();

    DeviceState GetDeviceState() const { return state_machine_.GetState(); }
    bool IsVoiceDetected() const { return audio_service_.IsVoiceDetected(); }
    
    /**
     * Request state transition
     * Returns true if transition was successful
     */
    bool SetDeviceState(DeviceState state);

    /**
     * Schedule a callback to be executed in the main task
     */
    void Schedule(std::function<void()>&& callback);

    /**
     * Alert with status, message, emotion and optional sound
     */
    void Alert(const char* status, const char* message, const char* emotion, int sound_index = -1) ;
    void DismissAlert();

    void AbortSpeaking(AbortReason reason);

    /**
     * Toggle chat state (event-based, thread-safe)
     * Sends MAIN_EVENT_TOGGLE_CHAT to be handled in Run()
     */
    void ToggleChatState();

    /**
     * Start listening (event-based, thread-safe)
     * Sends MAIN_EVENT_START_LISTENING to be handled in Run()
     */
    void StartListening();

    /**
     * Stop listening (event-based, thread-safe)
     * Sends MAIN_EVENT_STOP_LISTENING to be handled in Run()
     */
    void StopListening();

    void Reboot();
    void WakeWordInvoke(const std::string& wake_word);
    bool CanEnterSleepMode();
    void SendMcpMessage(const std::string& payload);
    void RegisterMcpBroadcastCallback(std::function<void(const std::string&)> callback);
    void SetAecMode(AecMode mode);
    AecMode GetAecMode() const { return aec_mode_; }
    AudioService& GetAudioService() { return audio_service_; }
    
    /**
     * Reset protocol resources (thread-safe)
     * Can be called from any task to release resources allocated after network connected
     * This includes closing audio channel, resetting protocol and ota objects
     */
    void ResetProtocol();
    void PlaySound(int index);

    EventGroupHandle_t event_group_ = nullptr;

private:
    Application();
    ~Application();

    std::mutex mutex_;
    std::deque<std::function<void()>> main_tasks_;
    std::unique_ptr<Protocol> protocol_;

    DeviceStateMachine state_machine_;
    ListeningMode listening_mode_ = kListeningModeAutoStop;
    AecMode aec_mode_ = kAecOff;
    std::string last_error_message_;
    AudioService audio_service_;
    mmap_assets_handle_t asset_audio;
    std::unique_ptr<Ota> ota_;

    std::function<void(const std::string&)> mcp_broadcast_callback_;

    bool has_server_time_ = false;
    bool aborted_ = false;
    bool assets_version_checked_ = false;
    bool play_popup_on_listening_ = false;  // Flag to play popup sound after state changes to listening
    int clock_ticks_ = 0;
    esp_timer_handle_t clock_timer_handle_ = nullptr;
    TaskHandle_t activation_task_handle_ = nullptr;

    // Event handlers
    void HandleStateChangedEvent();
    void HandleToggleChatEvent();
    void HandleStartListeningEvent();
    void HandleStopListeningEvent();
    void HandleNetworkConnectedEvent();
    void HandleNetworkDisconnectedEvent();
    void HandleActivationDoneEvent();
    void HandleWakeWordDetectedEvent();
    void ContinueOpenAudioChannel(ListeningMode mode);
    void ContinueWakeWordInvoke(const std::string& wake_word);

    // Activation task (runs in background)
    void ActivationTask();

    // Helper methods
    
    void Activate();
    void InitializeProtocol();
    void ShowActivationCode(const std::string& code, const std::string& message);
    void SetListeningMode(ListeningMode mode);
    ListeningMode GetDefaultListeningMode() const;
    
    // State change handler called by state machine
    void OnStateChanged(DeviceState old_state, DeviceState new_state);
};

#endif

#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    kNTStatus = 0,
    kNTMessage,
    kNTEmotion,
    kNTError
} NotificationType;

void xiaozhi_application_init();
void xiaozhi_application_run();
void SetMainEventBits(uint32_t bits);
void SetMainEventBitsFromISR(uint32_t bits);
void AddIntMcpTool(const char* name, const char* description, const char* property_name, 
    int min, int max, void (*mcp_callback)(int arg));
void SchedulePlaySound(int index);

extern void bat_monitor_info_update();
extern void display_notify(NotificationType type, const char* content);
extern void display_time_update(void);

#ifdef __cplusplus
}
#endif
#endif // _APPLICATION_H_
