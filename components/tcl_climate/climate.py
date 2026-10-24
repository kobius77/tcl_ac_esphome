import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, uart, sensor
from esphome.const import CONF_ID, UNIT_CELSIUS, UNIT_WATT, DEVICE_CLASS_TEMPERATURE, DEVICE_CLASS_POWER, STATE_CLASS_MEASUREMENT

DEPENDENCIES = ['uart']
AUTO_LOAD = ['climate']

CONF_EXT_TEMP_SENSOR = 'ext_temp_sensor'
CONF_EXT_TEMP_OUTPUT = 'ext_temp'
CONF_INTERNAL_TEMP = 'internal_temp'
CONF_POWER_SENSOR = 'power_sensor'
CONF_POWER_OUTPUT = 'power'
CONF_SENT_TEMP_OUTPUT = 'sent_temp'
CONF_BEEP = 'beep'
CONF_DISPLAY = 'display'
CONF_RAMP_MODE = 'ramp_mode'

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
    cv.Optional(CONF_EXT_TEMP_OUTPUT): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_POWER_OUTPUT): sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_SENT_TEMP_OUTPUT): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_BEEP, default=True): cv.boolean,
    cv.Optional(CONF_DISPLAY, default=True): cv.boolean,
    cv.Optional(CONF_RAMP_MODE, default=False): cv.boolean,
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
    if CONF_EXT_TEMP_OUTPUT in config:
        sens = await sensor.new_sensor(config[CONF_EXT_TEMP_OUTPUT])
        cg.add(var.set_ext_temp_output(sens))
    if CONF_POWER_OUTPUT in config:
        sens = await sensor.new_sensor(config[CONF_POWER_OUTPUT])
        cg.add(var.set_power_output(sens))
    if CONF_SENT_TEMP_OUTPUT in config:
        sens = await sensor.new_sensor(config[CONF_SENT_TEMP_OUTPUT])
        cg.add(var.set_sent_temp_output(sens))
    if CONF_BEEP in config:
        cg.add(var.set_beep(config[CONF_BEEP]))
    if CONF_DISPLAY in config:
        cg.add(var.set_display(config[CONF_DISPLAY]))
    if CONF_RAMP_MODE in config:
        cg.add(var.set_ramp_mode(config[CONF_RAMP_MODE]))