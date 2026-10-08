#pragma once
#include <cstddef>
#include <cstdint>
#ifdef __cplusplus
extern "C" {
#endif
void SecondScreenStartUsbAccessory(void);
void SecondScreenStopUsbAccessory(void);
bool SecondScreenUsbAccessorySend(const std::uint8_t* data, std::size_t size);
#ifdef __cplusplus
}
#endif
