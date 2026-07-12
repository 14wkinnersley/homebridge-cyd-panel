#include "hb_config.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/components/json/json_util.h"
#include "esp_heap_caps.h"

namespace esphome {
namespace hb_config {

static const char *const TAG = "hb_config";
static const char *const DEFAULT_CFG =
    "{\"room\":\"Office\",\"url\":\"\",\"user\":\"\",\"pass\":\"\",\"tiles\":[]}";

// ---- Setup page (served at /setup) --------------------------------------
static const char SETUP_HTML[] = R"HTML(<!doctype html><html><head>
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Panel Setup</title>
<style>
body{font-family:system-ui,-apple-system,sans-serif;margin:0;background:#0b1220;color:#e5e7eb}
.wrap{max-width:640px;margin:0 auto;padding:20px 16px 60px}
h1{font-size:20px;margin:8px 0 16px}h3{margin:20px 0 4px;color:#93a3b8}
label{display:block;margin:10px 0 4px;font-size:13px;color:#93a3b8}
input,select{width:100%;padding:10px;border-radius:8px;border:1px solid #2a3550;background:#131c31;color:#e5e7eb;box-sizing:border-box;font-size:14px}
.tile{border:1px solid #2a3550;border-radius:12px;padding:12px;margin:10px 0;background:#0f1730}
.row{display:flex;gap:8px}.row>*{flex:1}
button{margin-top:18px;width:100%;padding:13px;border:0;border-radius:10px;background:#2563eb;color:#fff;font-size:15px;font-weight:600}
#msg{margin-top:10px;text-align:center}.ok{color:#34d399}.chk{display:flex;align-items:center;gap:8px;margin-top:8px}
.chk input{width:auto}
button.ghost{background:#0f1730;border:1px solid #2a3550;color:#93a3b8;font-weight:500;margin-top:10px}
.hint{font-size:12px;color:#93a3b8;margin:6px 0}
select.pick{margin-bottom:2px}
</style></head><body><div class="wrap">
<h1>&#128241; Smart Panel Setup</h1>
<label>Room name (shown top-left)</label><input id="room">
<label>Homebridge URL</label><input id="url" placeholder="http://192.168.1.50:8581">
<div class="row"><div><label>Username</label><input id="user"></div>
<div><label>Password</label><input id="pass" type="password"></div></div>
<h3>Tiles</h3>
<button type="button" class="ghost" onclick="loadDevices()">&#128269; Load device list from Homebridge</button>
<div id="dmsg" class="hint">Tip: enter your Homebridge URL + login and Save first, then load the list to pick devices by name.</div>
<div id="tiles"></div>
<button onclick="save()">Save</button><div id="msg"></div>
</div><script>
var TYPES=['light','switch','fan','cover','climate','scene','script'];
var DEVICES=null;
function esc(s){return (s||'').replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;')}
function loadDevices(n){n=n||0;$('dmsg').textContent='Loading devices from Homebridge...';
 fetch('/hbdevices').then(function(r){return r.json()}).then(function(d){
  if(d&&d.loading){if(n>12){$('dmsg').textContent='Timed out. Save your URL + login, then try again.';return;}
   setTimeout(function(){loadDevices(n+1)},1500);return;}
  DEVICES=d;fillPickers();$('dmsg').textContent=((d&&d.length)||0)+' devices loaded - pick one per tile.';})
 .catch(function(){$('dmsg').textContent='Could not load devices (save your URL + login first).';});}
function fillPickers(){if(!DEVICES)return;for(var i=0;i<6;i++){var sel=$('pick'+i);if(!sel)continue;
  var cur=($('uid'+i).value||'');var h='<option value="">- pick a device -</option>';
  DEVICES.forEach(function(dev){h+='<option value="'+esc(dev.u)+'"'+(dev.u===cur?' selected':'')+'>'+esc(dev.n||dev.u)+'</option>';});
  sel.innerHTML=h;}}
function onPick(i){var sel=$('pick'+i),uid=sel.value;if(!uid)return;$('uid'+i).value=uid;
  var nm=sel.options[sel.selectedIndex].text;if(!$('title'+i).value)$('title'+i).value=nm;}
function row(i,t){t=t||{};return '<div class="tile"><b>Tile '+(i+1)+'</b>'
+'<label>Device</label><select class="pick" id="pick'+i+'" onchange="onPick('+i+')"><option value="">- load list, then pick -</option></select>'
+'<label>Accessory uniqueId</label><input id="uid'+i+'" value="'+(t.uid||'')+'">'
+'<div class="row"><div><label>Title</label><input id="title'+i+'" value="'+(t.title||'')+'"></div>'
+'<div><label>Type</label><select id="type'+i+'">'+TYPES.map(function(x){return '<option '+(t.type==x?'selected':'')+'>'+x+'</option>'}).join('')+'</select></div></div>'
+'<label class="chk"><input type="checkbox" id="en'+i+'" '+(t.enabled?'checked':'')+'> Show this tile</label></div>'}
function $(id){return document.getElementById(id)}
function load(){fetch('/hbcfg').then(function(r){return r.json()}).then(function(c){
$('room').value=c.room||'';$('url').value=c.url||'';$('user').value=c.user||'';$('pass').value=c.pass||'';
var h='';for(var i=0;i<6;i++)h+=row(i,(c.tiles||[])[i]);$('tiles').innerHTML=h;fillPickers();})}
function save(){var t=[];for(var i=0;i<6;i++)t.push({uid:$('uid'+i).value,title:$('title'+i).value,type:$('type'+i).value,enabled:$('en'+i).checked});
var s=JSON.stringify({room:$('room').value,url:$('url').value,user:$('user').value,pass:$('pass').value,tiles:t});
var CH=200,parts=[];for(var i=0;i<s.length;i+=CH)parts.push(s.slice(i,i+CH));if(!parts.length)parts=[''];
$('msg').textContent='Saving...';
(function send(k){if(k>=parts.length){$('msg').innerHTML='<span class="ok">Saved &#10003;</span>';return;}
var b='part='+encodeURIComponent(parts[k])+'&first='+(k===0?'1':'0')+'&last='+(k===parts.length-1?'1':'0');
fetch('/hbcfg',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:b}).then(function(){send(k+1)}).catch(function(){$('msg').textContent='Save error'})})(0);}
load();
</script></body></html>)HTML";

void HbConfig::setup() {
  this->pref_ = global_preferences->make_preference<char[CFG_MAX]>(fnv1_hash("hb_config_v1"));
  memset(this->save_buf_, 0, CFG_MAX);
  if (this->pref_.load(&this->save_buf_)) {
    this->config_json_ = this->save_buf_;
    ESP_LOGI(TAG, "Loaded config (%d bytes)", (int) this->config_json_.size());
  } else {
    this->config_json_ = DEFAULT_CFG;
    ESP_LOGI(TAG, "No stored config; using default");
  }
  this->parse_();
  if (web_server_base::global_web_server_base != nullptr) {
    web_server_base::global_web_server_base->add_handler(this);
    ESP_LOGI(TAG, "Registered /setup web handler");
  } else {
    ESP_LOGW(TAG, "web_server_base not available");
  }
}

void HbConfig::dump_config() {
  ESP_LOGCONFIG(TAG, "HB Config: room='%s' configured=%d", this->room_.c_str(),
                (int) this->configured());
}

bool HbConfig::canHandle(AsyncWebServerRequest *request) const {
  return request->url() == "/setup" || request->url() == "/hbcfg" ||
         request->url() == "/hbdevices";
}

void HbConfig::handleRequest(AsyncWebServerRequest *request) {
  if (request->url() == "/setup") {
    request->send(200, "text/html", SETUP_HTML);
    return;
  }
  if (request->url() == "/hbdevices") {
    // Return the cached compact device list, or ask the engine to fetch it.
    if (this->devices_ready_) {
      request->send(200, "application/json", this->devices_json_.c_str());
    } else {
      this->want_devices_ = true;
      request->send(200, "application/json", "{\"loading\":true}");
    }
    return;
  }
  // /hbcfg  (web_server_idf exposes the POST body as form args)
  if (request->method() == HTTP_GET) {
    request->send(200, "application/json", this->config_json_.c_str());
  } else {
    // Config arrives in small chunks (web_server_idf caps the POST body ~512 B):
    // part=<chunk>&first=<0|1>&last=<0|1>. Reassemble, save on the last chunk.
    if (request->hasArg("part")) {
      if (request->arg("first") == "1")
        this->chunk_buf_.clear();
      this->chunk_buf_ += request->arg("part");
      if (request->arg("last") == "1") {
        this->pending_json_ = this->chunk_buf_;  // defer the actual save to loop()
        this->pending_save_ = true;              // (main task has a large stack)
      }
    } else if (request->hasArg("cfg")) {
      this->pending_json_ = request->arg("cfg");
      this->pending_save_ = true;
    }
    request->send(200, "application/json", "{\"ok\":true}");
  }
}

void HbConfig::loop() {
  if (this->pending_save_) {
    this->pending_save_ = false;
    this->save_(this->pending_json_);
  }
}

void HbConfig::save_(const std::string &json) {
  if (json.size() >= CFG_MAX) {
    ESP_LOGW(TAG, "Config too large (%d B), not saved", (int) json.size());
    return;
  }
  this->config_json_ = json;
  memset(this->save_buf_, 0, CFG_MAX);
  strncpy(this->save_buf_, json.c_str(), CFG_MAX - 1);
  this->pref_.save(&this->save_buf_);
  global_preferences->sync();
  this->parse_();
  ESP_LOGI(TAG, "Saved config (%d B), room='%s'", (int) json.size(), this->room_.c_str());
}

void HbConfig::parse_() {
  this->room_.clear();
  this->url_.clear();
  this->user_.clear();
  this->pass_.clear();
  for (auto &t : this->tiles_)
    t = TileCfg{};
  json::parse_json(this->config_json_, [this](JsonObject root) -> bool {
    this->room_ = root["room"] | "";
    this->url_ = root["url"] | "";
    this->user_ = root["user"] | "";
    this->pass_ = root["pass"] | "";
    JsonArray tiles = root["tiles"];
    int i = 0;
    for (JsonObject t : tiles) {
      if (i >= 6)
        break;
      this->tiles_[i].uid = t["uid"] | "";
      this->tiles_[i].title = t["title"] | "";
      this->tiles_[i].type = t["type"] | "";
      this->tiles_[i].enabled = t["enabled"] | false;
      i++;
    }
    return true;
  });
  this->gen_++;  // signal YAML to re-apply room/titles/visibility
}

void HbConfig::begin_fetch() {
  this->fetching_ = true;
  this->fetch_ticks_ = 0;
  ESP_LOGI(TAG, "Fetching device list (free heap %u, largest block %u)",
           (unsigned) heap_caps_get_free_size(MALLOC_CAP_8BIT),
           (unsigned) heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}

void HbConfig::tick_fetch_watchdog() {
  // Called ~1 Hz from the engine. If a fetch never came back (error/timeout,
  // which doesn't run on_response), clear the guard after ~10 s so it can retry.
  if (this->fetching_ && ++this->fetch_ticks_ > 10) {
    ESP_LOGW(TAG, "Device fetch timed out; will retry");
    this->fetching_ = false;
    this->fetch_ticks_ = 0;
  }
}

void HbConfig::set_devices_json(const std::string &raw) {
  // The Homebridge layout is [{"name":room,"services":[{"uniqueId","name",...}]}].
  // Scan it into a compact [{"u":uid,"n":name}] list without a full DOM parse — the
  // raw payload is tens of KB and an ArduinoJson document would be memory-heavy.
  // ("nameBasedUniqueId" never contains the exact "uniqueId":" key, and the service
  // name is always the first "name":" after each uniqueId, so a linear scan is safe.)
  static const std::string KUID = "\"uniqueId\":\"";
  static const std::string KNAME = "\"name\":\"";
  std::string out = "[";
  bool first = true;
  size_t pos = 0;
  while ((pos = raw.find(KUID, pos)) != std::string::npos) {
    pos += KUID.size();
    size_t uend = raw.find('"', pos);
    if (uend == std::string::npos)
      break;
    std::string uid = raw.substr(pos, uend - pos);
    pos = uend + 1;
    std::string name;
    size_t np = raw.find(KNAME, pos);
    if (np != std::string::npos) {
      np += KNAME.size();
      size_t nend = np;
      while (true) {
        nend = raw.find('"', nend);
        if (nend == std::string::npos)
          break;
        if (raw[nend - 1] != '\\')  // unescaped quote = end of the name
          break;
        nend++;                     // escaped quote, keep scanning
      }
      if (nend != std::string::npos)
        name = raw.substr(np, nend - np);  // keep any JSON escaping as-is
    }
    if (uid.empty())
      continue;
    if (!first)
      out += ",";
    first = false;
    out += "{\"u\":\"";
    out += uid;
    out += "\",\"n\":\"";
    out += name;
    out += "\"}";
  }
  out += "]";
  this->devices_json_ = out;
  this->devices_ready_ = true;
  this->fetching_ = false;
  this->want_devices_ = false;
  ESP_LOGI(TAG, "Device list ready (%d bytes, free heap %u)", (int) out.size(),
           (unsigned) heap_caps_get_free_size(MALLOC_CAP_8BIT));
}

}  // namespace hb_config
}  // namespace esphome
