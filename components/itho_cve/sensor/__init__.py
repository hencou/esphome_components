import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_HUMIDITY,
    CONF_ID,
    CONF_TEMPERATURE,
    DEVICE_CLASS_HUMIDITY,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_PERCENT,
)
from .. import itho_cve_ns, CONF_ITHOCVE_ID, IthoCVE

#DEPENDENCIES = ["itho_cve"]
AUTO_LOAD = ["itho_cve"]

IthoCVE_Sensor = itho_cve_ns.class_("IthoCVE_Sensor", sensor.Sensor, cg.PollingComponent)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(IthoCVE_Sensor),
            cv.GenerateID(CONF_ITHOCVE_ID): cv.use_id(IthoCVE),
            cv.Required(CONF_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Required(CONF_HUMIDITY): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_HUMIDITY,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
        }
    )
    .extend(cv.polling_component_schema("60s"))
  )


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    if CONF_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_TEMPERATURE])
        cg.add(var.set_temperature_sensor(sens))

    if CONF_HUMIDITY in config:
        sens = await sensor.new_sensor(config[CONF_HUMIDITY])
        cg.add(var.set_humidity_sensor(sens))
    
    paren = await cg.get_variable(config[CONF_ITHOCVE_ID])
    cg.add(var.set_itho_cve_parent(paren))
