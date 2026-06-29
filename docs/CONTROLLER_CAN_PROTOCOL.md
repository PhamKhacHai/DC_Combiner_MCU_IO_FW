# Báo cáo kỹ thuật firmware DC Combiner MCU I/O FW

## Mục lục

1. [Tổng quan dự án](#1-tổng-quan-dự-án)
2. [Các chức năng firmware đã implement](#2-các-chức-năng-firmware-đã-implement)
3. [Cấu hình chính trong `config.h`](#3-cấu-hình-chính-trong-configh)
4. [CAN protocol cho Controller](#4-can-protocol-cho-controller)
5. [Luồng hoạt động cho Controller](#5-luồng-hoạt-động-cho-controller)
6. [Test cases đã thực hiện và kết quả](#6-test-cases-đã-thực-hiện-và-kết-quả)
7. [Kết quả đạt được](#7-kết-quả-đạt-được)
8. [Phần còn lại và đề xuất phát triển tiếp](#8-phần-còn-lại-và-đề-xuất-phát-triển-tiếp)
9. [Điểm khác biệt so với document/spec ban đầu](#9-điểm-khác-biệt-so-với-documentspec-ban-đầu)
10. [Kết luận](#10-kết-luận)

---

## 1. Tổng quan dự án

Project `DC_Combiner_MCU_IO_FW` là firmware cho MCU I/O board dùng trong DC Combiner Box 6 group. Board dùng STM32F103, giao tiếp với Charger Controller qua CAN bus, điều khiển 6 output contactor group và đọc 12 feedback input.

Mỗi group contactor có 1 output điều khiển chung và 2 feedback input:

```text
Group x output   -> OUT_x
Group x feedback -> FBx_P và FBx_N
```

Vai trò của MCU I/O board:

- Nhận command ON/OFF group từ Charger Controller qua CAN.
- Điều khiển 6 output `OUT_1..OUT_6`.
- Đọc 12 feedback input `FB1_P..FB6_N`.
- Debounce feedback và phân loại từng group là OFF, ON hoặc MISMATCH.
- Quản lý state machine contactor từng group.
- Phát hiện lỗi timeout, mismatch, stuck contact, unexpected OFF.
- Gửi status frame `0x510 + NodeID`.
- Gửi heartbeat/diagnostic frame `0x520 + NodeID`.
- Khi group fault: output của group bị fault được đưa về OFF.
- Khi Lost CAN fail-safe: toàn bộ output được đưa về OFF.

MCU/CubeMX hiện tại:

| Hạng mục | Giá trị hiện tại |
| --- | --- |
| MCU | STM32F103C8Tx / STM32F103C8T6 |
| Package | LQFP48 |
| Clock | HSE 8 MHz, PLL x9, SYSCLK 72 MHz |
| CAN | CAN1 trên PA11/PA12 |
| CAN bitrate | 500 kbps |
| CAN mode | Normal |
| CAN timing | Prescaler 4, BS1 13TQ, BS2 4TQ, SJW 1TQ |

---

## 2. Các chức năng firmware đã implement

### 2.1 BSP GPIO

Module:

```text
Core/Inc/bsp_gpio.h
Core/Src/bsp_gpio.c
```

Chức năng đã có:

- Điều khiển output `OUT_1..OUT_6`.
- Đọc feedback input `FB1_P/FB1_N .. FB6_P/FB6_N`.
- Đọc DIP switch NodeID.
- Quản lý output command mask.
- Có `BSP_GPIO_AllOutputsOff()` để tắt toàn bộ output.
- Dùng macro CubeMX trong `main.h`, không tự định nghĩa chân bằng số riêng.

Output logic hiện tại:

```c
#define CONFIG_OUTPUT_ACTIVE_HIGH (1U)
```

Nghĩa là:

- Command ON -> GPIO SET.
- Command OFF -> GPIO RESET.
- CubeMX init output default RESET.

#### Mapping output

| Group | Output net | MCU pin | Logic |
| --- | --- | --- | --- |
| Group 1 | OUT_1 | PB5 | Active HIGH |
| Group 2 | OUT_2 | PB3 | Active HIGH |
| Group 3 | OUT_3 | PA10 | Active HIGH |
| Group 4 | OUT_4 | PA8 | Active HIGH |
| Group 5 | OUT_5 | PB14 | Active HIGH |
| Group 6 | OUT_6 | PB12 | Active HIGH |

#### Mapping feedback input

Firmware đọc feedback theo group và side `CONFIG_FEEDBACK_POSITIVE` / `CONFIG_FEEDBACK_NEGATIVE`.

| Group | Feedback P | MCU pin | Feedback N | MCU pin |
| --- | --- | --- | --- | --- |
| Group 1 | FB1_P | PB6 | FB1_N | PB7 |
| Group 2 | FB2_P | PB4 | FB2_N | PB8 |
| Group 3 | FB3_P | PA15 | FB3_N | PB9 |
| Group 4 | FB4_P | PA9 | FB4_N | PC13 |
| Group 5 | FB5_P | PB15 | FB5_N | PC14 |
| Group 6 | FB6_P | PB13 | FB6_N | PC15 |

Feedback logic hiện tại:

```c
#define CONFIG_FEEDBACK_ACTIVE_LOW (1U)
```

Feedback active low đã được xác nhận qua board test:

- Pin feedback kéo xuống LOW/PGND/GND -> firmware đọc feedback ON.
- Pin feedback ở mức HIGH/open theo pull-up -> firmware đọc feedback OFF.
- Không cấp trực tiếp +24V vào chân feedback.

#### Mapping raw feedback bit

`BSP_GPIO_ReadFeedbackMask()` trả mask 12 bit theo thứ tự:

| Bit | Ý nghĩa |
| --- | --- |
| bit0 | FB1_P |
| bit1 | FB1_N |
| bit2 | FB2_P |
| bit3 | FB2_N |
| bit4 | FB3_P |
| bit5 | FB3_N |
| bit6 | FB4_P |
| bit7 | FB4_N |
| bit8 | FB5_P |
| bit9 | FB5_N |
| bit10 | FB6_P |
| bit11 | FB6_N |

#### Mapping DIP NodeID

| DIP | MCU pin | Ý nghĩa |
| --- | --- | --- |
| DIP1 | PA5 | NodeID bit0 |
| DIP2 | PA6 | NodeID bit1 |
| DIP3 | PA7 | NodeID bit2 |

DIP logic hiện tại:

```c
#define CONFIG_DIP_ACTIVE_LOW (1U)
```

Firmware đọc NodeID bằng cách đảo logic theo active low và mask với `CONFIG_CAN_NODE_ID_MAX = 7`. NodeID được dùng để tính CAN ID command/status/heartbeat.

---

### 2.2 Feedback debounce

Module:

```text
Core/Inc/feedback.h
Core/Src/feedback.c
```

Chức năng:

- `Feedback_Init(now_ms)` đọc raw feedback mask một lần và set stable = raw ban đầu.
- `Feedback_Update(now_ms)` đọc raw feedback mask một lần mỗi chu kỳ update.
- Từ raw mask, firmware derive trạng thái từng group.
- Có debounce theo `CONFIG_FEEDBACK_DEBOUNCE_MS`.
- API cho contactor dùng trả về trạng thái stable/debounced.
- `Feedback_GetRawMask()` trả raw mask tức thời để phục vụ status/diagnostic.

Phân loại feedback từng group:

| FBx_P | FBx_N | Trạng thái group |
| --- | --- | --- |
| OFF | OFF | `FEEDBACK_GROUP_OFF` |
| ON | ON | `FEEDBACK_GROUP_ON` |
| ON | OFF | `FEEDBACK_GROUP_MISMATCH` |
| OFF | ON | `FEEDBACK_GROUP_MISMATCH` |

Các mask stable:

- `Feedback_GetOnMask()`
- `Feedback_GetOffMask()`
- `Feedback_GetMismatchMask()`

Vì sao cần debounce:

- Feedback contactor có thể rung/glitch khi tiếp điểm chuyển trạng thái.
- Contactor state machine không nên xử lý ngay một mẫu nhiễu ngắn.
- Debounce nằm trong `feedback.c`, nên `contactor.c` dùng tín hiệu đã stable.

---

### 2.3 Contactor state machine

Module:

```text
Core/Inc/contactor.h
Core/Src/contactor.c
```

State hiện tại:

| State | Ý nghĩa |
| --- | --- |
| `CONTACTOR_STATE_OFF` | Output OFF, feedback expected OFF |
| `CONTACTOR_STATE_ON_REQUESTED` | Đã bật output, đang chờ feedback ON |
| `CONTACTOR_STATE_ON_CONFIRMED` | Feedback ON đủ, contactor đã đóng |
| `CONTACTOR_STATE_OFF_REQUESTED` | Đã tắt output, đang chờ feedback OFF |
| `CONTACTOR_STATE_FAULT` | Group đang fault, output bị tắt |

Fault type hiện tại:

| Fault | Ý nghĩa |
| --- | --- |
| `CONTACTOR_FAULT_NONE` | Không fault |
| `CONTACTOR_FAULT_ON_TIMEOUT` | Bật output nhưng feedback không ON trước timeout |
| `CONTACTOR_FAULT_OFF_TIMEOUT` | Tắt output nhưng feedback không OFF trước timeout |
| `CONTACTOR_FAULT_FEEDBACK_MISMATCH` | FBx_P và FBx_N lệch nhau |
| `CONTACTOR_FAULT_STUCK_ON` | Output OFF nhưng feedback vẫn ON |
| `CONTACTOR_FAULT_UNEXPECTED_OFF` | Đang ON_CONFIRMED nhưng feedback chuyển OFF |

Rule an toàn:

- Khi fault, `Contactor_SetFault()` tắt output group đó.
- Khi state FAULT, `Contactor_HandleFault()` tiếp tục giữ output OFF.
- `Contactor_RequestOn()` bị reject nếu group đang FAULT hoặc OFF_REQUESTED.
- `Contactor_SetCommandMask()` xử lý OFF trước ON.
- `Contactor_AllOff()` gọi request OFF cho toàn bộ group.
- `Contactor_ClearFault()` chỉ clear fault nếu feedback stable đang OFF.
- Nếu feedback vẫn ON hoặc MISMATCH khi clear fault, group vẫn FAULT và cập nhật fault tương ứng.

---

### 2.4 CAN communication

Module:

```text
Core/Inc/can_comm.h
Core/Src/can_comm.c
```

Chức năng:

- Đọc NodeID từ DIP switch.
- Config CAN filter exact Standard ID `0x500 + NodeID`.
- Start CAN và enable RX FIFO0 notification.
- Nhận command frame từ Controller.
- Apply command trong `CanComm_Task()`.
- Gửi status frame định kỳ.
- Gửi heartbeat/diagnostic frame định kỳ.
- Xử lý Clear Fault qua CAN.
- Xử lý Lost CAN timeout fail-safe.
- Chặn command ON khi fail-safe active.

CAN RX command chỉ được xem là hợp lệ khi:

- Standard ID.
- Data frame.
- `StdId == 0x500 + NodeID`.
- `DLC >= CONFIG_CAN_COMMAND_DLC`.
- `HAL_CAN_GetRxMessage()` trả OK.

Lưu ý:

- `CONFIG_CAN_COMMAND_DLC` hiện là `1U`, nên firmware chấp nhận command frame có DLC tối thiểu 1.
- Nếu frame có `DLC > CONFIG_CAN_COMMAND_FLAGS_INDEX`, firmware đọc thêm `Data[1]` làm control flags.
- Bên Controller nên gửi DLC 8 để thống nhất format và reserved bytes.

---

### 2.5 Diagnostic / Heartbeat

Module:

```text
Core/Inc/diagnostic.h
Core/Src/diagnostic.c
```

Chức năng:

- Build heartbeat payload 8 byte.
- Quản lý heartbeat sequence counter.
- `Diagnostic_Init()` reset sequence về 0.
- `Diagnostic_OnHeartbeatTxSuccess()` tăng sequence chỉ khi heartbeat TX thành công.
- Sequence là `uint8_t`, tự wrap từ `0xFF` về `0x00`.

Trách nhiệm module sau refactor:

| Module | Trách nhiệm |
| --- | --- |
| `can_comm.c` | CAN header, CAN ID, TX period, gọi HAL CAN |
| `diagnostic.c` | Build payload và quản lý heartbeat sequence |

Heartbeat không refresh Lost CAN timeout. Lost CAN chỉ refresh khi MCU nhận command frame hợp lệ từ Controller.

---

### 2.6 Fault manager hiện tại

Module:

```text
Core/Inc/fault.h
Core/Src/fault.c
```

Hiện trạng:

- `fault.h/.c` hiện là scaffold, chưa phải fault manager runtime chính.
- Group fault hiện do `contactor.c` quản lý.
- Lost CAN fail-safe và global status hiện do `can_comm.c` quản lý.
- Chưa refactor fault manager lớn để tránh ảnh hưởng behavior đã test.

Định hướng nếu mở rộng sau này:

- Có thể dùng `fault.c/h` làm helper/status summary.
- Không nên chuyển state machine/fault handling ra khỏi `contactor.c` khi chưa có yêu cầu rõ ràng.

---

### 2.7 App runtime

Module:

```text
Core/Inc/app.h
Core/Src/app.c
```

Init order:

```text
Feedback_Init(now_ms)
Contactor_Init(now_ms)
CanComm_Init(now_ms)
```

Task order:

```text
Feedback_Update(now_ms)
CanComm_Task(now_ms)
Contactor_Task(now_ms)
```

`App_Task()` chạy theo `CONFIG_APP_TASK_PERIOD_MS` và dùng phép trừ `uint32_t` để chống tràn tick.

Ghi chú runtime:

- `CanComm_Task()` gửi status/heartbeat trước khi `Contactor_Task()` xử lý state machine trong cùng một vòng `App_Task()`.
- Vì vậy một fault vừa phát sinh từ feedback stable có thể được phản ánh ở status frame sau chu kỳ task tiếp theo.

---

## 3. Cấu hình chính trong `config.h`

| Macro | Giá trị hiện tại | Ý nghĩa |
| --- | ---: | --- |
| `CONFIG_GROUP_COUNT` | 6 | Số group contactor |
| `CONFIG_FEEDBACK_PER_GROUP` | 2 | Mỗi group có feedback P/N |
| `CONFIG_FEEDBACK_COUNT` | 12 | Tổng feedback input |
| `CONFIG_DIP_SWITCH_COUNT` | 3 | NodeID 0..7 |
| `CONFIG_OUTPUT_ACTIVE_HIGH` | 1 | Output ON = GPIO SET |
| `CONFIG_OUTPUT_DEFAULT_ON_MASK` | 0x00 | Default output OFF |
| `CONFIG_FEEDBACK_ACTIVE_LOW` | 1 | Feedback ON khi pin LOW |
| `CONFIG_DIP_ACTIVE_LOW` | 1 | DIP active LOW |
| `CONFIG_APP_TASK_PERIOD_MS` | 10 ms | Chu kỳ app task |
| `CONFIG_FEEDBACK_DEBOUNCE_MS` | 30 ms | Debounce feedback |
| `CONFIG_CONTACTOR_STABLE_FAULT_DELAY_MS` | 30 ms | Delay chống glitch ở state ổn định |
| `CONFIG_CONTACTOR_ON_TIMEOUT_MS` | 1000 ms | Timeout chờ feedback ON |
| `CONFIG_CONTACTOR_OFF_TIMEOUT_MS` | 1000 ms | Timeout chờ feedback OFF |
| `CONFIG_STATUS_PERIOD_MS` | 100 ms | Chu kỳ status frame |
| `CONFIG_HEARTBEAT_PERIOD_MS` | 1000 ms | Chu kỳ heartbeat base |
| `CONFIG_CAN_COMMAND_TIMEOUT_MS` | 3000 ms | Lost CAN timeout |
| `CONFIG_CAN_BITRATE` | 500000 | CAN 500 kbps |
| `CONFIG_CAN_COMMAND_ID_BASE` | 0x500 | Base ID command |
| `CONFIG_CAN_STATUS_ID_BASE` | 0x510 | Base ID status |
| `CONFIG_CAN_HEARTBEAT_ID_BASE` | 0x520 | Base ID heartbeat |
| `CONFIG_CAN_COMMAND_DLC` | 1 | Minimum DLC command firmware chấp nhận |
| `CONFIG_CAN_STATUS_DLC` | 8 | DLC status |
| `CONFIG_CAN_HEARTBEAT_DLC` | 8 | DLC heartbeat |
| `CONFIG_CAN_HEARTBEAT_ENABLE` | 1 | Enable heartbeat |
| `CONFIG_CAN_HEARTBEAT_PERIOD_MS` | 1000 ms | Chu kỳ heartbeat thực tế |
| `CONFIG_CAN_CLEAR_FAULT_ENABLE` | 1 | Enable Clear Fault qua CAN |
| `CONFIG_FW_VERSION_MAJOR` | 1 | Firmware version major |
| `CONFIG_FW_VERSION_MINOR` | 0 | Firmware version minor |
| `CONFIG_FW_VERSION_PATCH` | 0 | Firmware version patch |

Global status bit:

| Bit | Macro | Ý nghĩa |
| --- | --- | --- |
| bit0 | `CONFIG_STATUS_CAN_ONLINE_MASK` | CAN runtime started |
| bit1 | `CONFIG_STATUS_ANY_FAULT_MASK` | Có group fault hoặc fail-safe active |
| bit2 | `CONFIG_STATUS_FAILSAFE_ACTIVE_MASK` | Fail-safe active, ví dụ Lost CAN |

---

## 4. CAN protocol cho Controller

### 4.1 CAN ID

Firmware dùng Standard ID, Data frame.

| Frame type | Direction | CAN ID |
| --- | --- | --- |
| Command | Controller -> MCU | `0x500 + NodeID` |
| Status | MCU -> Controller | `0x510 + NodeID` |
| Heartbeat / Diagnostic | MCU -> Controller | `0x520 + NodeID` |

Ví dụ:

| NodeID | Command | Status | Heartbeat |
| ---: | ---: | ---: | ---: |
| 0 | 0x500 | 0x510 | 0x520 |
| 1 | 0x501 | 0x511 | 0x521 |
| 7 | 0x507 | 0x517 | 0x527 |

---

### 4.2 Command frame Controller gửi xuống MCU

Frame khuyến nghị:

```text
CAN ID = 0x500 + NodeID
DLC    = 8
Type   = Standard Data Frame
```

Payload:

| Byte | Ý nghĩa |
| --- | --- |
| `Data[0]` | Output command mask |
| `Data[1]` | Control flags |
| `Data[2..7]` | Reserved, gửi 0 |

`Data[0]` output command mask:

| Bit | Ý nghĩa |
| --- | --- |
| bit0 | Group 1 command |
| bit1 | Group 2 command |
| bit2 | Group 3 command |
| bit3 | Group 4 command |
| bit4 | Group 5 command |
| bit5 | Group 6 command |
| bit6..7 | ignored/masked |

Ví dụ command NodeID 0:

| Data | Ý nghĩa |
| --- | --- |
| `00 00 00 00 00 00 00 00` | OFF all |
| `01 00 00 00 00 00 00 00` | ON Group 1 |
| `02 00 00 00 00 00 00 00` | ON Group 2 |
| `04 00 00 00 00 00 00 00` | ON Group 3 |
| `08 00 00 00 00 00 00 00` | ON Group 4 |
| `10 00 00 00 00 00 00 00` | ON Group 5 |
| `20 00 00 00 00 00 00 00` | ON Group 6 |
| `3F 00 00 00 00 00 00 00` | ON Group 1..6 |

`Data[1]` control flags:

| Bit | Ý nghĩa |
| --- | --- |
| bit0 | Clear Fault request |
| bit1..7 | Reserved |

Clear Fault:

```text
CAN ID = 0x500 + NodeID
Data   = 00 01 00 00 00 00 00 00
```

Rule an toàn:

- Clear Fault chỉ hợp lệ khi `Data[0] == 0x00`.
- Không được gửi ON command kèm Clear Fault cùng frame.
- Ví dụ `01 01 00 00 00 00 00 00` là command không hợp lệ theo rule hiện tại: firmware không clear fault, không bật output, và đưa output về OFF theo hướng fail-safe. Controller không được gửi ON và Clear Fault trong cùng một frame.

---

### 4.3 Status frame MCU gửi lên Controller

Frame:

```text
CAN ID = 0x510 + NodeID
DLC    = 8
Type   = Standard Data Frame
Period = CONFIG_STATUS_PERIOD_MS
```

Payload:

| Byte | Ý nghĩa |
| --- | --- |
| `Data[0]` | Output mask hiện tại |
| `Data[1]` | Feedback raw mask low byte |
| `Data[2]` | Feedback raw mask high byte |
| `Data[3]` | Group fault mask |
| `Data[4]` | Global status |
| `Data[5]` | Reserved, hiện gửi 0 |
| `Data[6]` | Reserved, hiện gửi 0 |
| `Data[7]` | Reserved, hiện gửi 0 |

`Data[0]` output mask:

| Bit | Ý nghĩa |
| --- | --- |
| bit0 | OUT Group 1 |
| bit1 | OUT Group 2 |
| bit2 | OUT Group 3 |
| bit3 | OUT Group 4 |
| bit4 | OUT Group 5 |
| bit5 | OUT Group 6 |

`Data[1]/Data[2]` feedback raw mask:

| Status byte/bit | Feedback |
| --- | --- |
| `Data[1] bit0` | FB1_P |
| `Data[1] bit1` | FB1_N |
| `Data[1] bit2` | FB2_P |
| `Data[1] bit3` | FB2_N |
| `Data[1] bit4` | FB3_P |
| `Data[1] bit5` | FB3_N |
| `Data[1] bit6` | FB4_P |
| `Data[1] bit7` | FB4_N |
| `Data[2] bit0` | FB5_P |
| `Data[2] bit1` | FB5_N |
| `Data[2] bit2` | FB6_P |
| `Data[2] bit3` | FB6_N |

Expected feedback raw khi group có đủ P/N ON:

| Group feedback đủ | Data[1] | Data[2] |
| --- | ---: | ---: |
| Group 1 | 0x03 | 0x00 |
| Group 2 | 0x0C | 0x00 |
| Group 3 | 0x30 | 0x00 |
| Group 4 | 0xC0 | 0x00 |
| Group 5 | 0x00 | 0x03 |
| Group 6 | 0x00 | 0x0C |

`Data[3]` group fault mask:

| Bit | Giá trị | Ý nghĩa |
| --- | ---: | --- |
| bit0 | 0x01 | Fault Group 1 |
| bit1 | 0x02 | Fault Group 2 |
| bit2 | 0x04 | Fault Group 3 |
| bit3 | 0x08 | Fault Group 4 |
| bit4 | 0x10 | Fault Group 5 |
| bit5 | 0x20 | Fault Group 6 |

`Data[4]` global status:

| Bit | Giá trị | Ý nghĩa |
| --- | ---: | --- |
| bit0 | 0x01 | CAN online |
| bit1 | 0x02 | Any fault |
| bit2 | 0x04 | Fail-safe active / Lost CAN |

Giá trị phổ biến:

| Data[4] | Ý nghĩa |
| ---: | --- |
| 0x01 | CAN online, không fault |
| 0x03 | CAN online + có fault |
| 0x07 | CAN online + có fault + fail-safe active |

---

### 4.4 Heartbeat / Diagnostic frame MCU gửi lên Controller

Frame:

```text
CAN ID = 0x520 + NodeID
DLC    = 8
Type   = Standard Data Frame
Period = CONFIG_CAN_HEARTBEAT_PERIOD_MS
```

Payload:

| Byte | Ý nghĩa |
| --- | --- |
| `Data[0]` | Firmware major version |
| `Data[1]` | Firmware minor version |
| `Data[2]` | Firmware patch version |
| `Data[3]` | NodeID |
| `Data[4]` | Global status, cùng ý nghĩa với status `Data[4]` |
| `Data[5]` | Valid command RX counter low byte |
| `Data[6]` | CAN TX fail/error counter low byte |
| `Data[7]` | Heartbeat sequence counter |

Giải thích:

- `Data[7]` tăng mỗi lần heartbeat gửi thành công.
- `Data[7]` là `uint8_t`, nên `FF -> 00` là bình thường.
- `Data[5]` tăng khi MCU nhận command frame hợp lệ từ Controller.
- `Data[6]` là low byte của `tx_error_count`. Giá trị này có thể khác nhau theo runtime; Controller không nên expect cố định một giá trị cụ thể, chỉ cần theo dõi nếu counter tăng liên tục.
- `Data[4]` giống global status trong status frame.

Ví dụ:

```text
0x520 Data = 01 00 00 00 01 00 90 D0
```

Đọc là:

- Firmware `1.0.0`.
- NodeID `0`.
- Global status `0x01`: CAN online, không fault.
- Valid command RX counter low byte = `0x00`.
- TX fail/error counter low byte = `0x90` tại thời điểm log.
- Heartbeat sequence = `0xD0`.

Ví dụ NodeID 1:

```text
0x521 Data = 01 00 00 01 01 01 00 0C
```

Đọc là:

- Firmware `1.0.0`.
- NodeID `1`.
- Global status `0x01`.
- Đã nhận command hợp lệ, counter low byte = `0x01`.
- TX fail counter = `0x00` tại thời điểm log.
- Sequence = `0x0C`.

---

## 5. Luồng hoạt động cho Controller

### 5.1 Boot/reset

Expected:

- GPIO output được CubeMX init RESET.
- `Contactor_Init()` gọi `BSP_GPIO_AllOutputsOff()`.
- Nếu CAN init OK, status global có bit CAN online.

Ví dụ status bình thường:

```text
0x510 Data = 00 00 00 00 01 00 00 00
```

---

### 5.2 Bật 1 group thành công

Ví dụ NodeID 0, bật Group 1:

Controller gửi:

```text
0x500 Data = 01 00 00 00 00 00 00 00
```

Khi output ON và feedback G1 đủ:

```text
0x510 Data = 01 03 00 00 01 00 00 00
```

Giải thích:

- `Data[0] = 01`: OUT Group 1 ON.
- `Data[1] = 03`: FB1_P và FB1_N đều ON.
- `Data[3] = 00`: không group fault.
- `Data[4] = 01`: CAN online, không fault.

Ví dụ NodeID 1, bật Group 6:

Controller gửi:

```text
0x501 Data = 20 00 00 00 00 00 00 00
```

Khi output ON và feedback G6 đủ:

```text
0x511 Data = 20 00 0C 00 01 00 00 00
```

Giải thích:

- `Data[0] = 20`: OUT Group 6 ON.
- `Data[2] = 0C`: FB6_P và FB6_N đều ON.
- `Data[3] = 00`: không group fault.
- `Data[4] = 01`: CAN online, không fault.

---

### 5.3 Bật group nhưng không có feedback

Ví dụ NodeID 0, bật Group 1 nhưng feedback không ON:

```text
0x500 Data = 01 00 00 00 00 00 00 00
```

Trong giai đoạn chờ feedback:

```text
0x510 Data = 01 00 00 00 01 00 00 00
```

Sau `CONFIG_CONTACTOR_ON_TIMEOUT_MS`, contactor vào fault và output OFF:

```text
0x510 Data = 00 00 00 01 03 00 00 00
```

Giải thích:

- `Data[0] = 00`: output đã bị tắt do fault.
- `Data[3] = 01`: Fault Group 1.
- `Data[4] = 03`: CAN online + có fault.

---

### 5.4 Clear Fault

Controller gửi:

```text
0x500 Data = 00 01 00 00 00 00 00 00
```

Nếu feedback đã OFF và điều kiện an toàn đạt:

```text
0x510 Data = 00 00 00 00 01 00 00 00
```

Nếu feedback vẫn ON hoặc MISMATCH:

- Firmware không báo sạch giả.
- Group vẫn FAULT.
- Output vẫn OFF.

---

### 5.5 Lost CAN fail-safe

Controller không được gửi command ON một lần rồi im lặng. Nếu muốn giữ output ON, Controller phải gửi lại command định kỳ trước `CONFIG_CAN_COMMAND_TIMEOUT_MS`.

Ví dụ NodeID 1 bật Group 6, feedback đủ:

```text
0x501 Data = 20 00 00 00 00 00 00 00
0x511 Data = 20 00 0C 00 01 00 00 00
```

Nếu sau đó Controller ngừng gửi command quá timeout, firmware gọi `Contactor_AllOff()` và set fail-safe active:

```text
0x511 Data = 00 00 0C 00 07 00 00 00
```

Giải thích:

- `Data[0] = 00`: MCU đã tự tắt output.
- `Data[2] = 0C`: feedback G6 vẫn đang ON tại thời điểm status.
- `Data[4] = 07`: CAN online + any fault + fail-safe active.

Nếu feedback vẫn ON đủ lâu sau khi output OFF, contactor có thể phát hiện stuck ON:

```text
0x511 Data = 00 00 0C 20 07 00 00 00
```

Giải thích:

- `Data[3] = 20`: Fault Group 6.
- `Data[4] = 07`: fail-safe vẫn active.

---

### 5.6 Command định kỳ để không bị Lost CAN

Nếu muốn giữ Group 1 ON:

```text
Gửi định kỳ:
0x500 Data = 01 00 00 00 00 00 00 00
```

Chu kỳ gửi command phải nhỏ hơn `CONFIG_CAN_COMMAND_TIMEOUT_MS`. Với cấu hình hiện tại, timeout là 3000 ms; Controller nên gửi lại command mỗi 500 ms hoặc 1000 ms.

Expected:

```text
Status không chuyển sang Data[4] = 07.
```

---

## 6. Test cases đã thực hiện và kết quả

### 6.1 Tổng hợp PC/unit test

Các PC tests đã được chạy bằng MSYS2 UCRT64 + GCC + CMake + Ninja.

| Test harness | Mục tiêu | Kết quả |
| --- | --- | --- |
| `Feedback_TEST` | Debounce, raw mask mapping, mismatch, invalid group, tick overflow | 9 passed, 0 failed |
| `Contactor_TEST` | State machine, timeout, stuck ON, mismatch, clear fault, fail-safe guards | 36 passed, 0 failed |
| `App_TEST` | Init/task order, period, tick overflow | 6 passed, 0 failed |
| `Integration_TEST` | App + feedback + contactor integration | 10 passed, 0 failed |
| `CanComm_TEST` | CAN init/RX/TX/status/clear fault/lost CAN/heartbeat | 51 passed, 0 failed |
| `CanComm_TEST` clear disabled | `CONFIG_CAN_CLEAR_FAULT_ENABLE=0U` | 1 passed, 0 failed |
| `Diagnostic_TEST` | Heartbeat payload và sequence counter | 6 passed, 0 failed |

Firmware build:

| Command | Kết quả |
| --- | --- |
| `cmake --build build\Debug --clean-first` | Build OK |

Memory:

| Region | Used | Size | Used % |
| --- | ---: | ---: | ---: |
| RAM | 1888 B | 20 KB | 9.22% |
| FLASH | 15296 B | 64 KB | 23.34% |

---

### 6.2 Bảng testcase PC/unit test đại diện

| ID | Mục tiêu test | Thao tác | Expected result | Actual result | Kết quả |
| --- | --- | --- | --- | --- | --- |
| FB-01 | Init feedback OFF/ON/MISMATCH | Mock raw feedback rồi `Feedback_Init()` | Stable state đúng từng group | Pass trong `Feedback_TEST` | PASS |
| FB-02 | Mapping raw bit | Set raw mask group 1..6 | bit P/N đúng thứ tự bit0..bit11 | Pass trong `Feedback_TEST` | PASS |
| FB-03 | Glitch ngắn | Raw ON ngắn hơn debounce | Stable không đổi | Pass trong `Feedback_TEST` | PASS |
| FB-04 | Debounce đủ thời gian | Raw ON giữ đủ 30 ms | Stable chuyển ON | Pass trong `Feedback_TEST` | PASS |
| CT-01 | Request ON thành công | OFF -> request ON -> feedback ON | ON_CONFIRMED | Pass trong `Contactor_TEST` | PASS |
| CT-02 | ON timeout | Request ON nhưng feedback OFF | FAULT / ON_TIMEOUT, output OFF | Pass trong `Contactor_TEST` | PASS |
| CT-03 | OFF timeout / stuck ON | Request OFF nhưng feedback ON | FAULT / STUCK_ON | Pass trong `Contactor_TEST` | PASS |
| CT-04 | Mismatch | Feedback P/N lệch đủ delay | FAULT / FEEDBACK_MISMATCH | Pass trong `Contactor_TEST` | PASS |
| CT-05 | Clear fault an toàn | Clear khi feedback OFF | State OFF, fault NONE | Pass trong `Contactor_TEST` | PASS |
| CAN-01 | CAN init | Config filter/start/notification OK | `CanComm_IsStarted()` true | Pass trong `CanComm_TEST` | PASS |
| CAN-02 | Wrong ID ignored | Gửi ID không đúng | Không set pending command | Pass trong `CanComm_TEST` | PASS |
| CAN-03 | Clear Fault hợp lệ | `Data[0]=00`, `Data[1]=01` | Gọi clear fault, output OFF | Pass trong `CanComm_TEST` | PASS |
| CAN-04 | Clear Fault + ON bị chặn | `Data[0]=01`, `Data[1]=01` | Không clear, không ON, all-off | Pass trong `CanComm_TEST` | PASS |
| CAN-05 | Lost CAN | Có command hợp lệ rồi timeout | Fail-safe active, all-off | Pass trong `CanComm_TEST` | PASS |
| CAN-06 | ON bị block trong fail-safe | Đang fail-safe, gửi ON | Output vẫn OFF | Pass trong `CanComm_TEST` | PASS |
| CAN-07 | Thoát fail-safe | Gửi OFF all hoặc clear hợp lệ | Fail-safe clear, output OFF | Pass trong `CanComm_TEST` | PASS |
| HB-01 | Heartbeat ID | NodeID 0/1/7 | 0x520/0x521/0x527 | Pass trong `CanComm_TEST` | PASS |
| HB-02 | Heartbeat payload | Build payload | Version/NodeID/status/counter/seq đúng | Pass trong `CanComm_TEST` và `Diagnostic_TEST` | PASS |
| HB-03 | Sequence wrap | Tăng sequence qua 0xFF | Wrap về 0x00 | Pass trong `Diagnostic_TEST` | PASS |
| APP-01 | Init order | `App_Init()` | Feedback init trước contactor init, sau đó CAN init | Pass trong `App_TEST` | PASS |
| INT-01 | Integration ON | App + feedback + contactor | Request ON thành ON_CONFIRMED | Pass trong `Integration_TEST` | PASS |

---

### 6.3 Board test bằng USB-CAN-B

Board test đã thực hiện bằng USB-CAN-B với CAN 500 kbps, Standard Data Frame. Các testcase dưới đây đã pass trong quá trình bring-up.

| ID | Mục tiêu test | Thao tác | Expected result | Actual result representative | Kết quả |
| --- | --- | --- | --- | --- | --- |
| BD-01 | Boot NodeID 0 | Reset/power-up board NodeID 0 | Nhận status `0x510`, heartbeat `0x520` | `0x510 Data = 00 00 00 00 01 00 00 00` và heartbeat `0x520` xuất hiện định kỳ | PASS |
| BD-02 | Heartbeat sequence NodeID 0 | Quan sát `0x520` | `Data[7]` tăng dần và wrap `FF -> 00` | Sequence tăng `D0, D1, ... FF, 00, 01...` | PASS |
| BD-03 | Command counter | Gửi command hợp lệ `0x500` | Heartbeat `Data[5]` tăng | Sau command hợp lệ, heartbeat `Data[5]` tăng từ `00` lên `01` | PASS |
| BD-04 | NodeID 1 | Set DIP NodeID 1, reset board | Command `0x501`, status `0x511`, heartbeat `0x521` | Nhận `0x511` và `0x521`; heartbeat `Data[3] = 01` | PASS |
| BD-05 | G1 feedback mapping | ON G1 và kéo FB1_P/FB1_N | `0x510 Data = 01 03 00 00 01 00 00 00` | Feedback G1 đủ ra `Data[1] = 03` | PASS |
| BD-06 | G2 feedback mapping | ON G2 và kéo FB2_P/FB2_N | `0x510 Data = 02 0C 00 00 01 00 00 00` | Feedback G2 đủ ra `Data[1] = 0C` | PASS |
| BD-07 | G3 feedback mapping | ON G3 và kéo FB3_P/FB3_N | `0x510 Data = 04 30 00 00 01 00 00 00` | Feedback G3 đủ ra `Data[1] = 30` | PASS |
| BD-08 | G4 feedback mapping | ON G4 và kéo FB4_P/FB4_N | `0x510 Data = 08 C0 00 00 01 00 00 00` | Feedback G4 đủ ra `Data[1] = C0` | PASS |
| BD-09 | G5 feedback mapping | ON G5 và kéo FB5_P/FB5_N | `0x510 Data = 10 00 03 00 01 00 00 00` | Feedback G5 đủ ra `Data[2] = 03` | PASS |
| BD-10 | G6 feedback mapping | ON G6 và kéo FB6_P/FB6_N | `0x511 Data = 20 00 0C 00 01 00 00 00` với NodeID 1 | Feedback G6 đủ ra `Data[2] = 0C`, không fault | PASS |
| BD-11 | Lost CAN fail-safe | Gửi ON một lần rồi ngừng command quá timeout | Output OFF, global `Data[4] = 07` | Status chuyển về output `00`, global `07` | PASS |
| BD-12 | Stuck feedback sau Lost CAN | Giữ feedback ON sau khi output bị all-off | `Data[3]` có group fault tương ứng | Với G6: `0x511 Data = 00 00 0C 20 07 00 00 00` | PASS |
| BD-13 | Heartbeat global status khi fault/fail-safe | Quan sát heartbeat trong fault/fail-safe | Heartbeat `Data[4]` phản ánh global status | Heartbeat `Data[4]` hiển thị `01`, `03`, hoặc `07` theo trạng thái hệ thống | PASS |

---

### 6.4 Feedback mapping expected trên status

Các ví dụ dưới đây giả định output đã ON và feedback logical ON đủ P/N:

| Group | Status expected |
| --- | --- |
| G1 | `01 03 00 00 01 00 00 00` |
| G2 | `02 0C 00 00 01 00 00 00` |
| G3 | `04 30 00 00 01 00 00 00` |
| G4 | `08 C0 00 00 01 00 00 00` |
| G5 | `10 00 03 00 01 00 00 00` |
| G6 | `20 00 0C 00 01 00 00 00` |

---

### 6.5 Fault test expected

ON không có feedback:

```text
Expected Data[3] = group fault mask
Expected Data[4] = 03
```

Ví dụ G1:

```text
0x510 Data = 00 00 00 01 03 00 00 00
```

---

### 6.6 Clear Fault expected

Controller gửi:

```text
0x500 Data = 00 01 00 00 00 00 00 00
```

Nếu feedback OFF:

```text
0x510 Data = 00 00 00 00 01 00 00 00
```

Nếu feedback chưa safe:

- `Data[3]` vẫn còn group fault.
- `Data[4]` vẫn có any fault.
- Output vẫn OFF.

---

### 6.7 Lost CAN expected

Gửi ON một lần, sau đó ngừng command quá `CONFIG_CAN_COMMAND_TIMEOUT_MS`:

```text
Expected output OFF
Expected Data[4] = 07
```

Nếu feedback vẫn ON đủ lâu sau khi output OFF:

```text
Expected Data[3] có group fault stuck contact
```

---

### 6.8 Heartbeat/Diagnostic expected

Expected:

- `0x520 + NodeID` xuất hiện định kỳ.
- `Data[0..2] = 01 00 00`.
- `Data[3] = NodeID`.
- `Data[4] = global status`.
- `Data[5]` tăng khi nhận command hợp lệ.
- `Data[7]` tăng mỗi heartbeat TX thành công và wrap `FF -> 00`.

Ví dụ:

```text
0x520 Data = 01 00 00 00 01 00 90 D0
0x521 Data = 01 00 00 01 01 01 00 0C
```

---

## 7. Kết quả đạt được

Firmware hiện tại đã đạt được:

- Điều khiển được 6 group output qua CAN.
- Đọc được 12 feedback input.
- Có feedback debounce theo thời gian.
- Có state machine contactor từng group.
- Phát hiện lỗi ON timeout, OFF timeout, feedback mismatch, stuck contact, unexpected OFF.
- Có clear fault an toàn.
- Có Lost CAN timeout fail-safe.
- Chặn ON command khi fail-safe active.
- Có status frame `0x510 + NodeID`.
- Có heartbeat/diagnostic frame `0x520 + NodeID`.
- Có NodeID qua DIP switch.
- Đã test NodeID 0/1.
- Đã test feedback mapping G1..G6 trên board.
- Đã test Lost CAN và stuck feedback behavior.
- Có PC test cho feedback, contactor, app, integration, CAN, diagnostic.
- Firmware Debug build OK trên CMake/Ninja.

---

## 8. Phần còn lại và đề xuất phát triển tiếp

Những phần có thể phát triển trong phase sau:

- `fault.c/h` hiện chưa cần refactor lớn; có thể dùng làm helper/status summary sau.
- Có thể mở rộng diagnostic thêm last fault code.
- Có thể thêm fault counter, lost CAN counter, CAN error counter đầy đủ hơn.
- Có thể thêm contactor cycle counter để bảo trì.
- Có thể thêm uptime, reset reason.
- Có thể thêm bootloader/update firmware qua CAN/UART nếu có yêu cầu riêng.

---

## 9. Điểm khác biệt so với document/spec ban đầu

Document Word `MCU_IO_Module_Spec_Detailed R1.0.docx` là spec ban đầu. Firmware hiện tại có một số điểm đã cụ thể hóa hoặc khác với gợi ý ban đầu:

| Hạng mục | Spec ban đầu | Firmware hiện tại |
| --- | --- | --- |
| CAN bitrate | 250 hoặc 500 kbps | 500 kbps |
| DO | Đề xuất Option A 6 DO | Đã implement 6 DO group |
| Feedback | 12 DI | Đã implement 12 feedback input |
| Status feedback | Spec gợi ý `Data[1]` là bitmask DI 1 byte | Firmware dùng `Data[1]` low byte và `Data[2]` high byte cho 12 bit feedback |
| Status fault/global | Spec gợi ý `Data[2]` fault, `Data[3]` global | Firmware dùng `Data[3]` group fault, `Data[4]` global |
| Command Data[1] | Reserved/future | Firmware dùng `Data[1] bit0` làm Clear Fault request |
| Heartbeat payload | Spec gợi ý version/temp/error optional | Firmware dùng version, NodeID, global status, RX counter low byte, TX fail low byte, sequence |
| Timeout ON/OFF | Spec gợi ý 100-500 ms configurable | Firmware hiện dùng 1000 ms |
| Lost CAN timeout | Spec gợi ý 2-3 s | Firmware hiện dùng 3000 ms |
| Fault manager | Spec nói gửi lỗi/chẩn đoán | Group fault ở `contactor.c`, Lost CAN/global ở `can_comm.c`, `fault.c/h` chưa là manager chính |
| Bootloader | Spec yêu cầu/đề xuất update CAN/UART | Chưa implement trong core firmware hiện tại |
| Logging/counter bảo trì | Spec đề xuất counter đóng/ngắt/lỗi | Chưa implement đầy đủ; heartbeat có RX counter low byte và TX fail low byte |

Kết luận cho Controller:

- Không dùng layout status frame cũ trong spec Word.
- Phải code theo firmware hiện tại:

```text
Status:
Data[0] = output mask
Data[1] = feedback raw low byte
Data[2] = feedback raw high byte
Data[3] = group fault mask
Data[4] = global status
Data[5..7] = reserved
```

---

## 10. Kết luận

Firmware `DC_Combiner_MCU_IO_FW` hiện đã có đủ các module runtime chính cho MCU I/O board điều khiển DC Combiner Box 6 group:

- GPIO/BSP.
- Feedback debounce.
- Contactor state machine.
- CAN command/status.
- Clear Fault qua CAN.
- Lost CAN fail-safe.
- Heartbeat/Diagnostic.
- App runtime hook vào `main.c`.

Các PC unit/integration tests hiện tại đều pass, firmware Debug build OK. Board test bằng USB-CAN-B đã xác nhận CAN protocol, NodeID, feedback mapping, Lost CAN và heartbeat hoạt động đúng.

Phần Controller có thể bắt đầu tích hợp theo CAN protocol trong báo cáo này, đặc biệt cần lưu ý:

- Gửi command định kỳ nếu muốn giữ ON.
- Đọc status 2 byte feedback raw.
- Không dùng heartbeat để refresh Lost CAN.
- Clear Fault chỉ gửi khi command mask bằng `0x00`.
- Khi fail-safe active, không tiếp tục gửi ON bừa; cần OFF all hoặc clear fault đúng rule.
