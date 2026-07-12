#pragma once
#include <string>
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/web_server_base/web_server_base.h"

namespace esphome {
namespace hb_config {

struct TileCfg {
  std::string uid;
  std::string title;
  std::string type;
  bool enabled{false};
};

// Serves "/setup" (config page) + "/hbcfg" (GET current config JSON, POST to save)
// and persists the config JSON to flash. Exposes accessors for YAML lambdas.
class HbConfig : public Component, public AsyncWebHandler {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE; }

  bool canHandle(AsyncWebServerRequest *request) const override;
  void handleRequest(AsyncWebServerRequest *request) override;

  // ---- accessors for YAML lambdas (Phase 2c wiring) ----
  std::string room() { return this->room_; }
  std::string url() { return this->url_; }
  std::string user() { return this->user_; }
  std::string pass() { return this->pass_; }
  std::string tile_uid(int i) { return valid_(i) ? this->tiles_[i - 1].uid : std::string(); }
  std::string tile_title(int i) { return valid_(i) ? this->tiles_[i - 1].title : std::string(); }
  std::string tile_type(int i) { return valid_(i) ? this->tiles_[i - 1].type : std::string(); }
  bool tile_enabled(int i) { return valid_(i) && this->tiles_[i - 1].enabled; }
  bool configured() { return !this->url_.empty(); }
  // Bumped on every (re)parse so YAML can detect config changes and re-apply live.
  uint32_t generation() { return this->gen_; }

 protected:
  static constexpr size_t CFG_MAX = 2048;
  static bool valid_(int i) { return i >= 1 && i <= 6; }
  void save_(const std::string &json);
  void parse_();

  ESPPreferenceObject pref_;
  char save_buf_[CFG_MAX];          // off-stack (save runs on the small httpd/main task)
  std::string config_json_;
  std::string chunk_buf_;
  std::string pending_json_;
  bool pending_save_{false};
  std::string room_, url_, user_, pass_;
  TileCfg tiles_[6];
  uint32_t gen_{0};
};

}  // namespace hb_config
}  // namespace esphome
