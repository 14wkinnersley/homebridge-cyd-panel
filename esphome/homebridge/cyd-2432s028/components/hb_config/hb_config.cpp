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
:root{--bg:#f5f5f7;--card:#ffffff;--line:#d2d2d7;--muted:#6e6e73;--text:#1d1d1f;--accent:#0071e3;--ok:#1d9d55;--shadow:0 1px 2px rgba(0,0,0,.05),0 10px 30px rgba(0,0,0,.06)}
*{box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",system-ui,sans-serif;margin:0;background:var(--bg);color:var(--text);-webkit-font-smoothing:antialiased}
.wrap{max-width:600px;margin:0 auto;padding:28px 16px 120px}
.hdr{margin:4px 2px 22px}
.hdr h1{font-size:26px;margin:0;font-weight:700;letter-spacing:-.02em}
.hdr p{margin:5px 0 0;font-size:14px;color:var(--muted)}
.card{background:var(--card);border:1px solid var(--line);border-radius:18px;padding:18px;margin:16px 0;box-shadow:var(--shadow)}
.card>h2{font-size:11px;text-transform:uppercase;letter-spacing:.06em;color:var(--muted);margin:0 0 10px;font-weight:600}
label{display:block;margin:12px 0 5px;font-size:13px;color:var(--muted)}
input,select{width:100%;padding:11px 12px;border-radius:10px;border:1px solid var(--line);background:#fff;color:var(--text);font-size:15px;outline:none;transition:border-color .15s,box-shadow .15s}
input:focus,select:focus{border-color:var(--accent);box-shadow:0 0 0 3px rgba(0,113,227,.15)}
.row{display:flex;gap:10px}.row>*{flex:1;min-width:0}
.tile{border:1px solid var(--line);border-radius:14px;padding:14px;margin:12px 0;background:#f5f5f7}
.tile .th{display:flex;align-items:center;gap:9px;margin-bottom:2px}
.badge{display:inline-flex;align-items:center;justify-content:center;width:22px;height:22px;border-radius:7px;background:var(--accent);color:#fff;font-size:12px;font-weight:700}
.tile .th b{font-size:14px}
.mono{font-family:ui-monospace,Menlo,Consolas,monospace;font-size:12px}
.chk{display:flex;align-items:center;gap:9px;margin-top:12px;font-size:14px;color:var(--text)}
.chk input{width:auto;transform:scale(1.15);accent-color:var(--accent)}
.ghost{width:100%;padding:12px;border:1px solid var(--line);border-radius:11px;background:#fff;color:var(--accent);font-size:14px;font-weight:600;cursor:pointer}
.ghost:active{background:#f0f0f2}
.hint{font-size:12px;color:var(--muted);margin:8px 2px 2px}
details{margin-top:10px}
details summary{cursor:pointer;color:var(--muted);font-size:12px;list-style:none;user-select:none}
details summary::-webkit-details-marker{display:none}
details summary:before{content:"\25B8 ";}
details[open] summary:before{content:"\25BE ";}
.bar{position:fixed;left:0;right:0;bottom:0;padding:12px 16px calc(14px + env(safe-area-inset-bottom));background:linear-gradient(180deg,rgba(245,245,247,0),var(--bg) 34%)}
.bar .inner{max-width:600px;margin:0 auto}
.save{width:100%;padding:14px;border:0;border-radius:12px;background:var(--accent);color:#fff;font-size:16px;font-weight:600;cursor:pointer;box-shadow:0 6px 16px rgba(0,113,227,.28)}
.save:active{opacity:.9}
#msg{text-align:center;font-size:13px;margin-top:8px;min-height:16px}
.ok{color:var(--ok);font-weight:600}
</style></head><body><div class="wrap">
<div class="hdr"><h1>Smart Panel Setup</h1><p>Configure your Homebridge wall panel</p></div>

<div class="card"><h2>Homebridge</h2>
<label>Room name (shown top-left on the panel)</label><input id="room" placeholder="Office">
<label>Homebridge URL</label><input id="url" class="mono" placeholder="http://192.168.1.50:8581">
<div class="row"><div><label>Username</label><input id="user"></div>
<div><label>Password</label><input id="pass" type="password"></div></div>
</div>

<div class="card"><h2>Tiles</h2>
<button type="button" class="ghost" onclick="loadDevices()">Load devices from Homebridge</button>
<div id="dmsg" class="hint">Enter your URL + login and tap Save first, then load the list to pick devices by name.</div>
<div id="tiles"></div>
</div>
</div>
<div class="bar"><div class="inner"><button class="save" onclick="save()">Save</button><div id="msg"></div></div></div>
<script>
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
function row(i,t){t=t||{};return '<div class="tile">'
+'<div class="th"><span class="badge">'+(i+1)+'</span><b>Tile '+(i+1)+'</b></div>'
+'<label>Device</label><select class="pick" id="pick'+i+'" onchange="onPick('+i+')"><option value="">- load list, then pick -</option></select>'
+'<div class="row"><div><label>Title</label><input id="title'+i+'" value="'+esc(t.title||'')+'"></div>'
+'<div><label>Type</label><select id="type'+i+'">'+TYPES.map(function(x){return '<option '+(t.type==x?'selected':'')+'>'+x+'</option>'}).join('')+'</select></div></div>'
+'<label class="chk"><input type="checkbox" id="en'+i+'" '+(t.enabled?'checked':'')+'> Show this tile</label>'
+'<details><summary>Advanced &#183; uniqueId</summary><input id="uid'+i+'" class="mono" value="'+esc(t.uid||'')+'" placeholder="paste a uniqueId if not in the list above"></details>'
+'</div>'}
function $(id){return document.getElementById(id)}
function load(){fetch('/hbcfg').then(function(r){return r.json()}).then(function(c){
$('room').value=c.room||'';$('url').value=c.url||'';$('user').value=c.user||'';$('pass').value=c.pass||'';
var h='';for(var i=0;i<6;i++)h+=row(i,(c.tiles||[])[i]);$('tiles').innerHTML=h;fillPickers();})}
function save(){var t=[];for(var i=0;i<6;i++)t.push({uid:$('uid'+i).value,title:$('title'+i).value,type:$('type'+i).value,enabled:$('en'+i).checked});
var s=JSON.stringify({room:$('room').value,url:$('url').value,user:$('user').value,pass:$('pass').value,tiles:t});
var CH=200,parts=[];for(var i=0;i<s.length;i+=CH)parts.push(s.slice(i,i+CH));if(!parts.length)parts=[''];
$('msg').textContent='Saving...';
(function send(k){if(k>=parts.length){$('msg').innerHTML='<span class="ok">Saved &#10003; &mdash; changes apply live</span>';return;}
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
