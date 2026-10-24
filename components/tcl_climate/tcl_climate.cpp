//#ifdef USE_ARDUINO

#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "tcl_climate.h"
#include <cmath>
#include <map>

namespace esphome {
namespace tcl_climate {

// Use constexpr for compile-time constants
static constexpr uint8_t REQ_CMD[] = {0xBB, 0x00, 0x01, 0x04, 0x02, 0x01, 0x00, 0xBD};
static constexpr int MAX_LINE_LENGTH = 100;
static constexpr int UPDATE_INTERVAL_MS = 450;

void TCLClimate::set_current_temperature(float current_temperature) {
  if (std::abs(this->current_temperature - current_temperature) < 0.1f) return;

  // Log current temperature change
  ESP_LOGD("TCL", "Current temperature updated to: %.1f°C", current_temperature);

  this->is_changed = true;
  this->current_temperature = current_temperature;
}

void TCLClimate::set_custom_fan_mode(StringRef fan_mode) {
  StringRef current(this->get_custom_fan_mode());
  if (!current.empty() && fan_mode == current.c_str()) {
    return;
  }

  // Log the fan mode change
  ESP_LOGI("TCL", "Fan mode changed to: %s", fan_mode.c_str());
  this->is_changed = true;

  this->set_custom_fan_mode_(fan_mode.c_str());
}

void TCLClimate::set_mode(climate::ClimateMode mode) {
  if (this->mode == mode) return;
  // Log the mode change
  const char* mode_str = "";
  switch (mode) {
    case climate::CLIMATE_MODE_OFF: mode_str = "OFF"; break;
    case climate::CLIMATE_MODE_COOL: mode_str = "COOL"; break;
    case climate::CLIMATE_MODE_HEAT: mode_str = "HEAT"; break;
    case climate::CLIMATE_MODE_FAN_ONLY: mode_str = "FAN ONLY"; break;
    case climate::CLIMATE_MODE_DRY: mode_str = "DRY"; break;
    case climate::CLIMATE_MODE_AUTO: mode_str = "AUTO"; break;
    default: mode_str = "UNKNOWN"; break;
  }
  ESP_LOGI("TCL", "Climate mode changed to: %s", mode_str);
  this->is_changed = true;
  this->mode = mode;
}

void TCLClimate::set_swing_mode(climate::ClimateSwingMode swing_mode) {
  if (this->swing_mode == swing_mode) return;

  const char* swing_str = "";
  switch (swing_mode) {
    case climate::CLIMATE_SWING_OFF: swing_str = "OFF"; break;
    case climate::CLIMATE_SWING_BOTH: swing_str = "BOTH"; break;
    case climate::CLIMATE_SWING_VERTICAL: swing_str = "VERTICAL"; break;
    case climate::CLIMATE_SWING_HORIZONTAL: swing_str = "HORIZONTAL"; break;
    default: swing_str = "UNKNOWN"; break;
  }
  ESP_LOGI("TCL", "Swing mode changed to: %s", swing_str);
  this->is_changed = true;
  this->swing_mode = swing_mode;
}

void TCLClimate::set_hswing_pos(const std::string &hswing_pos) {
  if (this->hswing_pos == hswing_pos) return;
  ESP_LOGI("TCL", "Horizontal swing position: %s", hswing_pos.c_str());
  this->hswing_pos = hswing_pos;
}

void TCLClimate::set_vswing_pos(const std::string &vswing_pos) {
  if (this->vswing_pos == vswing_pos) return;

  ESP_LOGI("TCL", "Vertical swing position: %s", vswing_pos.c_str());
  this->vswing_pos = vswing_pos;
}

void TCLClimate::set_target_temperature(float target_temperature) {
  if (std::abs(this->target_temperature - target_temperature) < 0.1f) return;
  // Log temperature change
  ESP_LOGI("TCL", "Target temperature changed to: %.1f°C", target_temperature);

  this->is_changed = true;
  this->target_temperature = target_temperature;
}

void TCLClimate::build_set_cmd(get_cmd_resp_t *get_cmd_resp) {
    memcpy(m_set_cmd.raw, set_cmd_base, sizeof(m_set_cmd.raw));

    // Manual assignment instead of struct initialization
    m_set_cmd.data.power = get_cmd_resp->data.power;
    m_set_cmd.data.off_timer_en = 0;
    m_set_cmd.data.on_timer_en = 0;
    m_set_cmd.data.beep = this->beep_on_ ? 1 : 0;
    m_set_cmd.data.disp = this->display_on_ ? 1 : 0;
    m_set_cmd.data.eco = 0;
    m_set_cmd.data.turbo = get_cmd_resp->data.turbo;
    m_set_cmd.data.mute = get_cmd_resp->data.mute;

    // Mode mapping using lookup table
    static constexpr uint8_t MODE_MAP[] = {
        0x00, // 0x00 - unused
        0x03, // 0x01 -> 0x03
        0x02, // 0x02 -> 0x02 (fan only)
        0x07, // 0x03 -> 0x07 (dry)
        0x01, // 0x04 -> 0x01 (heat)
        0x08  // 0x05 -> 0x08 (auto)
    };

    if (get_cmd_resp->data.mode < sizeof(MODE_MAP)) {
        m_set_cmd.data.mode = MODE_MAP[get_cmd_resp->data.mode];
    }

    // Temperature conversion
    m_set_cmd.data.temp = 15 - get_cmd_resp->data.temp;

    // Fan mode mapping using lookup table
    static constexpr uint8_t FAN_MAP[] = {
        0x00, // 0x00 -> 0x00 (auto)
        0x02, // 0x01 -> 0x02 (speed 1)
        0x03, // 0x02 -> 0x03 (speed 3)
        0x05, // 0x03 -> 0x05 (speed 5)
        0x06, // 0x04 -> 0x06 (speed 2)
        0x07  // 0x05 -> 0x07 (speed 4)
    };

    if (get_cmd_resp->data.fan < sizeof(FAN_MAP)) {
        m_set_cmd.data.fan = FAN_MAP[get_cmd_resp->data.fan];
    }

    // Swing control - extracted from old code
    if (get_cmd_resp->data.vswing_mv) {
      m_set_cmd.data.vswing = 0x07;
      m_set_cmd.data.vswing_fix = 0;
      m_set_cmd.data.vswing_mv = get_cmd_resp->data.vswing_mv;
    } else if (get_cmd_resp->data.vswing_fix) {
      m_set_cmd.data.vswing = 0;
      m_set_cmd.data.vswing_fix = get_cmd_resp->data.vswing_fix;
      m_set_cmd.data.vswing_mv = 0;
    }

    if (get_cmd_resp->data.hswing_mv) {
      m_set_cmd.data.hswing = 0x01;
      m_set_cmd.data.hswing_fix = 0;
      m_set_cmd.data.hswing_mv = get_cmd_resp->data.hswing_mv;
    } else if (get_cmd_resp->data.hswing_fix) {
      m_set_cmd.data.hswing = 0;
      m_set_cmd.data.hswing_fix = get_cmd_resp->data.hswing_fix;
      m_set_cmd.data.hswing_mv = 0;
    }

    m_set_cmd.data.half_degree = 0;

    // Calculate XOR checksum
    uint8_t xor_byte = 0;
    for (size_t i = 0; i < sizeof(m_set_cmd.raw) - 1; i++) {
        xor_byte ^= m_set_cmd.raw[i];
    }
    m_set_cmd.raw[sizeof(m_set_cmd.raw) - 1] = xor_byte;
}

void TCLClimate::setup() {
  set_update_interval(UPDATE_INTERVAL_MS);
  this->set_supported_custom_fan_modes({"Turbo", "Mute", "Automatic", "1", "2", "3", "4", "5"});
  auto restored = this->restore_state_();
  if (restored.has_value()) {
    restored->apply(this);
    user_has_set_target_ = true;
  }
  // Guard against idle temperatures (30°C/17°C) saved to RTC during boot race
  if (this->target_temperature >= 30.0f) {
    this->target_temperature = 24.0f;
  } else if (this->target_temperature <= 17.0f && this->target_temperature > 0.0f) {
    this->target_temperature = 20.0f;
  }
}

void TCLClimate::set_beep_runtime(bool on) {
    beep_on_ = on;
    if (m_get_cmd_resp.data.header == 0xBB) {
        get_cmd_resp_t resp = {0};
        memcpy(resp.raw, m_get_cmd_resp.raw, sizeof(resp.raw));
        build_set_cmd(&resp);
        ready_to_send_set_cmd_flag = true;
    }
}

void TCLClimate::set_display_runtime(bool on) {
    display_on_ = on;
    if (m_get_cmd_resp.data.header == 0xBB) {
        get_cmd_resp_t resp = {0};
        memcpy(resp.raw, m_get_cmd_resp.raw, sizeof(resp.raw));
        build_set_cmd(&resp);
        ready_to_send_set_cmd_flag = true;
    }
}

// Swing control methods from old code
void TCLClimate::control_vertical_swing(const std::string &swing_mode) {

  get_cmd_resp_t get_cmd_resp = {0};
  memcpy(get_cmd_resp.raw, m_get_cmd_resp.raw, sizeof(get_cmd_resp.raw));

  get_cmd_resp.data.vswing_mv = 0;
  get_cmd_resp.data.vswing_fix = 0;

  if (swing_mode == "Move full") get_cmd_resp.data.vswing_mv = 0x01;
  else if (swing_mode == "Move upper")  get_cmd_resp.data.vswing_mv = 0x02;
  else if (swing_mode == "Move lower")  get_cmd_resp.data.vswing_mv = 0x03;
  else if (swing_mode == "Fix top") get_cmd_resp.data.vswing_fix = 0x01;
  else if (swing_mode == "Fix upper") get_cmd_resp.data.vswing_fix = 0x02;
  else if (swing_mode == "Fix mid") get_cmd_resp.data.vswing_fix = 0x03;
  else if (swing_mode == "Fix lower") get_cmd_resp.data.vswing_fix = 0x04;
  else if (swing_mode == "Fix bottom") get_cmd_resp.data.vswing_fix = 0x05;

  if (get_cmd_resp.data.vswing_mv) get_cmd_resp.data.vswing = 0x01;
  else get_cmd_resp.data.vswing = 0;

  build_set_cmd(&get_cmd_resp);
  ready_to_send_set_cmd_flag = true;
}

void TCLClimate::control_horizontal_swing(const std::string &swing_mode) {

  get_cmd_resp_t get_cmd_resp = {0};
  memcpy(get_cmd_resp.raw, m_get_cmd_resp.raw, sizeof(get_cmd_resp.raw));

  get_cmd_resp.data.hswing_mv = 0;
  get_cmd_resp.data.hswing_fix = 0;

  if (swing_mode == "Move full") get_cmd_resp.data.hswing_mv = 0x01;
  else if (swing_mode == "Move left") get_cmd_resp.data.hswing_mv = 0x02;
  else if (swing_mode == "Move mid") get_cmd_resp.data.hswing_mv = 0x03;
  else if (swing_mode == "Move right") get_cmd_resp.data.hswing_mv = 0x04;
  else if (swing_mode == "Fix left") get_cmd_resp.data.hswing_fix = 0x01;
  else if (swing_mode == "Fix mid left") get_cmd_resp.data.hswing_fix = 0x02;
  else if (swing_mode == "Fix mid") get_cmd_resp.data.hswing_fix = 0x03;
  else if (swing_mode == "Fix mid right") get_cmd_resp.data.hswing_fix = 0x04;
  else if (swing_mode == "Fix right") get_cmd_resp.data.hswing_fix = 0x05;

  if (get_cmd_resp.data.hswing_mv) get_cmd_resp.data.hswing = 0x01;
  else get_cmd_resp.data.hswing = 0;

  build_set_cmd(&get_cmd_resp);
  ready_to_send_set_cmd_flag = true;
}

void TCLClimate::control(const climate::ClimateCall &call) {
    get_cmd_resp_t get_cmd_resp = {0};
    memcpy(get_cmd_resp.raw, m_get_cmd_resp.raw, sizeof(get_cmd_resp.raw));
    bool should_build_cmd = false;

    if (call.get_mode().has_value()) {
        climate::ClimateMode climate_mode = *call.get_mode();
        ESP_LOGI("TCL", "Received mode control command: %d", static_cast<int>(climate_mode));

        if (climate_mode == climate::CLIMATE_MODE_OFF) {
            get_cmd_resp.data.power = 0x00;
            user_has_set_target_ = false;
            } else {
                get_cmd_resp.data.power = 0x01;
                switch (climate_mode) {
                    case climate::CLIMATE_MODE_COOL:    get_cmd_resp.data.mode = 0x01; break;
                    case climate::CLIMATE_MODE_DRY:     get_cmd_resp.data.mode = 0x03; break;
                    case climate::CLIMATE_MODE_FAN_ONLY:get_cmd_resp.data.mode = 0x02; break;
                    case climate::CLIMATE_MODE_HEAT:
                    case climate::CLIMATE_MODE_HEAT_COOL:get_cmd_resp.data.mode = 0x04; break;
                    case climate::CLIMATE_MODE_AUTO:    get_cmd_resp.data.mode = 0x05; break;
                    default: break;
                }
                // Re-enable regulation when turning back on
                user_has_set_target_ = true;
                if (this->target_temperature >= 16.0f && this->target_temperature <= 31.0f) {
                    get_cmd_resp.data.temp = static_cast<uint8_t>(this->target_temperature) - 16;
                } else {
                    this->set_target_temperature(24.0f);
                    get_cmd_resp.data.temp = 8; // 24°C
                }
            }
        should_build_cmd = true;
    }

    if (call.get_target_temperature().has_value()) {
        float temp = *call.get_target_temperature();
        ESP_LOGI("TCL", "Received temperature control command: %.1f°C", temp);

        user_has_set_target_ = true;
        this->set_target_temperature(temp);
        get_cmd_resp.data.temp = static_cast<uint8_t>(temp) - 16;

        // If the AC is off and no mode was given, turn it on and pick the
        // operating direction from the sign of the error:
        // room colder than target -> HEAT, room warmer or equal -> COOL.
        if (!call.get_mode().has_value() &&
            m_get_cmd_resp.data.header == 0xBB && m_get_cmd_resp.data.power == 0x00) {
            float ref = isnan(this->current_temperature) ? temp : this->current_temperature;
            if (temp > ref) {
                get_cmd_resp.data.power = 0x01;
                get_cmd_resp.data.mode = 0x04;
                this->set_mode(climate::CLIMATE_MODE_HEAT);
                ESP_LOGI("TCL", "AC is off — turning on in HEAT mode (target %.1f > current %.1f)", temp, ref);
            } else {
                get_cmd_resp.data.power = 0x01;
                get_cmd_resp.data.mode = 0x01;
                this->set_mode(climate::CLIMATE_MODE_COOL);
                ESP_LOGI("TCL", "AC is off — turning on in COOL mode (target %.1f <= current %.1f)", temp, ref);
            }
        }
        should_build_cmd = true;
    }

    if (call.get_swing_mode().has_value()) {
        climate::ClimateSwingMode swing_mode = *call.get_swing_mode();

        switch(swing_mode) {
            case climate::CLIMATE_SWING_OFF:
                get_cmd_resp.data.hswing = 0;
                get_cmd_resp.data.vswing = 0;
                break;
            case climate::CLIMATE_SWING_BOTH:
                get_cmd_resp.data.hswing = 1;
                get_cmd_resp.data.vswing = 1;
                break;
            case climate::CLIMATE_SWING_VERTICAL:
                get_cmd_resp.data.hswing = 0;
                get_cmd_resp.data.vswing = 1;
                break;
            case climate::CLIMATE_SWING_HORIZONTAL:
                get_cmd_resp.data.hswing = 1;
                get_cmd_resp.data.vswing = 0;
                break;
        }
        should_build_cmd = true;
    }

    // Updated custom fan mode handling
    StringRef custom_fan_mode(call.get_custom_fan_mode());
    if (!custom_fan_mode.empty()) {
        std::string fan_mode(custom_fan_mode.c_str());
        ESP_LOGI("TCL", "Received fan mode control command: %s", fan_mode.c_str());

        get_cmd_resp.data.turbo = 0x00;
        get_cmd_resp.data.mute = 0x00;

        // Use map for fan mode parsing
        static const std::map<std::string, std::pair<uint8_t, uint8_t>> FAN_MODE_MAP = {
            {"Turbo",      {0x03, 0x01}},
            {"Mute",       {0x01, 0x01}},
            {"Automatic",  {0x00, 0x00}},
            {"1",          {0x01, 0x00}},
            {"2",          {0x04, 0x00}},
            {"3",          {0x02, 0x00}},
            {"4",          {0x05, 0x00}},
            {"5",          {0x03, 0x00}}
        };

        auto it = FAN_MODE_MAP.find(fan_mode);
        if (it != FAN_MODE_MAP.end()) {
            get_cmd_resp.data.fan = it->second.first;
            if (fan_mode == "Turbo") get_cmd_resp.data.turbo = 0x01;
            else if (fan_mode == "Mute") get_cmd_resp.data.mute = 0x01;
        }
        should_build_cmd = true;
    }

    if (should_build_cmd) {
        ESP_LOGI("TCL", "Building and sending command to AC unit");

        build_set_cmd(&get_cmd_resp);
        ready_to_send_set_cmd_flag = true;
        this->publish_state();
    }
}

climate::ClimateTraits TCLClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
  traits.set_supported_modes({
    climate::CLIMATE_MODE_OFF,
    climate::CLIMATE_MODE_COOL,
    climate::CLIMATE_MODE_HEAT,
    climate::CLIMATE_MODE_FAN_ONLY,
    climate::CLIMATE_MODE_DRY,
    climate::CLIMATE_MODE_AUTO
  });
  traits.set_supported_swing_modes({
    climate::CLIMATE_SWING_OFF,
    climate::CLIMATE_SWING_BOTH,
    climate::CLIMATE_SWING_VERTICAL,
    climate::CLIMATE_SWING_HORIZONTAL
  });
  traits.set_visual_min_temperature(16.0);
  traits.set_visual_max_temperature(31.0);
  traits.set_visual_target_temperature_step(1.0);
  return traits;
}

void TCLClimate::update() {
    if (ready_to_send_set_cmd_flag) {
        ready_to_send_set_cmd_flag = false;
        write_array(m_set_cmd.raw, sizeof(m_set_cmd.raw));
    } else {
        write_array(REQ_CMD, sizeof(REQ_CMD));
    }
}

int TCLClimate::read_data_line(int readch, uint8_t *buffer, int len) {
    static int pos = 0;
    static bool wait_len = false;
    static int skipch = 0;

    if (readch < 0) return -1;

    if (readch == 0xBB && skipch == 0 && !wait_len) {
        pos = 0;
        skipch = 3; // wait for length byte
        wait_len = true;
        if (pos < len) buffer[pos++] = static_cast<uint8_t>(readch);
    } else if (skipch == 0 && wait_len) {
        if (pos < len) buffer[pos++] = static_cast<uint8_t>(readch);
        skipch = readch + 1; // +1 for checksum
        wait_len = false;
    } else if (skipch > 0) {
        if (pos < len) buffer[pos++] = static_cast<uint8_t>(readch);
        if (--skipch == 0 && !wait_len) return pos;
    }

    return -1;
}

bool TCLClimate::is_valid_xor(uint8_t *buffer, int len) {
    if (len < 1) return false;

    uint8_t xor_byte = 0;
    for (int i = 0; i < len - 1; i++) {
        xor_byte ^= buffer[i];
    }
    return xor_byte == buffer[len - 1];
}

void TCLClimate::print_hex_str(uint8_t *buffer, int len) {
    if (len <= 0) return;

    char str[MAX_LINE_LENGTH * 3] = {0};
    char *pstr = str;

    for (int i = 0; i < len && (pstr - str) < sizeof(str) - 3; i++) {
        pstr += sprintf(pstr, "%02X ", buffer[i]);
    }

    ESP_LOGD("TCL", "Received: %s", str);
}

void TCLClimate::loop() {
    static uint8_t buffer[MAX_LINE_LENGTH];

    while (available()) {
        int len = read_data_line(read(), buffer, MAX_LINE_LENGTH);
        if (len == sizeof(m_get_cmd_resp) && buffer[3] == 0x04) {
            memcpy(m_get_cmd_resp.raw, buffer, len);

            if (is_valid_xor(buffer, len)) {
                print_hex_str(buffer, len);

                // Calculate current temperature with exponential smoothing
                float raw_temp = ((static_cast<uint16_t>(buffer[17] << 8) | buffer[18]) / 374.0f - 32.0f) / 1.8f;
                if (isnan(curr_temp_)) {
                    curr_temp_ = raw_temp;
                } else {
                    curr_temp_ = curr_temp_ * 0.85f + raw_temp * 0.15f;
                }
                this->is_changed = false;
                last_sent_temp_ = static_cast<float>(m_get_cmd_resp.data.temp + 16);

                // Set mode from AC response
                if (m_get_cmd_resp.data.power == 0x00 &&
                    (m_get_cmd_resp.data.temp == 14 || m_get_cmd_resp.data.temp == 1)) {
                    // AC is in our idle state (power=0, field=14/1) — set mode from AC mode
                    static const std::map<uint8_t, climate::ClimateMode> IDLE_MODE_MAP = {
                        {0x01, climate::CLIMATE_MODE_COOL},
                        {0x04, climate::CLIMATE_MODE_HEAT},
                    };
                    auto it = IDLE_MODE_MAP.find(m_get_cmd_resp.data.mode);
                    if (it != IDLE_MODE_MAP.end()) {
                        this->set_mode(it->second);
                    }
                } else if (m_get_cmd_resp.data.power == 0x00 && !user_has_set_target_) {
                    // AC reports power off with a non-idle field and user hasn't set target — truly off
                    this->set_mode(climate::CLIMATE_MODE_OFF);
                } else if (user_has_set_target_) {
                    // AC is on and user has an active target — sync mode from AC
                    static const std::map<uint8_t, climate::ClimateMode> MODE_MAP = {
                        {0x01, climate::CLIMATE_MODE_COOL},
                        {0x03, climate::CLIMATE_MODE_DRY},
                        {0x02, climate::CLIMATE_MODE_FAN_ONLY},
                        {0x04, climate::CLIMATE_MODE_HEAT},
                        {0x05, climate::CLIMATE_MODE_AUTO}
                    };
                    auto it = MODE_MAP.find(m_get_cmd_resp.data.mode);
                    if (it != MODE_MAP.end()) {
                        this->set_mode(it->second);
                    }
                }
                // else: user_has_set_target_=false and power=1 — user turned OFF but AC hasn't shut down yet.
                // Don't touch mode; leave it as OFF.

                // Set fan mode
                static const std::map<uint8_t, std::string> FAN_MODE_MAP = {
                    {0x00, "Automatic"},
                    {0x01, "1"},
                    {0x04, "2"},
                    {0x02, "3"},
                    {0x05, "4"},
                    {0x03, "5"}
                };

                StringRef current_fan(StringRef(this->get_custom_fan_mode()));

                if (m_get_cmd_resp.data.turbo) {
                  // String literal to StringRef - use explicit construction
                  this->set_custom_fan_mode(StringRef("Turbo"));
                } else if (m_get_cmd_resp.data.mute) {
                  this->set_custom_fan_mode(StringRef("Mute"));
                } else {
                  auto it = FAN_MODE_MAP.find(m_get_cmd_resp.data.fan);
                  if (it != FAN_MODE_MAP.end()) {
                    StringRef current_fan(StringRef(this->get_custom_fan_mode()));
                    if (current_fan.empty() || current_fan != it->second) {
                      // Convert std::string to StringRef
                      this->set_custom_fan_mode(StringRef(it->second.c_str(), it->second.size()));
                    }
                  }
                }

                // Set swing mode - extracted from old code
                if (m_get_cmd_resp.data.hswing && m_get_cmd_resp.data.vswing) {
                    this->set_swing_mode(climate::CLIMATE_SWING_BOTH);
                } else if (!m_get_cmd_resp.data.hswing && !m_get_cmd_resp.data.vswing) {
                    this->set_swing_mode(climate::CLIMATE_SWING_OFF);
                } else if (m_get_cmd_resp.data.vswing) {
                    this->set_swing_mode(climate::CLIMATE_SWING_VERTICAL);
                } else if (m_get_cmd_resp.data.hswing) {
                    this->set_swing_mode(climate::CLIMATE_SWING_HORIZONTAL);
                }

                // Set swing positions - extracted from old code
                if (m_get_cmd_resp.data.vswing_mv == 0x01) set_vswing_pos("Move full");
                else if (m_get_cmd_resp.data.vswing_mv == 0x02) set_vswing_pos("Move upper");
                else if (m_get_cmd_resp.data.vswing_mv == 0x03) set_vswing_pos("Move lower");
                else if (m_get_cmd_resp.data.vswing_fix == 0x01) set_vswing_pos("Fix top");
                else if (m_get_cmd_resp.data.vswing_fix == 0x02) set_vswing_pos("Fix upper");
                else if (m_get_cmd_resp.data.vswing_fix == 0x03) set_vswing_pos("Fix mid");
                else if (m_get_cmd_resp.data.vswing_fix == 0x04) set_vswing_pos("Fix lower");
                else if (m_get_cmd_resp.data.vswing_fix == 0x05) set_vswing_pos("Fix bottom");
                else set_vswing_pos("Last position");

                if (m_get_cmd_resp.data.hswing_mv == 0x01) set_hswing_pos("Move full");
                else if (m_get_cmd_resp.data.hswing_mv == 0x02) set_hswing_pos("Move left");
                else if (m_get_cmd_resp.data.hswing_mv == 0x03) set_hswing_pos("Move mid");
                else if (m_get_cmd_resp.data.hswing_mv == 0x04) set_hswing_pos("Move right");
                else if (m_get_cmd_resp.data.hswing_fix == 0x01) set_hswing_pos("Fix left");
                else if (m_get_cmd_resp.data.hswing_fix == 0x02) set_hswing_pos("Fix mid left");
                else if (m_get_cmd_resp.data.hswing_fix == 0x03) set_hswing_pos("Fix mid");
                else if (m_get_cmd_resp.data.hswing_fix == 0x04) set_hswing_pos("Fix mid right");
                else if (m_get_cmd_resp.data.hswing_fix == 0x05) set_hswing_pos("Fix right");
                else set_hswing_pos("Last position");

                // Keep display off if user disabled it — AC sometimes re-enables it
                if (!display_on_ && m_get_cmd_resp.data.disp) {
                    get_cmd_resp_t resp = {0};
                    memcpy(resp.raw, m_get_cmd_resp.raw, sizeof(resp.raw));
                    build_set_cmd(&resp);
                    ready_to_send_set_cmd_flag = true;
                }

                if (int_temp_sensor_ != nullptr) {
                    int_temp_sensor_->publish_state(curr_temp_);
                }

                if (ext_temp_sensor_ != nullptr && ext_temp_sensor_->has_state()) {
                    float ext_temp = ext_temp_sensor_->get_state();
                    this->set_current_temperature(ext_temp);

                    if (ext_temp_output_ != nullptr) {
                        ext_temp_output_->publish_state(ext_temp);
                    }

                    if (ramp_mode_) {
                        uint32_t now = millis();
                        uint32_t step_duration = 8UL * 60 * 1000;
                        uint8_t step = (now - ramp_step_start_ms_) / step_duration;

                        if (step >= 13) {
                            // Ramp complete — go to cool idle and stop
                            m_get_cmd_resp.data.power = 0x00;
                            m_get_cmd_resp.data.mode = 0x01;
                            m_get_cmd_resp.data.temp = 14;
                            build_set_cmd(&m_get_cmd_resp);
                            ready_to_send_set_cmd_flag = true;
                            ramp_mode_ = false;
                            user_has_set_target_ = false;
                            ESP_LOGI("TCL", "Ramp complete, AC set to idle");
                            // Still log one final row
                            float power = 0;
                            if (power_sensor_ != nullptr && power_sensor_->has_state()) power = power_sensor_->get_state();
                            char hex_buf[128] = {0};
                            char *p = hex_buf;
                            for (int i = 0; i < (int)sizeof(m_get_cmd_resp.raw) && (p - hex_buf) < (int)sizeof(hex_buf) - 3; i++)
                                p += snprintf(p, sizeof(hex_buf) - (p - hex_buf), "%02X", m_get_cmd_resp.raw[i]);
                            ESP_LOGI("TCL", "RAMP,%lu,%u,%.1f,%.1f,%.1f,%.0f,%u,%s",
                                now / 1000, ramp_step_, ramp_target_, ext_temp, curr_temp_, power, 14, hex_buf);
                        } else {
                            if (step > 12) step = 12;
                            ramp_step_ = step;
                            ramp_target_ = 28.0f - (float)step;

                        user_has_set_target_ = true;
                        this->target_temperature = ramp_target_;

                        uint8_t field = static_cast<uint8_t>(ramp_target_) - 16;
                        if (field > 14) field = 14;

                        float power = 0;
                        if (power_sensor_ != nullptr && power_sensor_->has_state()) {
                            power = power_sensor_->get_state();
                        }

                        if (now - ramp_last_log_ms_ >= 5000) {
                            ramp_last_log_ms_ = now;
                            char hex_buf[128] = {0};
                            char *p = hex_buf;
                            for (int i = 0; i < (int)sizeof(m_get_cmd_resp.raw) && (p - hex_buf) < (int)sizeof(hex_buf) - 3; i++) {
                                p += snprintf(p, sizeof(hex_buf) - (p - hex_buf), "%02X", m_get_cmd_resp.raw[i]);
                            }
                            ESP_LOGI("TCL", "RAMP,%lu,%u,%.1f,%.1f,%.1f,%.0f,%u,%s",
                                now / 1000, ramp_step_, ramp_target_, ext_temp, curr_temp_, power, field, hex_buf);
                        }

                        if (user_has_set_target_ && (m_get_cmd_resp.data.temp != field || m_get_cmd_resp.data.power == 0)) {
                            m_get_cmd_resp.data.power = 0x01;
                            m_get_cmd_resp.data.mode = 0x01;
                            m_get_cmd_resp.data.temp = field;
                            build_set_cmd(&m_get_cmd_resp);
                            ready_to_send_set_cmd_flag = true;
                        }

                        last_sent_temp_ = ramp_target_;
                        }   // closes else of step>=13
                    } else {
                        // No auto-fallback here; user must explicitly set target via HA
                        // RTC restore in setup() handles boot persistence

                        if (user_has_set_target_) {
                            float target = this->target_temperature;
                            float error = ext_temp - target;

                            // Decide the regulation direction:
                            //  - explicit COOL/HEAT mode set by the user wins
                            //  - otherwise (AUTO, OFF with an active target, or a
                            //    stale AC mode like summer's cool idle) pick
                            //    HEAT vs COOL from the sign of the error
                            uint8_t desired_mode;
                            if (this->mode == climate::CLIMATE_MODE_COOL) {
                                desired_mode = 0x01;
                            } else if (this->mode == climate::CLIMATE_MODE_HEAT) {
                                desired_mode = 0x04;
                            } else {
                                desired_mode = (error < 0 ? 0x04 : 0x01);
                            }

                            // Modulating control: scale the setpoint offset with
                            // the remaining error so the compressor backs off as
                            // the room approaches the target.
                            // e_hold > 0 -> still below target (heat) / above
                            // target (cool); e_hold < 0 -> overshoot.
                            float e_hold = (desired_mode == 0x04) ? -error : error;
                            float signed_gap;
                            if (e_hold >= 1.0f) {
                                // Far from target: full aggression (same P-gain as before)
                                signed_gap = e_hold * 5.0f + 0.5f;
                            } else if (e_hold > 0.0f) {
                                // Taper zone (<1° to target): shrink the offset so the
                                // AC modulates down instead of holding a fixed demand
                                signed_gap = (e_hold * 5.0f + 0.5f) * e_hold;
                            } else {
                                // Overshoot: push the setpoint below the indoor reading
                                // so the compressor drops to minimum / shuts off
                                signed_gap = e_hold * 5.0f - 0.5f;
                            }
                            if (signed_gap < -4.0f) signed_gap = -4.0f;
                            if (signed_gap > 8.0f) signed_gap = 8.0f;
                            // NOTE for heat: buffer[17]/[18] is the COIL temperature
                            // (hot), not room temp. Anchoring the setpoint to the
                            // room sensor (ext_temp) lets the AC's own thermostat
                            // see a setpoint near/around room temp, so it modulates
                            // and idles naturally. Cooling keeps the proven logic.
                            float sent_temp;
                            if (desired_mode == 0x04) {
                                sent_temp = ext_temp + signed_gap;
                            } else {
                                sent_temp = curr_temp_ - signed_gap;
                            }
                            if (isnan(sent_temp)) sent_temp = 24.0f;
                            if (sent_temp < 16.0f) sent_temp = 16.0f;
                            if (sent_temp > 30.0f) sent_temp = 30.0f;
                            last_sent_temp_ = sent_temp;

                            uint8_t desired_field = static_cast<uint8_t>(floorf(sent_temp + 0.499f)) - 16;
                            if (desired_field > 14) desired_field = 14;

                            if (m_get_cmd_resp.data.power == 0x00 ||
                                m_get_cmd_resp.data.mode != desired_mode ||
                                m_get_cmd_resp.data.temp != desired_field) {
                                m_get_cmd_resp.data.power = 0x01;
                                m_get_cmd_resp.data.mode = desired_mode;
                                m_get_cmd_resp.data.temp = desired_field;
                                build_set_cmd(&m_get_cmd_resp);
                                ready_to_send_set_cmd_flag = true;
                                ESP_LOGD("TCL", "%s: ext=%.1f tgt=%.1f int=%.1f gap=%.1f sent=%.0f",
                                         desired_mode == 0x04 ? "Heat" : "Cool",
                                         ext_temp, target, curr_temp_, signed_gap, sent_temp);
                            }
                        }
                    }
                } else {
                    this->set_current_temperature(curr_temp_);
                }

                if (sent_temp_output_ != nullptr) {
                    sent_temp_output_->publish_state(last_sent_temp_);
                }

                // One-line periodic status: everything correlated in one row.
                static uint32_t last_stat_ms = 0;
                if (millis() - last_stat_ms >= 10000) {
                    last_stat_ms = millis();
                    const char* mode_name = "?";
                    switch (this->mode) {
                        case climate::CLIMATE_MODE_OFF: mode_name = "OFF"; break;
                        case climate::CLIMATE_MODE_COOL: mode_name = "COOL"; break;
                        case climate::CLIMATE_MODE_HEAT: mode_name = "HEAT"; break;
                        case climate::CLIMATE_MODE_AUTO: mode_name = "AUTO"; break;
                        case climate::CLIMATE_MODE_FAN_ONLY: mode_name = "FAN"; break;
                        case climate::CLIMATE_MODE_DRY: mode_name = "DRY"; break;
                        default: mode_name = "?"; break;
                    }
                    ESP_LOGI("TCL", "STAT,fw3,ext=%.1f,tgt=%.1f,u_mode=%s,ac_pwr=%u,ac_mode=%u,ac_set=%u,cmd=%.1f,powerW=%.0f",
                             static_cast<double>(this->current_temperature), this->target_temperature, mode_name,
                             m_get_cmd_resp.data.power, m_get_cmd_resp.data.mode, m_get_cmd_resp.data.temp + 16,
                             static_cast<double>(last_sent_temp_),
                             static_cast<double>(power_sensor_ && power_sensor_->has_state() ? power_sensor_->get_state() : NAN));
                }

                if (power_sensor_ != nullptr && power_sensor_->has_state()) {
                    if (power_output_ != nullptr) {
                        power_output_->publish_state(power_sensor_->get_state());
                    }
                }

                if (this->is_changed) {
                    this->publish_state();
                }
            }
        }
    }
}

void TCLClimate::set_ramp_mode(bool ramp_on) {
    ramp_mode_ = ramp_on;
    if (ramp_on) {
        ramp_step_start_ms_ = millis();
        ramp_last_log_ms_ = millis();
        ramp_step_ = 0;
        ramp_target_ = 28.0f;
    }
}

}  // namespace tcl_climate
}  // namespace esphome
//#endif  // USE_ARDUINO
