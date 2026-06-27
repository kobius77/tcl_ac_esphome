import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, uart, sensor
from esphome.const import CONF_ID, UNIT_CELSIUS, UNIT_WATT, DEVICE_CLASS_TEMPERATURE, DEVICE_CLASS_POWER, STATE_CLASS_MEASUREMENT

DEPENDENCIES = ['uart']
AUTO_LOAD = ['climate']

CONF_EXT_TEMP_SENSOR = 'ext_temp_sensor'
CONF_EXT_TEMP_SENSOR_2 = 'ext_temp_sensor_2'
CONF_INTERNAL_TEMP = 'internal_temp'
CONF_POWER_SENSOR = 'power_sensor'
CONF_EXT_TEMP = 'ext_temp'
CONF_EXT_TEMP_2 = 'ext_temp_2'
CONF_POWER = 'power'
CONF_SENT_TEMP = 'sent_temp'
CONF_FALLBACK_TEMP = 'fallback_temperature'
CONF_DISPLAY = 'display'
CONF_BEEP = 'beep'
CONF_REGULATION_INTERVAL = 'regulation_interval'
CONF_EXT_TEMP_METHOD = 'ext_temp_method'

tcl_climate_ns = cg.esphome_ns.namespace('tcl_climate')
TCLClimate = tcl_climate_ns.class_('TCLClimate', climate.Climate, uart.UARTDevice, cg.PollingComponent)

CONFIG_SCHEMA = climate.climate_schema(TCLClimate).extend({
    cv.GenerateID(): cv.declare_id(TCLClimate),
    cv.Optional(CONF_EXT_TEMP_SENSOR): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_EXT_TEMP_SENSOR_2): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_INTERNAL_TEMP): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_POWER_SENSOR): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_EXT_TEMP): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_EXT_TEMP_2): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_POWER): sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_SENT_TEMP): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_FALLBACK_TEMP, default=23.0): cv.temperature,
    cv.Optional(CONF_DISPLAY, default=True): cv.boolean,
    cv.Optional(CONF_BEEP, default=True): cv.boolean,
    cv.Optional(CONF_REGULATION_INTERVAL, default="30s"): cv.positive_time_period_milliseconds,
    cv.Optional(CONF_EXT_TEMP_METHOD, default="Primary"): cv.string_strict,
}).extend(uart.UART_DEVICE_SCHEMA).extend(cv.polling_component_schema('450ms'))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)
    await uart.register_uart_device(var, config)
    if CONF_EXT_TEMP_SENSOR in config:
        sens = await cg.get_variable(config[CONF_EXT_TEMP_SENSOR])
        cg.add(var.set_ext_temp_sensor(sens))
    if CONF_EXT_TEMP_SENSOR_2 in config:
        sens = await cg.get_variable(config[CONF_EXT_TEMP_SENSOR_2])
        cg.add(var.set_ext_temp_sensor_2(sens))
    if CONF_INTERNAL_TEMP in config:
        sens = await sensor.new_sensor(config[CONF_INTERNAL_TEMP])
        cg.add(var.set_int_temp_sensor(sens))
    if CONF_POWER_SENSOR in config:
        sens = await cg.get_variable(config[CONF_POWER_SENSOR])
        cg.add(var.set_power_sensor(sens))
    if CONF_EXT_TEMP in config:
        sens = await sensor.new_sensor(config[CONF_EXT_TEMP])
        cg.add(var.set_ext_temp_output(sens))
    if CONF_EXT_TEMP_2 in config:
        sens = await sensor.new_sensor(config[CONF_EXT_TEMP_2])
        cg.add(var.set_ext_temp_2_output(sens))
    if CONF_POWER in config:
        sens = await sensor.new_sensor(config[CONF_POWER])
        cg.add(var.set_power_output(sens))
    if CONF_SENT_TEMP in config:
        sens = await sensor.new_sensor(config[CONF_SENT_TEMP])
        cg.add(var.set_sent_temp_output(sens))
    if CONF_FALLBACK_TEMP in config:
        cg.add(var.set_fallback_temperature(config[CONF_FALLBACK_TEMP]))
    if CONF_DISPLAY in config:
        cg.add(var.set_display(config[CONF_DISPLAY]))
    if CONF_BEEP in config:
        cg.add(var.set_beep(config[CONF_BEEP]))
    if CONF_REGULATION_INTERVAL in config:
        cg.add(var.set_regulation_interval(config[CONF_REGULATION_INTERVAL]))
    if CONF_EXT_TEMP_METHOD in config:
        cg.add(var.set_ext_temp_method(config[CONF_EXT_TEMP_METHOD]))
