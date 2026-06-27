import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, uart, sensor
from esphome.const import CONF_ID, UNIT_CELSIUS, DEVICE_CLASS_TEMPERATURE, STATE_CLASS_MEASUREMENT

DEPENDENCIES = ['uart']
AUTO_LOAD = ['climate']

CONF_EXT_TEMP_SENSOR = 'ext_temp_sensor'
CONF_INTERNAL_TEMP = 'internal_temp'
CONF_POWER_SENSOR = 'power_sensor'
CONF_DISPLAY = 'display'

tcl_climate_ns = cg.esphome_ns.namespace('tcl_climate')
TCLClimate = tcl_climate_ns.class_('TCLClimate', climate.Climate, uart.UARTDevice, cg.PollingComponent)

CONFIG_SCHEMA = climate.climate_schema(TCLClimate).extend({
    cv.GenerateID(): cv.declare_id(TCLClimate),
    cv.Optional(CONF_EXT_TEMP_SENSOR): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_INTERNAL_TEMP): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_POWER_SENSOR): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_DISPLAY, default=True): cv.boolean,
}).extend(uart.UART_DEVICE_SCHEMA).extend(cv.polling_component_schema('450ms'))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)
    await uart.register_uart_device(var, config)
    if CONF_EXT_TEMP_SENSOR in config:
        sens = await cg.get_variable(config[CONF_EXT_TEMP_SENSOR])
        cg.add(var.set_ext_temp_sensor(sens))
    if CONF_INTERNAL_TEMP in config:
        sens = await sensor.new_sensor(config[CONF_INTERNAL_TEMP])
        cg.add(var.set_int_temp_sensor(sens))
    if CONF_POWER_SENSOR in config:
        sens = await cg.get_variable(config[CONF_POWER_SENSOR])
        cg.add(var.set_power_sensor(sens))
    if CONF_DISPLAY in config:
        cg.add(var.set_display(config[CONF_DISPLAY]))