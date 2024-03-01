#ifndef LED_STRIP_HPP
#define LED_STRIP_HPP

#include <led_strip.h>

#include "component.hpp"
#include "declarations.hpp"
#include "effects.hpp"

namespace ring_lights {

#if defined(CONFIG_SK6812)
	#define LED_TYPE led_strip_type_t::LED_STRIP_SK6812
#elif defined(CONFIG_WS2812)
	#define LED_TYPE led_strip_type_t::LED_STRIP_WS2812
#elif defined(CONFIG_APA106)
	#define LED_TYPE led_strip_type_t::LED_STRIP_APA106
#elif defined(CONFIG_SM16703)
	#define LED_TYPE led_strip_type_t::LED_STRIP_SM16703
#else
	#error Please define a valid LED type using menuconfig
#endif

class ring_lights : public component,
	public has_queue<1, effect_msg, 0>,
	public has_queue<1, brightness_msg, 0> {
public:
	ring_lights() = default;
	~ring_lights() = default;

	/* Component override functions */
	etl::string<50> get_tag() override { return TAG; };
	res get_status() override;
	res initialize() override;
	res run() override;
	res stop() override;

	void enqueue(effect_msg& msg) override { effect_msg_queue::enqueue(msg); }
	void enqueue(brightness_msg& msg) override { brightness_msg_queue::enqueue(msg); }

private:
	using effect_msg_queue = has_queue<1, effect_msg, 0>;
	using brightness_msg_queue = has_queue<1, brightness_msg, 0>;

	static const inline char TAG[] = "Ring lights";
	rgb_t m_current_effect_buffer[NUM_LEDS]{};
	rgb_t m_new_effect_buffer[NUM_LEDS]{};

	COMPONENT_STATUS m_status = UNINITIALIZED;
	etl::string<128> m_err_status;
	bool m_run = false;
	uint32_t m_effect_transition_ticks_left = 0;

	effect_msg m_current_effect{
		.effect = RAINBOW_UNIFORM,
		.param_a = 0,
		.param_b = 0,
		.primary_color = {.h = 0, .s = 0, .v = 0},
		.secondary_color = {.h = 0, .s = 0, .v = 0}
	};

	effect_msg m_new_effect;

	static void start_flush(void*);
	void flush_thread();
	void transition_effect();

	effect_func m_current_effect_func_p = &effects::rainbow_uniform;
	effect_func m_new_effect_func_p = &effects::rainbow_uniform;

	inline static led_strip_t m_strip = {
		.type = LED_TYPE,
		.is_rgbw = false,
		.brightness = 63,
		.length = NUM_LEDS,
		.gpio = gpio_num_t(DATA_PIN),
		.channel = (rmt_channel_t) CONFIG_LED_RMT_CHANNEL,
		.buf = nullptr
	};
};

} /* namespace ring_lights */


#endif /* LED_STRIP_HPP */