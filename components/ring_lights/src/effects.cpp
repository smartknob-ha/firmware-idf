#include "../include/effects.hpp"
#include <math.h>
#include <cmath>

#include "esp_log.h"

namespace ring_lights {

#define M_TAU (2 * M_PI)

#define DEGREE_PER_LED (360.0 / NUM_LEDS)
#define RAD_PER_LED (M_TAU / NUM_LEDS)

// Reset an angle to between 0 and +inf
#define WRAP_NEGATIVE_DEGREE(a) (a < 0.0 ? 360.0 - a : a)

// Mirror a radian on the right half of the unit circle to the left half
#define REFLECT_ON_Y_RAD(a) (a >= 0 ? M_PI - a : -M_PI - a)

#define GET_SMALLEST_DEGREE_DIFFERENCE(a, b) (180.0 - abs(abs(a - b) - 180.0))

#define DEGREE_TO_RADIAN(a) (a * M_PI / 180)

// Just make sure to enter your angles in clockwise order
int_fast16_t GET_CLOCKWISE_DIFF_DEGREES(int_fast16_t a, int_fast16_t b) {
	int_fast16_t diff = b - a;
	if (diff < 0) {
		return diff + 360;
	}
	return diff;
}

// Just make sure to enter your angles in clockwise order
double GET_CLOCKWISE_DIFF_RADIAN(double a, double b) {
	double diff = a - b;
	if (diff < 0) {
		return fmod(diff + M_TAU, 2 * M_PI);
	}
	return diff;
}

int_fast16_t GET_COUNTER_CLOCKWISE_DIFF(int_fast16_t a, int_fast16_t b) {
	int_fast16_t diff = a - b;
	if (diff < 0) {
		return diff + 360;
	}
	return diff;
}
/* IS_BETWEEN_A_B_CLOCKWISE_DEGREES calculates whether angle c is between angle a and b
*		in a clockwise direction
*  @returns how far the current angle is between a and b, clockwise.
*/
int_fast16_t IS_BETWEEN_A_B_CLOCKWISE_DEGREES(int_fast16_t a, int_fast16_t b, int_fast16_t c) {
	int_fast16_t relative_total_width = a + GET_CLOCKWISE_DIFF_DEGREES(a, b);
	int_fast16_t relative_angle = a + GET_CLOCKWISE_DIFF_DEGREES(a, c);

	return relative_angle - relative_total_width;
}

/* IS_BETWEEN_A_B_CLOCKWISE_RADIAN calculates whether angle c is between angle a and b
*		in a clockwise direction
*  @returns how far the current angle is between a and b, clockwise.
*/
double IS_BETWEEN_A_B_CLOCKWISE_RADIAN(double a, double b, double c) {
	double relative_total_width = a + GET_CLOCKWISE_DIFF_RADIAN(a, b);
	double relative_angle = a + GET_CLOCKWISE_DIFF_RADIAN(a, c);

	return relative_angle - relative_total_width;
}

template<typename T>
double GET_LED_ANGLE_DEGREES(T led) {
	static_assert(std::is_arithmetic<T>::value);
	double angle = 270 - (double) led * DEGREE_PER_LED;
	if (angle < 0) {
		angle = 360 + angle;
	}
	return angle;
}

template<typename T>
double GET_LED_ANGLE_RAD(T led) {
	static_assert(std::is_arithmetic<T>::value);
	double angle = (led * RAD_PER_LED) + (RAD_PER_LED / 2) - M_PI;
	if (angle < 0) {
		angle = M_TAU + angle;
	}
	if (angle > M_PI) {
		angle = angle - M_TAU;
	}
	return angle;
}

bool HSV_IS_EQUAL(hsv_t a, hsv_t b) {
	return a.h == b.h && a.s == b.s && a.v == b.v;
}

void effects::pointer(rgb_t (& buffer)[NUM_LEDS], effect_msg& msg) {
	int_fast16_t angle_degrees = WRAP_NEGATIVE_DEGREE(msg.param_a);
	int_fast16_t width_degree = msg.param_b;
	int_fast16_t width_half_pointer_degree = width_degree / 2;

	hsv_t color = msg.primary_color;

	for (int_fast8_t i = NUM_LEDS - 1; i >= 0; i--) {
		double current_degree = i * DEGREE_PER_LED;
		double degrees_to_center = GET_SMALLEST_DEGREE_DIFFERENCE(angle_degrees, current_degree);
		double progress = 0.0;

		if (degrees_to_center <= width_half_pointer_degree) {
			progress = degrees_to_center / width_degree;
		}

		if (-1 <= progress && progress <= 0) {
			color.value = static_cast<uint8_t>(static_cast<double>(msg.primary_color.value) * abs(progress));
		} else if (0 < progress && progress <= 1) {
			color.value = static_cast<uint8_t>(static_cast<double>(msg.primary_color.value) * (1 - progress));
		}

		buffer[wrap_index(i)] = hsv2rgb_rainbow(color);
		color.value = 0;
	}
}

void effects::percent(rgb_t (& buffer)[NUM_LEDS], effect_msg& msg) {
	int_fast16_t start = msg.param_a;
	int_fast16_t end = msg.param_b;

	// Scale `end` to percentage
	double percent = static_cast<double>(msg.param_c) / 100;
	int_fast16_t total_width = GET_CLOCKWISE_DIFF_DEGREES(start, end);
	int_fast16_t correct_end = start + (total_width * (percent));
	correct_end %= 360;

	for (int_fast8_t i = 0; i < NUM_LEDS; i++) {
		double current_degree = GET_LED_ANGLE_DEGREES(i);
		if (IS_BETWEEN_A_B_CLOCKWISE_DEGREES(start, correct_end, current_degree)) {
			buffer[i] = hsv2rgb_rainbow(msg.primary_color);
		} else {
			buffer[i] = {{0}, {0}, {0}};
		}
	}
}

void effects::fill(rgb_t (& buffer)[NUM_LEDS], effect_msg& msg) {
	for (auto& i : buffer) {
		i = hsv2rgb_rainbow(msg.primary_color);
	}
}

void effects::gradient(rgb_t (& buffer)[NUM_LEDS], effect_msg& msg) {
	// Since every opposite LED gets the same color,
	// the gradient buffer only needs to be half the size
	// of the LED buffer
	const static uint_fast8_t GRADIENT_RESOLUTION = NUM_LEDS / 2;

	static hsv_t p_primary_color, p_secondary_color;
	static rgb_t GRADIENT_BUFFER[GRADIENT_RESOLUTION];

	// Only recalculate the gradient if the colors changed
	if (!HSV_IS_EQUAL(msg.primary_color, p_primary_color) || !HSV_IS_EQUAL(msg.secondary_color, p_secondary_color)) {
		rgb_fill_gradient2_hsv(GRADIENT_BUFFER, GRADIENT_RESOLUTION, msg.primary_color, msg.secondary_color,
		                       COLOR_BACKWARD_HUES);
	}

	double gradient_angle = msg.param_a % 360;

	// Divide by 50 so 100 percent spans 2 units on the unit circle
	double gradient_width = static_cast<double>(msg.param_b) / 50.0;
	// Subtract 1 so 50 percent will be 0, which is the center of the unit circle
	double gradient_center = (static_cast<double>(msg.param_c) / 50.0) - 1;

	double gradient_start_corner = std::clamp(gradient_center + (gradient_width / 2), -1.0, 1.0);
	double gradient_end_corner = std::clamp(gradient_center - (gradient_width / 2), -1.0, 1.0);

	// The angles of the gradient drawn on the right are between .5 * PI and -.5 * PI
	double start_angle_right = asin(gradient_start_corner);
	double end_angle_right = asin(gradient_end_corner);
	double end_angle_left = REFLECT_ON_Y_RAD(end_angle_right);

	double width_radian = start_angle_right - end_angle_right;

	for (int_fast16_t i = 0; i < NUM_LEDS; i++) {
		double current_degree = GET_LED_ANGLE_RAD(i);
		current_degree -= DEGREE_TO_RADIAN(gradient_angle);

		double progress_right = GET_CLOCKWISE_DIFF_RADIAN(start_angle_right, current_degree)/* / width_radian*/;
		double progress_left = GET_CLOCKWISE_DIFF_RADIAN(end_angle_left, current_degree)/* / width_radian*/;


		buffer[i] = rgb_t {};
		if (progress_right < width_radian) {
			double progress = progress_right / width_radian;
			buffer[i] = GRADIENT_BUFFER[static_cast<int>(std::round(progress * GRADIENT_RESOLUTION))];
		} else if (progress_left < width_radian) {
			double progress = 1 - (progress_left / width_radian);
			buffer[i] = GRADIENT_BUFFER[static_cast<int>(std::round(progress * GRADIENT_RESOLUTION))];
		} else if (progress_left < M_PI) {
			buffer[i] = hsv2rgb_rainbow(msg.primary_color);
		} else {
			buffer[i] = hsv2rgb_rainbow(msg.secondary_color);
		}
	}
}

void effects::rainbow_uniform(rgb_t (& buffer)[NUM_LEDS], effect_msg& msg) {
	static uint8_t hsv_step = 0;
	hsv_step++;
	hsv_t color{
		.hue = hsv_step, .saturation = msg.primary_color.saturation, .value = msg.primary_color.value
	};

	rgb_t rgb = hsv2rgb_rainbow(color);

	// Give all LED's the same colour
	for (auto& i : buffer) {
		i = rgb;
	}
}

template<typename T>
T effects::wrap_index(T index) {
	static_assert(std::is_arithmetic<T>::value);

	if (0 <= index && index < NUM_LEDS) {
		return index;
	}

	return std::abs(index % NUM_LEDS);
}

} /* namespace ring_lights */