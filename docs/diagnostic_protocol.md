# Diagnostic Counter Protocol

Tài liệu này mô tả phần diagnostic counter runtime của firmware
`DC_Combiner_MCU_IO_FW`.

## Phạm vi

- Counter chỉ lưu trong RAM runtime.
- Counter reset về 0 khi MCU reset hoặc mất nguồn.
- Chưa lưu Flash/EEPROM.
- Không thay đổi các frame cũ:
  - Command: `0x500 + NodeID`
  - Status: `0x510 + NodeID`
  - Heartbeat: `0x520 + NodeID`
- Diagnostic request không refresh Lost CAN timeout.
- Diagnostic request không bật/tắt output, không clear fault, không thoát fail-safe.

## CAN ID

| Frame | Direction | CAN ID | DLC |
| --- | --- | --- | --- |
| Diagnostic request | Controller -> MCU | `0x530 + NodeID` | minimum 3, khuyến nghị 8 |
| Diagnostic response | MCU -> Controller | `0x540 + NodeID` | 8 |

Tất cả đều là Standard Data Frame, không dùng Extended ID, không dùng RTR.

## Request payload

```text
Data[0] = command
Data[1] = counter_id hoặc magic0
Data[2] = group_index hoặc magic1
Data[3..7] = reserved, gửi 0
```

Command:

| Value | Macro | Ý nghĩa |
| ---: | --- | --- |
| `0x01` | `CONFIG_CAN_DIAG_COMMAND_READ_COUNTER` | Đọc 1 counter |
| `0x02` | `CONFIG_CAN_DIAG_COMMAND_RESET_ALL_COUNTERS` | Reset toàn bộ counter RAM |

Group index:

| Value | Ý nghĩa |
| ---: | --- |
| `0..5` | Group 1..6 |
| `0xFF` | Global counter |

## Response payload

Read counter response:

```text
Data[0] = 0x81
Data[1] = counter_id
Data[2] = group_index
Data[3] = value byte0, LSB
Data[4] = value byte1
Data[5] = value byte2
Data[6] = value byte3, MSB
Data[7] = status
```

Reset response:

```text
Data[0] = 0x82
Data[1] = 0x00
Data[2] = 0xFF
Data[3..6] = 0
Data[7] = status
```

Generic error response:

```text
Data[0] = 0x80
Data[1] = counter_id
Data[2] = group_index
Data[3] = command nhận được
Data[4..6] = 0
Data[7] = status
```

Status:

| Value | Macro | Ý nghĩa |
| ---: | --- | --- |
| `0x00` | `CONFIG_CAN_DIAG_STATUS_OK` | OK |
| `0x01` | `CONFIG_CAN_DIAG_STATUS_INVALID_COMMAND` | Command không hợp lệ |
| `0x02` | `CONFIG_CAN_DIAG_STATUS_INVALID_COUNTER_ID` | Counter ID không hợp lệ |
| `0x03` | `CONFIG_CAN_DIAG_STATUS_INVALID_GROUP` | Group index không hợp lệ |
| `0x04` | `CONFIG_CAN_DIAG_STATUS_BAD_DLC` | DLC request quá ngắn |
| `0x05` | `CONFIG_CAN_DIAG_STATUS_RESET_MAGIC_INVALID` | Reset magic không đúng |
| `0x06` | `CONFIG_CAN_DIAG_STATUS_INTERNAL_ERROR` | Lỗi nội bộ dự phòng |

## Counter ID

Group counters, dùng `group_index = 0..5`:

| Counter ID | Macro | Ý nghĩa |
| ---: | --- | --- |
| `0x01` | `CONFIG_CAN_DIAG_COUNTER_CONTACTOR_CLOSE` | Số lần feedback stable chuyển sang ON |
| `0x02` | `CONFIG_CAN_DIAG_COUNTER_CONTACTOR_OPEN` | Số lần feedback stable chuyển từ ON sang OFF |
| `0x03` | `CONFIG_CAN_DIAG_COUNTER_FEEDBACK_MISMATCH` | Số lần feedback stable vào MISMATCH |
| `0x04` | `CONFIG_CAN_DIAG_COUNTER_GROUP_FAULT` | Số lần group vào fault |
| `0x05` | `CONFIG_CAN_DIAG_COUNTER_ON_TIMEOUT_FAULT` | Số lần fault ON_TIMEOUT |
| `0x06` | `CONFIG_CAN_DIAG_COUNTER_UNEXPECTED_OFF_FAULT` | Số lần fault UNEXPECTED_OFF |
| `0x07` | `CONFIG_CAN_DIAG_COUNTER_FAULT_CLEAR` | Số lần clear fault thành công |

Global counters, dùng `group_index = 0xFF`:

| Counter ID | Macro | Ý nghĩa |
| ---: | --- | --- |
| `0x20` | `CONFIG_CAN_DIAG_COUNTER_CAN_COMMAND_RX` | Số command control hợp lệ nhận ở `0x500 + NodeID` |
| `0x21` | `CONFIG_CAN_DIAG_COUNTER_CAN_TX` | Số CAN frame firmware gửi thành công |
| `0x22` | `CONFIG_CAN_DIAG_COUNTER_CAN_RX_ERROR` | Số lỗi RX HAL CAN |
| `0x23` | `CONFIG_CAN_DIAG_COUNTER_CAN_TX_ERROR` | Số lỗi TX HAL CAN |
| `0x24` | `CONFIG_CAN_DIAG_COUNTER_CAN_TIMEOUT` | Số lần Lost CAN timeout |
| `0x25` | `CONFIG_CAN_DIAG_COUNTER_FAILSAFE_ENTER` | Số lần vào fail-safe |
| `0x26` | `CONFIG_CAN_DIAG_COUNTER_DIAG_REQUEST` | Số diagnostic request hợp lệ |
| `0x27` | `CONFIG_CAN_DIAG_COUNTER_DIAG_ERROR` | Số diagnostic request lỗi |

Counter là `uint32_t`, little-endian trong response và tăng bão hòa tại
`UINT32_MAX`, không wrap về 0.

## Reset counter

Reset toàn bộ counter RAM:

```text
CAN ID = 0x530 + NodeID
Data   = 02 A5 5A 00 00 00 00 00
```

Response OK:

```text
CAN ID = 0x540 + NodeID
Data   = 82 00 FF 00 00 00 00 00
```

Nếu magic không đúng, firmware không reset counter và trả:

```text
Data[0] = 0x82
Data[7] = 0x05
```

## Ví dụ đọc counter

Đọc số lần nhận command control hợp lệ của NodeID 0:

```text
Request:
CAN ID = 0x530
Data   = 01 20 FF 00 00 00 00 00

Response ví dụ value = 3:
CAN ID = 0x540
Data   = 81 20 FF 03 00 00 00 00
```

Đọc số lần Group 1 vào fault:

```text
Request:
CAN ID = 0x530
Data   = 01 04 00 00 00 00 00 00

Response ví dụ value = 1:
CAN ID = 0x540
Data   = 81 04 00 01 00 00 00 00
```

## Lưu ý an toàn

- Request `0x530 + NodeID` không được dùng để giữ output ON.
- Lost CAN chỉ refresh khi nhận command control hợp lệ ở `0x500 + NodeID`.
- Nếu hệ thống đang fail-safe, diagnostic request vẫn chỉ trả counter và không tự thoát fail-safe.
- Clear fault vẫn dùng command frame `0x500 + NodeID`, không dùng diagnostic frame.
