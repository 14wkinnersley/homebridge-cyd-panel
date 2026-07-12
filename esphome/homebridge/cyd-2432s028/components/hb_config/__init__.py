import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

# Custom component: serves an on-device "/setup" web page and stores the panel
# configuration (Homebridge URL/creds, room name, tiles) in flash (NVS).
DEPENDENCIES = ["web_server_base"]
AUTO_LOAD = ["json"]
CODEOWNERS = ["@wells"]

hb_config_ns = cg.esphome_ns.namespace("hb_config")
HbConfig = hb_config_ns.class_("HbConfig", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(HbConfig),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
