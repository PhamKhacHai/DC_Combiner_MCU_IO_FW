# Current Status

## Da Hoan Thanh

* Da doc va doi chieu schematic/PCB voi project CubeMX.
* Da xac nhan MCU STM32F103C8Tx / STM32F103C8T6, package LQFP48.
* Da xac nhan GPIO output `OUT_1..OUT_6` khop schematic.
* Da xac nhan feedback input `FB1_P..FB6_N` khop schematic.
* Da xac nhan DIP switch `DIP1..DIP3` tren PA5/PA6/PA7.
* Da xac nhan CAN tren PA11/PA12, 500 kbps.
* Da xac nhan clock: HSE 8 MHz, PLL x9, SYSCLK 72 MHz, APB1 36 MHz.
* Da tao cac module scaffold rong trong `Core/Inc` va `Core/Src`.
* Da tao bo tai lieu context Markdown.
* Da viet `Core/Inc/config.h`: group count, polarity, timeout, CAN ID base, DLC, status indexes va helper tao CAN ID theo NodeID.
* Da build lai sau khi cap nhat `config.h`: build OK.
* Da them comment huong dan code cho `bsp_gpio.h/.c`, chua viet logic hoan chinh theo yeu cau user.
* Da sua comment trong `bsp_gpio.h/.c` sang tieng Viet khong dau, ghi ro tung viec user can tu code.
* Da code xong `bsp_gpio.h/.c`: API output, feedback, DIP NodeID, output mask va fail-safe all-off.
* Da them `Core/Src/bsp_gpio.c` vao CMake user sources.
* Da build lai sau khi them `bsp_gpio`: build OK.
* Da bo sung comment API ro rang trong `Core/Inc/bsp_gpio.h` de phan biet khai bao header va logic trong `.c`.
* Da code them public interface cho `bsp_gpio.h`: typedef BSP, C++ guard, `BSP_GPIO_IsGroupValid()` va prototype dung type BSP.
* Da dong bo chu ky ham trong `bsp_gpio.c` voi `bsp_gpio.h`, build OK.
* Da lam gon `bsp_gpio.h` thanh interface code ro rang, khong con comment dai che noi dung.
* Da chinh `.vscode/c_cpp_properties.json` sang `windows-gcc-arm` de giam loi IntelliSense sai do dang dung MSVC mode.
* Da build lai sau khi sua `bsp_gpio.h` va IntelliSense config: build OK.
* Da thay the hoan toan `Core/Inc/bsp_gpio.h` ban comment cu bang header code that gom typedef va prototype API; build OK.
* Da code xong `feedback.h/.c`: snapshot feedback, trang thai group OFF/ON/MISMATCH, raw mask 12 bit va mask group ON/OFF/MISMATCH.
* Da them `Core/Src/feedback.c` vao CMake user sources.
* Da build lai sau khi them `feedback`: build OK.
* Đã review và nâng cấp `feedback.h/.c`: `Feedback_Init/Update(now_ms)`, raw mask đọc một lần, debounce stable theo `CONFIG_FEEDBACK_DEBOUNCE_MS`.
* Đã build lại sau khi sửa `feedback`: build OK.
* Da code xong `contactor.h/.c`: state machine 6 group, request ON/OFF, set command mask, all off, clear fault, getter state/fault/mask.
* Da them `Core/Src/contactor.c` vao CMake user sources.
* Da build lai sau khi them `contactor`: build OK.
* Đã review và vá bug logic fail-safe trong `contactor.c`: chặn ON khi `OFF_REQUESTED`, giữ timeout OFF, clear fault không ảnh hưởng group bình thường.
* Đã build lại sau khi vá `contactor.c`: build OK.
* Đã kiểm tra tích hợp sau khi `feedback` đổi API sang `now_ms`: không có caller runtime kiểu cũ cần sửa.
* Đã xác nhận `bsp_gpio_feedback_mask_t` là `uint16_t`, đủ chứa raw feedback mask 12 bit.
* Đã tách `CONFIG_CONTACTOR_STABLE_FAULT_DELAY_MS` khỏi `CONFIG_FEEDBACK_DEBOUNCE_MS` và build OK.
* Đã code `app.h/.c` runtime tối thiểu: init feedback trước contactor, task feedback trước contactor.
* Đã hook `App_Init()` và `App_Task()` vào `main.c` trong vùng USER CODE.
* Đã thêm `Core/Src/app.c` vào root `CMakeLists.txt` và build OK.
* Đã tạo PC unit test harness bước 1 cho `feedback` tại `F:\Work\Project\MOCK_TEST\Feedback_TEST`.
* PC test harness hiện chưa chạy được trên máy này vì chưa có PC C compiler trong PATH; CMake mặc định chọn NMake nhưng không tìm thấy `nmake`.
* Đã tạo PC unit test harness cho `contactor` tại `F:\Work\Project\MOCK_TEST\Contactor_TEST`.
* Đã build và chạy `contactor` unit test bằng MSYS2 UCRT64 + GCC + CMake + Ninja: 36/36 testcase pass.
* Đã bổ sung các testcase fail-safe còn thiếu cho `Contactor_TEST` từ Test 27 đến Test 36.
* Đã tạo PC unit test harness cho `app` tại `F:\Work\Project\MOCK_TEST\App_TEST`.
* Đã build và chạy `app` unit test bằng MSYS2 UCRT64 + GCC + CMake + Ninja: 6/6 testcase pass.
* Đã tạo PC integration test harness tại `F:\Work\Project\MOCK_TEST\Integration_TEST`.
* Đã build và chạy integration test link thật `app.c` + `feedback.c` + `contactor.c`, mock HAL tick và BSP GPIO: 10/10 testcase pass.
* Đã kiểm tra firmware thật trước khi code CAN: `main.c` hook `App_Init()`/`App_Task()` đúng vùng USER CODE.
* Đã kiểm tra root `CMakeLists.txt` include đủ `app.c`, `bsp_gpio.c`, `feedback.c`, `contactor.c` và không include test files.
* Đã build firmware thật bằng `cmake --build build\Debug`: build OK.
* Đã code `can_comm.h/.c` tối thiểu: CAN filter exact Standard ID `0x500 + NodeID`, start CAN, RX FIFO0 notification, RX pending command và status TX định kỳ.
* Đã tích hợp `CanComm_Init()` và `CanComm_Task()` vào `app.c`.
* Đã thêm `Core/Src/can_comm.c` vào root `CMakeLists.txt` và build firmware thật OK.
* Đã tạo PC unit test harness cho `can_comm` tại `F:\Work\Project\MOCK_TEST\CanComm_TEST`.
* Đã build và chạy `can_comm` unit test bằng MSYS2 UCRT64 + GCC + CMake + Ninja: 23/23 testcase pass.
* Đã bổ sung các testcase RX/error guard còn thiếu cho `CanComm_TEST` từ Test 19 đến Test 23.
* Đã thêm chức năng Clear Fault qua CAN trong `can_comm`: `Data[0]` là output command mask, `Data[1] bit0` là clear fault request.
* Đã thêm macro protocol trong `config.h`: `CONFIG_CAN_COMMAND_MASK_INDEX`, `CONFIG_CAN_COMMAND_FLAGS_INDEX`, `CONFIG_CAN_COMMAND_CLEAR_FAULT_MASK`, `CONFIG_CAN_CLEAR_FAULT_ENABLE`.
* Clear Fault qua CAN chỉ hợp lệ khi `Data[0] == 0x00`; frame vừa ON vừa `CLEAR_FAULT` sẽ không clear fault, không bật output và được xử lý fail-safe all-off.
* Lệnh Clear Fault Node 0: CAN ID `0x500`, CANID bytes `00 00 05 00`, Data `00 01 00 00 00 00 00 00`.
* Đã cập nhật `CanComm_TEST`: test chính 28/28 pass, test macro disable `CONFIG_CAN_CLEAR_FAULT_ENABLE=0U` 1/1 pass.
* Đã cập nhật mock CAN cho `App_TEST` và `Integration_TEST` để các harness cũ build lại được sau khi `app.c` tích hợp `can_comm`.
* Đã chạy lại toàn bộ PC tests bằng MSYS2 UCRT64 + GCC + CMake + Ninja: `Feedback_TEST` 9/9, `Contactor_TEST` 36/36, `App_TEST` 6/6, `Integration_TEST` 10/10, `CanComm_TEST` 28/28 + disable 1/1 pass.
* Đã build firmware thật sau khi thêm Clear Fault qua CAN bằng `cmake --build build\Debug`: build OK.
* Đã implement Lost CAN timeout fail-safe trong `can_comm` dựa trên command frame hợp lệ `0x500 + NodeID`.
* Lost CAN dùng `CONFIG_CAN_COMMAND_TIMEOUT_MS`; controller muốn giữ output ON thì phải gửi command định kỳ trước timeout.
* Sau boot chưa từng nhận command thì không báo lost CAN ngay; chỉ khi đã từng nhận command hợp lệ rồi im lặng quá timeout mới fail-safe.
* Khi lost CAN timeout, firmware gọi `Contactor_AllOff(now_ms)`, set fail-safe active, không tự clear group fault và không tự bật lại output.
* Lost CAN là global fail-safe: `Data[3]` không set group fault riêng, `Data[4]` set `CAN_ONLINE | ANY_FAULT | FAILSAFE_ACTIVE`, tức `0x07` nếu chỉ có lost CAN.
* Khi đang fail-safe, command ON không được bật output và không tự thoát fail-safe; chỉ thoát bằng OFF all hoặc Clear Fault hợp lệ với `Data[0] == 0x00`.
* Đã cập nhật `CanComm_TEST`: test chính 37/37 pass, test macro disable `CONFIG_CAN_CLEAR_FAULT_ENABLE=0U` 1/1 pass.
* Đã chạy lại toàn bộ PC tests sau Lost CAN: `Feedback_TEST` 9/9, `Contactor_TEST` 36/36, `App_TEST` 6/6, `Integration_TEST` 10/10, `CanComm_TEST` 37/37 + disable 1/1 pass.
* Đã build firmware thật sau khi thêm Lost CAN bằng `cmake --build build\Debug`: build OK, RAM 1880 B, FLASH 14784 B.
* Da implement Heartbeat/Diagnostic CAN frame `0x520 + NodeID` trong `can_comm`.
* Heartbeat gui dinh ky theo `CONFIG_CAN_HEARTBEAT_PERIOD_MS`, enable bang `CONFIG_CAN_HEARTBEAT_ENABLE`.
* Heartbeat payload: FW version, NodeID, global status, valid command RX counter low byte, CAN TX fail counter low byte, sequence counter.
* Heartbeat khong refresh Lost CAN timeout; Lost CAN chi refresh boi command frame hop le `0x500 + NodeID` tu controller.
* Status frame `0x510 + NodeID` giu nguyen layout cu va chay song song voi heartbeat `0x520 + NodeID`.
* Da cap nhat `CanComm_TEST`: test chinh 51/51 pass, test macro disable `CONFIG_CAN_CLEAR_FAULT_ENABLE=0U` 1/1 pass.
* Da chay lai toan bo PC tests sau Heartbeat: `Feedback_TEST` 9/9, `Contactor_TEST` 36/36, `App_TEST` 6/6, `Integration_TEST` 10/10, `CanComm_TEST` 51/51 + disable 1/1 pass.
* Da build firmware that sau khi them Heartbeat bang `cmake --build build\Debug`: build OK, RAM 1880 B, FLASH 15104 B.
* Da refactor Heartbeat/Diagnostic frame `0x520 + NodeID`: tach heartbeat payload builder va sequence counter sang `diagnostic.h/.c`.
* `diagnostic.c` chi build payload va quan ly heartbeat sequence; khong goi HAL CAN, khong include `can.h`, khong doc GPIO va khong dieu khien output.
* `can_comm.c` van giu trach nhiem CAN ID/header, period TX va `HAL_CAN_AddTxMessage()`; protocol heartbeat/status/command khong doi.
* Da them `Core/Src/diagnostic.c` vao root `CMakeLists.txt`.
* Da tao PC unit test harness `Diagnostic_TEST`: 6/6 testcase pass.
* Da cap nhat `CanComm_TEST` de link that `diagnostic.c`: `test_can_comm.exe` 51/51 pass, `test_can_comm_clear_disabled.exe` 1/1 pass.
* Da chay lai toan bo PC tests sau khi refactor diagnostic: `Feedback_TEST` 9/9, `Contactor_TEST` 36/36, `App_TEST` 6/6, `Integration_TEST` 10/10, `CanComm_TEST` 51/51 + disable 1/1, `Diagnostic_TEST` 6/6 pass.
* Da build firmware that sau khi refactor diagnostic bang `cmake --build build\Debug`: build OK, RAM 1888 B, FLASH 15296 B.
* Da doc source hien tai, test harness va Word spec `MCU_IO_Module_Spec_Detailed R1.0.docx`, sau do tao bao cao ky thuat `DC_Combiner_MCU_IO_FW_Report.md`.
* Bao cao da ghi ro CAN protocol hien tai, mapping output/feedback, testcase pass, build RAM/FLASH va diem khac so voi spec ban dau.

## Dang Lam

* Quy uoc moi: sau moi task code/config, can cap nhat file Markdown trang thai truoc khi bao xong.
* Da co app runtime toi thieu de noi `feedback` va `contactor`.
* Module nen lam tiep theo: test board status/heartbeat/lost CAN voi USB-CAN-B, sau do xem xet fault manager rieng neu can.

## Van De Con Ton Tai

* Chua test board that voi USB-CAN-B.
* Chua test Clear Fault qua CAN tren board that voi USB-CAN-B.
* Chua test Lost CAN timeout tren board that voi USB-CAN-B.
* Chua test Heartbeat/Diagnostic frame tren board that voi USB-CAN-B.
* `Contactor_TEST` hiện chỉ là unit test mock cho `contactor`; chưa test app/CAN/integration trong bước này.
* `App_TEST` hiện chỉ là unit test mock cho `app`; chưa test CAN/integration/board thật trong bước này.
* `Integration_TEST` đã test chuỗi `app + feedback + contactor` trên PC, nhưng chưa test CAN hoặc board thật.
* Feedback co kha nang active LOW, can xac nhan bang board that.
* `feedback` da co debounce theo thoi gian; raw mask van la raw tuc thoi de phuc vu diagnostic/status.
* `contactor` dang dung feedback stable/debounced va dung `CONFIG_CONTACTOR_STABLE_FAULT_DELAY_MS` cho delay chong glitch rieng trong state on/off on dinh.
* App runtime toi thieu da goi `Feedback_Init(now_ms)`, `Contactor_Init(now_ms)`, `Feedback_Update(now_ms)` va `Contactor_Task(now_ms)`.
* DIP switch co pull-up ngoai va active LOW theo schematic/context, can test lai bang board that.
* Cac file source module moi chua duoc them vao CMake neu can build chung voi firmware.
* Neu IDE van bao loi cu trong `bsp_gpio`, can reload VS Code/CubeIDE language server de xoa cache index.

## Buoc Tiep Theo

* Test board voi USB-CAN-B de verify RX command va TX status thuc te.
* Test Clear Fault tren board that: gui Node 0 `CAN ID 0x500`, Data `00 01 00 00 00 00 00 00`, sau do doc status `0x510` va kiem tra `Data[0]=0x00`, `Data[3]=0x00`, `Data[4]=0x01` neu feedback dang safe/OFF.
* Test Lost CAN tren board that: gui ON dinh ky de giu output, dung gui command qua `CONFIG_CAN_COMMAND_TIMEOUT_MS`, kiem tra output OFF va status `Data[4]=0x07` neu khong co group fault khac.
* Test Heartbeat tren board that: Node 0 phai thay `0x520`, DLC 8, Data `01 00 00 00 01 xx xx seq` khi binh thuong va sequence tang moi heartbeat.
* Sau khi CAN runtime on dinh tren board, can xem xet them diagnostic/fault manager rieng neu can.

## Canh Bao Quan Trong

* Khong de OUT bat mac dinh khi reset/power-up.
* Khong sua code ngoai vung USER CODE neu file do do CubeMX sinh ra.
* Sau khi CubeMX generate lai, can kiem tra cac file custom co con duoc build khong.
* Neu them file `.c` moi vao build, can dam bao CMakeLists.txt include file do neu CMake khong tu nhan.
* Khong tu dinh nghia lai pin bang so rieng; dung macro CubeMX trong `main.h`.
