/**
 * @file reset_reason.h
 * @brief STM32H750 RCC reset-cause snapshot API.
 */
#ifndef BSP_BOARD_RESET_REASON_H
#define BSP_BOARD_RESET_REASON_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Capture RCC->RSR once for the current firmware image.
 * @note Call during early board initialization, before reset flags are cleared.
 *       The first capture is not thread-safe; initialize before concurrent use.
 */
void reset_reason_capture(void);

/**
 * @brief Clear hardware reset flags without losing the first software snapshot.
 * @note A preceding bootloader must leave the flags intact for the application.
 */
void reset_reason_clear_flags(void);

/**
 * @brief Return the first RCC->RSR snapshot for this firmware image.
 * @return Captured raw RCC->RSR register value.
 * @note Performs the initial capture if not already done.
 */
uint32_t reset_reason_get_rsr(void);

/**
 * @brief Output the first RCC->RSR snapshot and all asserted known reset flags.
 * @note Requires ULOG to be initialized; invoke in application startup logging,
 *       not during the early board-level capture/clear initialization.
 */
void reset_reason_log(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BSP_BOARD_RESET_REASON_H */
