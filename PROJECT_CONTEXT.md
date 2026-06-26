# Project Context

## 1. Mục tiêu project

* Firmware cho MCU I/O module điều khiển DC Combiner Box 6 group.
* MCU nhận lệnh từ controller qua CAN.
* MCU điều khiển 6 output contactor.
* MCU đọc 12 feedback input.
* MCU quản lý state machine từng group.
* MCU phát hiện lỗi feedback mismatch, stuck contact, lost CAN.
* MCU gửi status, fault, heartbeat/diagnostic qua CAN.
* Fail-safe: reset, power-up, mất CAN, bootloader/update firmware thì tất cả output phải OFF.

## 2. Phần cứng chính

* MCU: STM32F103C8Tx / STM32F103C8T6.
* Package: LQFP48.
* Mạch nạp/debug: J-Link OB 072 qua SWD.
* SWDIO = PA13.
* SWDCLK = PA14.
* CAN dùng ISO1050 CAN transceiver.
* CAN_RX = PA11.
* CAN_TX = PA12.
* CAN bitrate hiện tại: 500 kbps.
* Clock: HSE 8 MHz, PLL x9, SYSCLK 72 MHz, APB1 36 MHz.

## 3. Digital Output mapping

| Group   | Net   | MCU pin | Logic       |
| ------- | ----- | ------- | ----------- |
| Group 1 | OUT_1 | PB5     | Active HIGH |
| Group 2 | OUT_2 | PB3     | Active HIGH |
| Group 3 | OUT_3 | PA10    | Active HIGH |
| Group 4 | OUT_4 | PA8     | Active HIGH |
| Group 5 | OUT_5 | PB14    | Active HIGH |
| Group 6 | OUT_6 | PB12    | Active HIGH |

Ghi chú:

* Mỗi OUT điều khiển đồng thời 2 contactor trong cùng group: TE+ và TE-.
* GPIO = 1 thì OUT ON.
* GPIO = 0 thì OUT OFF.
* Default output level phải là LOW.
* CubeMX cấu hình: GPIO Output Push Pull, No Pull, default Low.
* Lý do No Pull: mạch DO đã có pull-down ngoài ở đầu vào buffer/driver.

## 4. Digital Input feedback mapping

| Group   | Feedback TE+ | MCU pin | Feedback TE- | MCU pin |
| ------- | ------------ | ------- | ------------ | ------- |
| Group 1 | FB1_P        | PB6     | FB1_N        | PB7     |
| Group 2 | FB2_P        | PB4     | FB2_N        | PB8     |
| Group 3 | FB3_P        | PA15    | FB3_N        | PB9     |
| Group 4 | FB4_P        | PA9     | FB4_N        | PC13    |
| Group 5 | FB5_P        | PB15    | FB5_N        | PC14    |
| Group 6 | FB6_P        | PB13    | FB6_N        | PC15    |

Ghi chú:

* Feedback có khả năng là active LOW.
* MCU đọc 0 có thể hiểu là feedback ON.
* MCU đọc 1 có thể hiểu là feedback OFF.
* CẦN XÁC NHẬN: cần xác nhận lại polarity feedback bằng board thật nếu chưa test.
* CubeMX cấu hình: GPIO Input, No Pull.
* Lý do No Pull: schematic đã có pull-up ngoài.

## 5. DIP switch mapping

| DIP  | MCU pin | Ý nghĩa      |
| ---- | ------- | ------------ |
| DIP1 | PA5     | NodeID bit 0 |
| DIP2 | PA6     | NodeID bit 1 |
| DIP3 | PA7     | NodeID bit 2 |

Ghi chú:

* DIP có pull-up ngoài.
* DIP OFF -> MCU đọc 1.
* DIP ON -> MCU đọc 0.
* Khi code cần đảo logic để tính NodeID.

## 6. CAN protocol dự kiến


CAN frame summary:

* `0x500 + NodeID`: Command frame, Controller -> MCU.
* `0x510 + NodeID`: Status frame, MCU -> Controller.
* `0x520 + NodeID`: Heartbeat/Diagnostic frame, MCU -> Controller.
Command frame:

* ID = 0x500 + NodeID.
* Data[0] = output command mask cho 6 group.
* Data[0] bit0 = Group 1.
* Data[0] bit1 = Group 2.
* Data[0] bit2 = Group 3.
* Data[0] bit3 = Group 4.
* Data[0] bit4 = Group 5.
* Data[0] bit5 = Group 6.
* Data[1] = control flags.
* Data[1] bit0 = Clear Fault request.
* Clear Fault chỉ hợp lệ khi Data[0] == 0x00; nếu vừa ON vừa Clear Fault thì firmware không clear fault, không bật output và xử lý fail-safe all-off.
* Lệnh Clear Fault Node 0: CAN ID 0x500, CANID bytes `00 00 05 00`, Data `00 01 00 00 00 00 00 00`.
* Controller muốn giữ output ON thì phải gửi command frame định kỳ trước `CONFIG_CAN_COMMAND_TIMEOUT_MS`.
* Nếu MCU đã từng nhận command hợp lệ rồi quá timeout không nhận thêm command hợp lệ, MCU tự all-off và set Lost CAN fail-safe.
* Khi đang Lost CAN fail-safe, command ON không được bật output; chỉ OFF all hoặc Clear Fault hợp lệ với Data[0] == 0x00 mới thoát fail-safe.

Status frame:

* ID = 0x510 + NodeID.
* Data[0] = DO bitmask hiện tại.
* Data[1] = DI feedback low byte.
* Data[2] = DI feedback high byte.
* Data[3] = fault bitmask group 1..6.
* Data[4] = global status.
* Data[5..7] = reserved.
* Lost CAN là global fail-safe, không phải group fault riêng: Data[3] không set bit group nếu chỉ lost CAN.
* Nếu chỉ có lost CAN, Data[4] = 0x07: CAN online 0x01, any/global fault 0x02, fail-safe active 0x04.

Heartbeat/diagnostic frame:

* ID = 0x520 + NodeID.
* Frame type = Standard Data Frame.
* DLC = 8.
* Period = `CONFIG_CAN_HEARTBEAT_PERIOD_MS`.
* Data[0] = firmware major version.
* Data[1] = firmware minor version.
* Data[2] = firmware patch version.
* Data[3] = NodeID.
* Data[4] = global status, same meaning as status frame Data[4].
* Data[5] = valid command RX counter low byte.
* Data[6] = CAN TX fail/error counter low byte.
* Data[7] = heartbeat sequence counter.
* Heartbeat khong refresh Lost CAN timeout; Lost CAN chi refresh boi command frame hop le `0x500 + NodeID` tu controller.
* Gửi định kỳ, ví dụ 1000 ms.
* `diagnostic.h/.c` build payload heartbeat và quản lý heartbeat sequence.
* `can_comm.c` tạo CAN header, quản lý period TX và gọi `HAL_CAN_AddTxMessage()`.

## 7. State machine từng group

Mỗi group contactor cần có state:

* OFF.
* ON_REQUESTED.
* ON_CONFIRMED.
* OFF_REQUESTED.
* FAULT.

Luồng ON:

* Nhận request ON.
* Set OUT_x ON.
* State = ON_REQUESTED.
* Chờ cả TE+ và TE- feedback ON.
* Nếu đúng trong timeout -> ON_CONFIRMED.
* Nếu sai/timeout -> FAULT.

Luồng OFF:

* Nhận request OFF.
* Set OUT_x OFF.
* State = OFF_REQUESTED.
* Chờ cả TE+ và TE- feedback OFF.
* Nếu đúng trong timeout -> OFF.
* Nếu vẫn ON/timeout -> FAULT, stuck contact.

## 8. Cấu trúc firmware mong muốn

Core/Inc:

* app.h
* config.h
* bsp_gpio.h
* feedback.h
* contactor.h
* can_comm.h
* fault.h
* diagnostic.h

Core/Src:

* app.c
* bsp_gpio.c
* feedback.c
* contactor.c
* can_comm.c
* fault.c
* diagnostic.c

Ý nghĩa:

* main.c: điểm khởi động chương trình, gọi init và App_Task.
* app.c/h: điều phối toàn bộ hệ thống.
* bsp_gpio.c/h: thao tác GPIO theo board thật.
* feedback.c/h: đọc và debounce feedback.
* contactor.c/h: state machine 6 group.
* can_comm.c/h: nhận/gửi CAN, quản lý CAN ID/header/filter/runtime.
* fault.c/h: quản lý lỗi.
* diagnostic.c/h: build heartbeat payload, quản lý heartbeat sequence, counter/debug info không phụ thuộc HAL CAN.
* config.h: hằng số cấu hình, timeout, active LOW/HIGH, CAN ID base.

## 9. Trạng thái CubeMX hiện tại

* CubeMX đã cấu hình đúng pinout theo schematic/PCB đã kiểm tra.
* CAN1 dùng PA11/PA12, Normal mode, 500 kbps.
* CAN timing: Prescaler 4, BS1 13TQ, BS2 4TQ, SJW 1TQ.
* GPIO output mặc định được set RESET trong MX_GPIO_Init.
* JTAG disabled, SWD enabled, nên PB3/PB4/PA15 dùng được cho GPIO theo schematic.

## 10. Trạng thái module firmware đã code

* `bsp_gpio.h/.c`: đã có API điều khiển output, đọc feedback, đọc DIP NodeID, output mask và hàm fail-safe tắt toàn bộ output.
* `app.h/.c`: đã có runtime tối thiểu; `App_Init()` gọi `Feedback_Init(now_ms)` trước `Contactor_Init(now_ms)`, `App_Task()` gọi `Feedback_Update(now_ms)` trước `Contactor_Task(now_ms)` theo `CONFIG_APP_TASK_PERIOD_MS`.
* `feedback.h/.c`: đã có API init/update theo `now_ms`, đọc raw feedback mask 12 bit một lần mỗi update, debounce theo `CONFIG_FEEDBACK_DEBOUNCE_MS`, đọc trạng thái stable TE+/TE- từng group và group ON/OFF/MISMATCH mask.
* `feedback` tách raw tức thời cho diagnostic/status và stable/debounced cho API `Feedback_IsGroupOn/Off/Mismatch()`.
* `contactor.h/.c`: đã có state machine 6 group với state OFF, ON_REQUESTED, ON_CONFIRMED, OFF_REQUESTED, FAULT.
* `contactor` dùng `bsp_gpio` để điều khiển output và dùng `feedback` để xác nhận TE+/TE-; không đọc GPIO trực tiếp.
* `contactor` đã có fail-safe khi fault: tắt output, clear command_on, không tự retry, chỉ clear fault khi feedback đã OFF.
* `contactor` đã được vá các lỗi logic fail-safe: không cho ON khi đang OFF_REQUESTED, `RequestOff()` không reset timeout nếu đã OFF_REQUESTED, `ClearFault()/ClearAllFaults()` không tắt nhầm group bình thường.
* `contactor` dùng `CONFIG_CONTACTOR_STABLE_FAULT_DELAY_MS` làm delay chống glitch riêng trong trạng thái ổn định ON_CONFIRMED/OFF; macro này đã tách khỏi `CONFIG_FEEDBACK_DEBOUNCE_MS`.
* `can_comm.h/.c`: đã có CAN command/status tối thiểu, Clear Fault qua CAN, Lost CAN timeout fail-safe theo `CONFIG_CAN_COMMAND_TIMEOUT_MS`, status global báo fail-safe active, và heartbeat TX `0x520 + NodeID`.
* `diagnostic.h/.c`: đã tách heartbeat payload builder và sequence counter; module này không gọi HAL CAN, không include `can.h`, không đọc GPIO và không điều khiển output.
* `main.c` đã hook `App_Init()` và `App_Task()` trong vùng USER CODE; app runtime hiện gọi `Feedback`, `CanComm` và `Contactor`, chưa có fault manager riêng.
