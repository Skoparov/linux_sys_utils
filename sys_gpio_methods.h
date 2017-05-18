#ifndef __SYS_GPIO_METHODS_H__
#define __SYS_GPIO_METHODS_H__

#include <string>

namespace utils
{

namespace sys
{

namespace gpio
{

enum class gpio_direction{ in, out };

/// \brief Enable gpio
void enable_gpio_line( uint32_t line, const gpio_direction& direction );

/// \brief Disable gpio
void disable_gpio_line( uint32_t line );

/// \brief Set gpio direction
void set_gpio_line_direction( uint32_t line, const gpio_direction& direction );

/// \brief Get gpio direction
gpio_direction get_gpio_line_direction( uint32_t line );

/// \brief Check if line is enabled
bool gpio_line_enabled( uint32_t line );

/// \brief Set gpio line status
void set_gpio_line_status( uint32_t line, int8_t status );

/// \brief Get gpio line status
int8_t get_gpio_line_status( uint32_t line );

}


}

}


#endif
