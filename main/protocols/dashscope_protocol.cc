#include "dashscope_protocol.h"
#include "board.h"
#include "system_info.h"
#include "settings.h"

#include <esp_log.h>
#include <cJSON.h>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include "assets/lang_config.h"

const char* const DashScopeProtocol::TAG = "DashScope";

DashScopeProtocol::DashScopeProtocol()
    : event_group_(xEventGroupCreate()),
      audio_channel_opened_(false) {
}

DashScopeProtocol::~DashScopeProtocol() {
    if (event_group_ != nullptr) {
        vEventGroupDelete(event_group_);
    }
}

bool DashScopeProtocol::Start() {
    // Only connect when audio channel is needed
    return true;
}

bool DashScopeProtocol::InitializeHttpClient() {
    auto& board = Board::GetInstance();
    auto network = board.GetNetwork();

    http_client_ = network->CreateHttp(0);
    if (!http_client_) {
        ESP_LOGE(TAG, "Failed to create HTTP client");
        return false;
    }

    // Set headers
    http_client_->SetHeader("Content-Type", "application/json");
    http_client_->SetHeader("Authorization", ("Bearer " + api_key_).c_str());
    http_client_->SetHeader("X-DashScope-SSE", "enable"); // Enable streaming
    http_client_->SetHeader("Accept", "text/event-stream");

    return true;
}

bool DashScopeProtocol::OpenAudioChannel() {
    ESP_LOGI(TAG, "Opening DashScope audio channel");

    // Get configuration from Kconfig or settings
    ESP_LOGI(TAG, "Reading DashScope configuration...");

#if defined(CONFIG_DASHSCOPE_API_KEY)
    api_key_ = CONFIG_DASHSCOPE_API_KEY;
    ESP_LOGI(TAG, "API Key from Kconfig: %s", api_key_.empty() ? "empty" : "set");
#endif
#if defined(CONFIG_DASHSCOPE_APP_ID)
    app_id_ = CONFIG_DASHSCOPE_APP_ID;
    ESP_LOGI(TAG, "App ID from Kconfig: %s", app_id_.empty() ? "empty" : "set");
#endif
#if defined(CONFIG_DASHSCOPE_BASE_URL)
    base_url_ = CONFIG_DASHSCOPE_BASE_URL;
    ESP_LOGI(TAG, "Base URL from Kconfig: %s", base_url_.c_str());
#else
    base_url_ = "https://dashscope.aliyuncs.com/api/v1/";
    ESP_LOGI(TAG, "Using default base URL: %s", base_url_.c_str());
#endif

    // If not set in Kconfig, try to get from settings
    if (api_key_.empty() || app_id_.empty()) {
        ESP_LOGI(TAG, "Trying to get configuration from settings...");
        Settings settings("dashscope", false);
        if (api_key_.empty()) {
            api_key_ = settings.GetString("api_key");
            ESP_LOGI(TAG, "API Key from settings: %s", api_key_.empty() ? "empty" : "set");
        }
        if (app_id_.empty()) {
            app_id_ = settings.GetString("app_id");
            ESP_LOGI(TAG, "App ID from settings: %s", app_id_.empty() ? "empty" : "set");
        }
        if (base_url_ == "https://dashscope.aliyuncs.com/api/v1/") {
            base_url_ = settings.GetString("base_url", "https://dashscope.aliyuncs.com/api/v1/");
            ESP_LOGI(TAG, "Base URL from settings: %s", base_url_.c_str());
        }
    }

    if (api_key_.empty() || app_id_.empty()) {
        ESP_LOGE(TAG, "DashScope API key or App ID not configured");
        SetError(Lang::Strings::SERVER_ERROR);
        return false;
    }

    if (!InitializeHttpClient()) {
        SetError(Lang::Strings::SERVER_ERROR);
        return false;
    }

    audio_channel_opened_ = true;
    audio_channel_open_time_ = std::chrono::steady_clock::now();

    if (on_audio_channel_opened_) {
        on_audio_channel_opened_();
    }

    ESP_LOGI(TAG, "DashScope audio channel opened successfully");
    return true;
}

void DashScopeProtocol::CloseAudioChannel() {
    ESP_LOGI(TAG, "Closing DashScope audio channel");
    audio_channel_opened_ = false;
    http_client_.reset();
    conversation_history_.clear();

    if (on_audio_channel_closed_) {
        on_audio_channel_closed_();
    }
}

bool DashScopeProtocol::IsAudioChannelOpened() const {
    return audio_channel_opened_ && !error_occurred_ && !IsAudioChannelTimeout();
}

bool DashScopeProtocol::IsAudioChannelTimeout() const {
    if (!audio_channel_opened_) {
        return false;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - audio_channel_open_time_).count();

    return elapsed > AUDIO_CHANNEL_TIMEOUT_MS;
}

bool DashScopeProtocol::SendAudio(std::unique_ptr<AudioStreamPacket> packet) {
    if (!IsAudioChannelOpened()) {
        return false;
    }

    // For now, we'll treat audio as text input
    // In a full implementation, you might want to implement speech-to-text
    // For demonstration, we'll send a placeholder text
    std::string text_input = "[语音输入]";

    return SendConversationRequest(text_input);
}

bool DashScopeProtocol::SendText(const std::string& text) {
    if (!IsAudioChannelOpened()) {
        return false;
    }

    return SendConversationRequest(text);
}

bool DashScopeProtocol::SendConversationRequest(const std::string& prompt) {
    if (!http_client_) {
        ESP_LOGE(TAG, "HTTP client not initialized");
        SetError(Lang::Strings::SERVER_ERROR);
        return false;
    }

    std::string request_json = BuildRequestJson(prompt);
    std::string url = base_url_ + "applications/" + app_id_ + "/conversation";

    http_client_->SetContent(std::move(request_json));

    ESP_LOGI(TAG, "Sending conversation request to: %s", url.c_str());
    ESP_LOGD(TAG, "Request JSON: %s", request_json.c_str());

    if (!http_client_->Open("POST", url)) {
        ESP_LOGE(TAG, "Failed to open HTTP connection");
        SetError(Lang::Strings::SERVER_ERROR);
        return false;
    }

    auto status_code = http_client_->GetStatusCode();
    if (status_code != 200) {
        ESP_LOGE(TAG, "HTTP request failed with status: %d", status_code);
        SetError(Lang::Strings::SERVER_ERROR);
        return false;
    }

    // Read streaming response
    std::string response_data = http_client_->ReadAll();
    http_client_->Close();

    if (response_data.empty()) {
        ESP_LOGW(TAG, "No data received from server");
        SetError(Lang::Strings::SERVER_ERROR);
        return false;
    }

    ESP_LOGD(TAG, "Received response: %s", response_data.c_str());

    // Process complete lines (SSE format)
    std::istringstream iss(response_data);
    std::string line;

    while (std::getline(iss, line)) {
        // Remove trailing \r if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        ProcessStreamingResponse(line);
    }

    return true;
}

std::string DashScopeProtocol::BuildRequestJson(const std::string& prompt) const {
    cJSON* root = cJSON_CreateObject();

    // Add messages array
    cJSON* messages = cJSON_AddArrayToObject(root, "messages");

    // Add conversation history
    for (const auto& msg : conversation_history_) {
        cJSON* message = cJSON_CreateObject();
        cJSON_AddStringToObject(message, "role", msg.role.c_str());
        cJSON_AddStringToObject(message, "content", msg.content.c_str());
        cJSON_AddItemToArray(messages, message);
    }

    // Add current prompt
    cJSON* current_message = cJSON_CreateObject();
    cJSON_AddStringToObject(current_message, "role", "user");
    cJSON_AddStringToObject(current_message, "content", prompt.c_str());
    cJSON_AddItemToArray(messages, current_message);

    // Add session_id if exists
    if (!session_id_.empty()) {
        cJSON_AddStringToObject(root, "session_id", session_id_.c_str());
    }

    // Enable streaming
    cJSON_AddBoolToObject(root, "stream", true);
    cJSON_AddBoolToObject(root, "incremental_output", true);

    char* json_str = cJSON_PrintUnformatted(root);
    std::string result(json_str);
    cJSON_free(json_str);
    cJSON_Delete(root);

    return result;
}

void DashScopeProtocol::ProcessStreamingResponse(const std::string& line) {
    ESP_LOGD(TAG, "Processing line: %s", line.c_str());

    if (line.empty()) return;

    if (line.find("data: ") == 0) {
        std::string json_data = line.substr(6); // Remove "data: " prefix

        if (json_data.empty() || json_data == "[DONE]") {
            return; // Skip empty data or completion marker
        }

        cJSON* root = cJSON_Parse(json_data.c_str());
        if (!root) {
            ESP_LOGE(TAG, "Failed to parse JSON: %s", json_data.c_str());
            return;
        }

        // Check for errors
        cJSON* status_code = cJSON_GetObjectItem(root, "status_code");
        if (cJSON_IsNumber(status_code) && status_code->valueint != 200) {
            cJSON* message = cJSON_GetObjectItem(root, "message");
            const char* error_msg = message && cJSON_IsString(message) ?
                                   message->valuestring : "Unknown error";
            ESP_LOGE(TAG, "DashScope API error (%d): %s", status_code->valueint, error_msg);
            SetError(Lang::Strings::SERVER_ERROR);
            cJSON_Delete(root);
            return;
        }

        // Process output
        cJSON* output = cJSON_GetObjectItem(root, "output");
        if (cJSON_IsObject(output)) {
            // Update session_id
            cJSON* session_id = cJSON_GetObjectItem(output, "session_id");
            if (cJSON_IsString(session_id)) {
                session_id_ = session_id->valuestring;
                ESP_LOGD(TAG, "Updated session_id: %s", session_id_.c_str());
            }

            // Process text content
            cJSON* text = cJSON_GetObjectItem(output, "text");
            if (cJSON_IsString(text) && strlen(text->valuestring) > 0) {
                ESP_LOGI(TAG, "Received text: %s", text->valuestring);

                if (on_incoming_json_) {
                    // Create a JSON response for the application layer
                    cJSON* response_json = cJSON_CreateObject();
                    cJSON_AddStringToObject(response_json, "type", "speech");
                    cJSON_AddStringToObject(response_json, "text", text->valuestring);
                    cJSON_AddStringToObject(response_json, "session_id", session_id_.c_str());

                    on_incoming_json_(response_json);
                    cJSON_Delete(response_json);
                }

                // Add to conversation history
                AddMessageToHistory("assistant", text->valuestring);
            }

            // Check if this is the final response
            cJSON* finish_reason = cJSON_GetObjectItem(output, "finish_reason");
            if (cJSON_IsString(finish_reason) && strcmp(finish_reason->valuestring, "stop") == 0) {
                ESP_LOGI(TAG, "Conversation finished");
            }
        }

        cJSON_Delete(root);
    } else if (line.find("event: ") == 0) {
        // Handle SSE event types if needed
        ESP_LOGD(TAG, "SSE event: %s", line.c_str());
    }
}

void DashScopeProtocol::AddMessageToHistory(const std::string& role, const std::string& content) {
    conversation_history_.push_back({role, content});

    // Limit conversation history to prevent memory issues
    const size_t MAX_HISTORY_SIZE = 50;
    if (conversation_history_.size() > MAX_HISTORY_SIZE) {
        conversation_history_.erase(conversation_history_.begin());
    }
}

void DashScopeProtocol::SendWakeWordDetected(const std::string& wake_word) {
    Protocol::SendWakeWordDetected(wake_word);
    // Reset conversation history when wake word is detected
    conversation_history_.clear();
    session_id_.clear();
}

void DashScopeProtocol::SendStartListening(ListeningMode mode) {
    Protocol::SendStartListening(mode);
    // Could be used to prepare for audio input
}

void DashScopeProtocol::SendStopListening() {
    Protocol::SendStopListening();
    // Could be used to finalize audio processing
}

void DashScopeProtocol::SendAbortSpeaking(AbortReason reason) {
    Protocol::SendAbortSpeaking(reason);
    // Cancel any ongoing requests if needed
}

void DashScopeProtocol::SendMcpMessage(const std::string& message) {
    Protocol::SendMcpMessage(message);
    // Handle MCP messages if needed
}