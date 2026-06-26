#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/* Board layout */
#define CONFIG_GROUP_COUNT (6U)
#define CONFIG_FEEDBACK_PER_GROUP (2U)
#define CONFIG_FEEDBACK_COUNT (CONFIG_GROUP_COUNT * CONFIG_FEEDBACK_PER_GROUP)
#define CONFIG_DIP_SWITCH_COUNT (3U)

/* GPIO logic */
#define CONFIG_OUTPUT_ACTIVE_HIGH (1U)
#define CONFIG_OUTPUT_DEFAULT_ON_MASK (0x00U)

/*
 * Feedback and DIP inputs have external pull-ups on the schematic.
 * Feedback polarity still needs final confirmation on real hardware.
 */
#define CONFIG_FEEDBACK_ACTIVE_LOW (1U)
#define CONFIG_DIP_ACTIVE_LOW (1U)

/* Bit masks for the six contactor groups */
#define CONFIG_GROUP_1_MASK (1U << 0)
#define CONFIG_GROUP_2_MASK (1U << 1)
#define CONFIG_GROUP_3_MASK (1U << 2)
#define CONFIG_GROUP_4_MASK (1U << 3)
#define CONFIG_GROUP_5_MASK (1U << 4)
#define CONFIG_GROUP_6_MASK (1U << 5)
#define CONFIG_GROUP_ALL_MASK (CONFIG_GROUP_1_MASK | CONFIG_GROUP_2_MASK | \
                               CONFIG_GROUP_3_MASK | CONFIG_GROUP_4_MASK | \
                               CONFIG_GROUP_5_MASK | CONFIG_GROUP_6_MASK)

/* Timing, in milliseconds */

#define CONFIG_APP_TASK_PERIOD_MS (10U)
#define CONFIG_FEEDBACK_DEBOUNCE_MS (30U)
#define CONFIG_CONTACTOR_STABLE_FAULT_DELAY_MS (30U)
#define CONFIG_CONTACTOR_ON_TIMEOUT_MS (1000U) // đúng là 1000 test 10000
#define CONFIG_CONTACTOR_OFF_TIMEOUT_MS (1000U)
#define CONFIG_HEARTBEAT_PERIOD_MS (1000U)
#define CONFIG_FW_VERSION_MAJOR (1U)
#define CONFIG_FW_VERSION_MINOR (0U)
#define CONFIG_FW_VERSION_PATCH (0U)
#define CONFIG_STATUS_PERIOD_MS (100U)        // đúng là 100 test 1000
#define CONFIG_CAN_COMMAND_TIMEOUT_MS (3000U) // đúng là 3000 test 12000

/* CAN bus */
#define CONFIG_CAN_BITRATE (500000U)
#define CONFIG_CAN_NODE_ID_MIN (0U)
#define CONFIG_CAN_NODE_ID_MAX ((1U << CONFIG_DIP_SWITCH_COUNT) - 1U)
#define CONFIG_CAN_COMMAND_ID_BASE (0x500U)
#define CONFIG_CAN_STATUS_ID_BASE (0x510U)
#define CONFIG_CAN_HEARTBEAT_ID_BASE (0x520U)
#define CONFIG_CAN_DIAGNOSTIC_ID_BASE CONFIG_CAN_HEARTBEAT_ID_BASE

#define CONFIG_CAN_COMMAND_DLC (1U)
#define CONFIG_CAN_STATUS_DLC (8U)
#define CONFIG_CAN_HEARTBEAT_DLC (8U)
#ifndef CONFIG_CAN_HEARTBEAT_ENABLE
#define CONFIG_CAN_HEARTBEAT_ENABLE (1U)
#endif

#ifndef CONFIG_CAN_HEARTBEAT_PERIOD_MS
#define CONFIG_CAN_HEARTBEAT_PERIOD_MS CONFIG_HEARTBEAT_PERIOD_MS
#endif
/* CAN command payload indexes and control flags */
#define CONFIG_CAN_COMMAND_MASK_INDEX (0U)
#define CONFIG_CAN_COMMAND_FLAGS_INDEX (1U)
#define CONFIG_CAN_COMMAND_CLEAR_FAULT_MASK (1U << 0)

#ifndef CONFIG_CAN_CLEAR_FAULT_ENABLE
#define CONFIG_CAN_CLEAR_FAULT_ENABLE (1U)
#endif

/* CAN status payload indexes */
#define CONFIG_CAN_STATUS_DO_MASK_INDEX (0U)
#define CONFIG_CAN_STATUS_FB_LOW_INDEX (1U)
#define CONFIG_CAN_STATUS_FB_HIGH_INDEX (2U)
#define CONFIG_CAN_STATUS_FAULT_MASK_INDEX (3U)
#define CONFIG_CAN_STATUS_GLOBAL_INDEX (4U)

/* Global status bits */
#define CONFIG_STATUS_CAN_ONLINE_MASK (1U << 0)
#define CONFIG_STATUS_ANY_FAULT_MASK (1U << 1)
#define CONFIG_STATUS_FAILSAFE_ACTIVE_MASK (1U << 2)

typedef enum
{
  CONFIG_GROUP_1 = 0,
  CONFIG_GROUP_2,
  CONFIG_GROUP_3,
  CONFIG_GROUP_4,
  CONFIG_GROUP_5,
  CONFIG_GROUP_6,
} config_group_t;

typedef enum
{
  CONFIG_FEEDBACK_POSITIVE = 0,
  CONFIG_FEEDBACK_NEGATIVE = 1,
} config_feedback_side_t;

static inline uint32_t Config_CanCommandId(uint8_t node_id)
{
  return CONFIG_CAN_COMMAND_ID_BASE + (uint32_t)(node_id & CONFIG_CAN_NODE_ID_MAX);
}

static inline uint32_t Config_CanStatusId(uint8_t node_id)
{
  return CONFIG_CAN_STATUS_ID_BASE + (uint32_t)(node_id & CONFIG_CAN_NODE_ID_MAX);
}

static inline uint32_t Config_CanHeartbeatId(uint8_t node_id)
{
  return CONFIG_CAN_HEARTBEAT_ID_BASE + (uint32_t)(node_id & CONFIG_CAN_NODE_ID_MAX);
}

#endif
