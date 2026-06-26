# Task Log

## 2026-06-23 13:12 - Khởi tạo tài liệu context project

### Mục tiêu

Tạo bộ tài liệu Markdown để lưu context, task log và trạng thái hiện tại của firmware STM32 cho MCU I/O module DC Combiner Box 6 group.

### Việc đã làm

* Tạo `PROJECT_CONTEXT.md`.
* Tạo `TASK_LOG.md`.
* Tạo `CURRENT_STATUS.md`.
* Ghi lại pin mapping GPIO, CAN, clock, cấu trúc module firmware và các quy tắc làm việc.

### Kết quả cuối cùng

* Ba file context đã được tạo ở thư mục gốc project.
* Nội dung phản ánh cấu hình đúng hiện tại đã xác nhận: pinout khớp schematic, CAN 500 kbps, SYSCLK 72 MHz, APB1 36 MHz.

### File liên quan

* `PROJECT_CONTEXT.md`
* `TASK_LOG.md`
* `CURRENT_STATUS.md`

### Ghi chú

* CẦN XÁC NHẬN: polarity feedback và DIP switch cần được xác nhận lại bằng board thật khi test phần cứng.

## 2026-06-23 - Tạo module scaffold rỗng

### Mục tiêu

Tạo các file module rỗng theo cấu trúc firmware mong muốn, chưa viết logic xử lý.

### Việc đã làm

* Tạo header trong `Core/Inc`: `app.h`, `config.h`, `bsp_gpio.h`, `feedback.h`, `contactor.h`, `can_comm.h`, `fault.h`, `diagnostic.h`.
* Tạo source trong `Core/Src`: `app.c`, `bsp_gpio.c`, `feedback.c`, `contactor.c`, `can_comm.c`, `fault.c`, `diagnostic.c`.
* Mỗi header chỉ có include guard rỗng.
* Mỗi source chỉ include header tương ứng.

### Kết quả cuối cùng

* Module scaffold đã tồn tại đúng tên file, đúng thư mục.
* Không thêm GPIO logic, state machine, CAN protocol, debounce hoặc fault handling.
* Không sửa file CubeMX sinh ra như `main.c`, `gpio.c`, `can.c`.
* Không sửa `CMakeLists.txt`.

### File liên quan

* `Core/Inc/app.h`
* `Core/Inc/config.h`
* `Core/Inc/bsp_gpio.h`
* `Core/Inc/feedback.h`
* `Core/Inc/contactor.h`
* `Core/Inc/can_comm.h`
* `Core/Inc/fault.h`
* `Core/Inc/diagnostic.h`
* `Core/Src/app.c`
* `Core/Src/bsp_gpio.c`
* `Core/Src/feedback.c`
* `Core/Src/contactor.c`
* `Core/Src/can_comm.c`
* `Core/Src/fault.c`
* `Core/Src/diagnostic.c`

### Ghi chú

* Đã từng có lỗi format header khi tạo bằng PowerShell và đã sửa lại thành include guard đúng nhiều dòng.

## 2026-06-23 - Sửa tài liệu Markdown sang tiếng Việt có dấu

### Mục tiêu

Chuyển nội dung 3 file context Markdown từ tiếng Việt không dấu sang tiếng Việt có dấu để dễ đọc và dễ bàn giao context.

### Việc đã làm

* Sửa `PROJECT_CONTEXT.md` sang tiếng Việt có dấu.
* Sửa `CURRENT_STATUS.md` sang tiếng Việt có dấu.
* Sửa `TASK_LOG.md` sang tiếng Việt có dấu.
* Giữ nguyên nội dung kỹ thuật, pin mapping, clock, CAN và trạng thái hiện tại.

### Kết quả cuối cùng

* Ba file Markdown hiện đã dùng tiếng Việt có dấu.
* Không thay đổi code firmware.
* Không thay đổi cấu hình CubeMX, GPIO, CAN, clock hoặc CMake.

### File liên quan

* `PROJECT_CONTEXT.md`
* `TASK_LOG.md`
* `CURRENT_STATUS.md`

### Ghi chú

* Từ các task sau, tiếp tục cập nhật tài liệu bằng tiếng Việt có dấu.

## 2026-06-23 16:40 - Code module feedback

### Mục tiêu

Code đầy đủ `feedback.h/.c` để đọc và lưu snapshot trạng thái feedback 12 input cho 6 group.

### Việc đã làm

* Code `Core/Inc/feedback.h` với enum trạng thái group, struct status và API public.
* Code `Core/Src/feedback.c` dùng `bsp_gpio` để đọc feedback đã đổi polarity.
* Thêm phân loại trạng thái từng group: `FEEDBACK_GROUP_OFF`, `FEEDBACK_GROUP_ON`, `FEEDBACK_GROUP_MISMATCH`.
* Thêm API lấy raw feedback mask 12 bit, group ON mask, group OFF mask và group mismatch mask.
* Thêm `Core/Src/feedback.c` vào `CMakeLists.txt`.
* Build lại project.

### Kết quả cuối cùng

* `feedback` đã build OK.
* Module hiện là snapshot logic raw, chưa có debounce theo thời gian.
* Không sửa file CubeMX sinh ra như `main.c`, `gpio.c`, `can.c`.

### File liên quan

* `Core/Inc/feedback.h`
* `Core/Src/feedback.c`
* `CMakeLists.txt`
* `PROJECT_CONTEXT.md`
* `CURRENT_STATUS.md`
* `TASK_LOG.md`

## 2026-06-24 08:21 - Phân tích luồng feedback

### Mục tiêu

Giải thích luồng xử lý hiện tại của `feedback.h/.c` và cách module này dùng `bsp_gpio`.

### Việc đã làm

* Đọc `Core/Inc/feedback.h`.
* Đọc `Core/Src/feedback.c`.
* Đối chiếu với `Core/Inc/bsp_gpio.h` và `Core/Src/bsp_gpio.c`.

### Kết quả cuối cùng

* Không sửa code.
* Xác định `feedback` hiện là lớp snapshot trạng thái feedback, chưa có debounce theo thời gian.

### File liên quan

* `Core/Inc/feedback.h`
* `Core/Src/feedback.c`
* `Core/Inc/bsp_gpio.h`
* `Core/Src/bsp_gpio.c`

### Ghi chú

* Nếu cần debounce, nên bổ sung ở bước sau trước khi dùng feedback để quyết định fault/stuck contact.

## 2026-06-24 09:39 - Phác thảo flow module contactor

### Mục tiêu

Nêu flow xử lý và danh sách hàm dự kiến cho module `contactor.h/.c`, chưa code.

### Việc đã làm

* Đọc lại `PROJECT_CONTEXT.md` và `CURRENT_STATUS.md`.
* Phác thảo state machine contactor cho 6 group.
* Xác định các API dự kiến cho init, command ON/OFF, update state, đọc trạng thái và fault.

### Kết quả cuối cùng

* Chưa sửa code.
* Flow đề xuất: command -> set output -> chờ feedback -> confirmed hoặc fault timeout/mismatch.

### File liên quan

* `PROJECT_CONTEXT.md`
* `CURRENT_STATUS.md`
* `TASK_LOG.md`

### Ghi chú

* Module `contactor` nên dùng `bsp_gpio` để điều khiển OUT và dùng `feedback` để xác nhận trạng thái TE+/TE-.

## 2026-06-24 10:06 - Review flow thiết kế module contactor

### Mục tiêu

Review flow thiết kế `contactor.h/.c` trước khi code.

### Việc đã làm

* Đọc yêu cầu review từ attachment.
* Đối chiếu với context project và trạng thái module `bsp_gpio`, `feedback`.
* Kiểm tra flow init, ON, OFF, ON_CONFIRMED, OFF, FAULT, clear fault, command mask, timeout và API.

### Kết quả cuối cùng

* Chưa sửa code.
* Flow tổng thể hợp lý nhưng cần chỉnh rõ fault enum, init an toàn, clear fault, debounce và mask/status API trước khi code.

### File liên quan

* `PROJECT_CONTEXT.md`
* `CURRENT_STATUS.md`
* `TASK_LOG.md`

### Ghi chú

* Khuyến nghị không fault mismatch ngay lập tức trong trạng thái chuyển mạch; nên chờ timeout hoặc debounce/stable window.

## 2026-06-24 10:25 - Code module contactor

### Mục tiêu

Code đầy đủ `contactor.h/.c` theo flow state machine đã chốt, ưu tiên fail-safe.

### Việc đã làm

* Code `Core/Inc/contactor.h` với state enum, fault enum và API public.
* Code `Core/Src/contactor.c` với state machine 6 group.
* Thêm flow init an toàn: tắt toàn bộ output, không set OFF mù nếu feedback chưa OFF.
* Thêm flow request ON/OFF, task update, all-off, command mask, clear fault và getter mask.
* Thêm xử lý timeout chống tràn tick bằng `(uint32_t)(now_ms - start_ms) >= timeout`.
* Thêm chống glitch trong state ổn định `ON_CONFIRMED` và `OFF` bằng `CONFIG_FEEDBACK_DEBOUNCE_MS`.
* Thêm `Core/Src/contactor.c` vào `CMakeLists.txt`.
* Build lại project.

### Kết quả cuối cùng

* Build OK.
* `contactor` không đọc GPIO trực tiếp; chỉ dùng `bsp_gpio` và `feedback`.
* Khi fault, output được tắt, `command_on` về false và không tự retry.
* `ClearFault()` chỉ clear thành công nếu feedback đã OFF.

### File liên quan

* `Core/Inc/contactor.h`
* `Core/Src/contactor.c`
* `CMakeLists.txt`
* `PROJECT_CONTEXT.md`
* `CURRENT_STATUS.md`
* `TASK_LOG.md`

### Ghi chú

* `contactor` hiện dùng snapshot feedback raw đã đổi polarity; debounce thật trong `feedback` vẫn là việc có thể bổ sung sau.

## 2026-06-24 11:00 - Sửa bug logic fail-safe module contactor

### Mục tiêu

Review các lỗi được nêu cho `contactor.h/.c`, xác nhận lỗi thật trong code hiện tại và chỉ sửa các điểm ảnh hưởng logic/fail-safe.

### Việc đã làm

* Xác nhận `Contactor_RequestOn()` đang cho ON khi group ở `OFF_REQUESTED`; đã sửa để chỉ cho ON từ `OFF`, còn `ON_REQUESTED/ON_CONFIRMED` là idempotent.
* Xác nhận `Contactor_SetCommandMask()` có thể bật group đang `OFF_REQUESTED`; đã sửa để phần ON chỉ gọi `RequestOn()` khi state là `OFF`.
* Xác nhận `Contactor_RequestOff()` reset `state_start_ms` khi gọi lặp trong `OFF_REQUESTED`; đã sửa để không reset timeout đang chạy.
* Xác nhận `Contactor_AllOff()` bị ảnh hưởng gián tiếp qua `RequestOff()`; đã xử lý bằng việc làm `RequestOff()` idempotent.
* Xác nhận `Contactor_ClearAllFaults()` và `Contactor_ClearFault()` có thể tắt nhầm group không fault; đã thêm guard để chỉ tác động group đang fault.
* Sửa `Contactor_GetFault()` cho group invalid trả `CONTACTOR_FAULT_NONE` thay vì gán nhầm thành feedback mismatch.
* Gia cố `Contactor_HandleFault()` để luôn ép state về `FAULT`, tắt output, clear command và gán fault mặc định nếu state lỗi chưa có fault.
* Build lại project.

### Kết quả cuối cùng

* Build OK bằng `cmake --build build\Debug`.
* Không đổi enum/API public trong `contactor.h`.
* Không sửa file CubeMX sinh ra.

### File liên quan

* `Core/Src/contactor.c`
* `PROJECT_CONTEXT.md`
* `CURRENT_STATUS.md`
* `TASK_LOG.md`

## 2026-06-24 12:15 - Code app runtime tối thiểu

### Mục tiêu

Code `app.h/.c` runtime tối thiểu để nối `feedback` và `contactor`, chưa thêm CAN, fault manager hoặc diagnostic.

### Việc đã làm

* Code `Core/Inc/app.h` với API `App_Init()` và `App_Task()`.
* Code `Core/Src/app.c`:
  * `App_Init()` lấy `now_ms = HAL_GetTick()`, gọi `Feedback_Init(now_ms)` trước `Contactor_Init(now_ms)`.
  * `App_Task()` gate theo `CONFIG_APP_TASK_PERIOD_MS`, gọi `Feedback_Update(now_ms)` trước `Contactor_Task(now_ms)`.
  * So sánh tick dùng `(uint32_t)(now_ms - app_last_task_ms)` để chống tràn tick.
* Hook `App_Init()` vào `main.c` sau `MX_GPIO_Init()` và `MX_CAN_Init()` trong vùng USER CODE 2.
* Hook `App_Task()` vào vòng `while (1)` trong vùng USER CODE 3.
* Include `app.h` trong vùng USER CODE Includes của `main.c`.
* Thêm `Core/Src/app.c` vào root `CMakeLists.txt`.
* Build lại project.

### Kết quả cuối cùng

* Build OK bằng `cmake --build build\Debug`.
* `app.c` không gọi CAN, fault manager hoặc diagnostic.
* `app.c` không đọc/ghi GPIO trực tiếp; chỉ điều phối module-level API.
* Không sửa file CubeMX sinh ra ngoài vùng USER CODE.

### File liên quan

* `Core/Inc/app.h`
* `Core/Src/app.c`
* `Core/Src/main.c`
* `CMakeLists.txt`
* `PROJECT_CONTEXT.md`
* `CURRENT_STATUS.md`
* `TASK_LOG.md`

### Ghi chú

* Lập trường khi sửa: các bug 1-6 là lỗi logic/safety thật và đã vá. Hai điểm đề xuất thêm cũng hợp lý, nhưng chọn không thêm enum mới để tránh đổi API ở giai đoạn này.

## 2026-06-24 11:35 - Review và sửa module feedback

### Mục tiêu

Review `feedback.h/.c` theo hướng không sửa máy móc: đọc code hiện tại, xác nhận bug thật, rồi mới sửa các điểm có lợi cho runtime/safety.

### Xác nhận trước khi sửa

* `feedback` đang đọc GPIO qua `bsp_gpio`: `Feedback_Update()` gọi `BSP_GPIO_ReadFeedbackMask()`, nhưng sau đó lại đọc từng chân bằng `BSP_GPIO_ReadFeedback()`.
* Raw feedback mask format: mỗi group dùng 2 bit, TE+ ở bit `group * 2`, TE- ở bit `group * 2 + 1`; Group 1 là bit 0/1, Group 6 là bit 10/11.
* `Feedback_Update()` cũ đọc input nhiều lần trong một lần update, nên raw mask và group status có thể không cùng snapshot.
* `feedback` cũ chưa có debounce thật, chỉ là snapshot logic đã đổi polarity.
* `contactor` đang dùng API `Feedback_IsGroupOn/Off/Mismatch()` như trạng thái feedback hợp lệ; trước khi sửa, đó là snapshot, sau khi sửa là stable/debounced.
* Đổi API `Feedback_Init/Feedback_Update` sang nhận `now_ms` không làm vỡ caller hiện tại vì chưa có caller ngoài chính `feedback.c`.

### Việc đã làm

* Đổi API thành `Feedback_Init(uint32_t now_ms)` và `Feedback_Update(uint32_t now_ms)`.
* Sửa `Feedback_Update()` để chỉ đọc phần cứng một lần bằng `BSP_GPIO_ReadFeedbackMask()`.
* Derive raw P/N, raw state, candidate state và stable state từ cùng một raw mask.
* Thêm debounce theo `CONFIG_FEEDBACK_DEBOUNCE_MS` bằng phép trừ `uint32_t` chống tràn tick.
* Giữ `Feedback_GetRawMask()` là raw tức thời.
* Chuyển các API group status và ON/OFF/MISMATCH mask sang dùng stable/debounced state.
* Giữ invalid group trả MISMATCH theo hướng fail-safe, thêm comment rõ trong code.
* Không thêm enum mới, không sửa pin mapping, không sửa file CubeMX sinh ra.

### Kết quả cuối cùng

* Build OK bằng `cmake --build build\Debug`.
* `feedback` không điều khiển output và không gọi `BSP_GPIO_SetOutput()`.
* `contactor` vẫn build OK, tiếp tục không đọc GPIO trực tiếp.

### Manual verification

* Test 1-3: pass theo kiểm tra mapping raw mask và init stable = raw.
* Test 4-7: pass theo flow candidate/stable debounce.
* Test 8: pass vì mask stable được rebuild theo state loại trừ lẫn nhau.
* Test 9: pass vì invalid group được guard trước khi truy cập mảng.
* Test 10: pass vì debounce dùng `(uint32_t)(now_ms - candidate_start_ms) >= CONFIG_FEEDBACK_DEBOUNCE_MS`.

### File liên quan

* `Core/Inc/feedback.h`
* `Core/Src/feedback.c`
* `PROJECT_CONTEXT.md`
* `CURRENT_STATUS.md`
* `TASK_LOG.md`

## 2026-06-24 11:55 - Kiểm tra tích hợp sau khi feedback đổi API

### Mục tiêu

Kiểm tra toàn project sau khi `feedback` có debounce thật và API `Feedback_Init/Update(uint32_t now_ms)`, chỉ sửa các điểm thật sự cần thiết.

### Việc đã kiểm tra

* Search source project không thấy caller runtime kiểu cũ `Feedback_Init()` hoặc `Feedback_Update()` không truyền `now_ms`.
* `app.c` hiện vẫn rỗng và `main.c` chưa gọi trực tiếp `Feedback_Init/Update` hoặc `Contactor_Init/Task`, nên chưa có caller runtime cần sửa.
* `bsp_gpio_feedback_mask_t` hiện là `uint16_t`, đủ chứa raw feedback mask 12 bit.
* `CONFIG_FEEDBACK_DEBOUNCE_MS` tồn tại trong `config.h`.
* `contactor` vẫn dùng `Feedback_IsGroupOn/Off/Mismatch()` và không đọc GPIO trực tiếp.
* `CMakeLists.txt` đã có `Core/Src/bsp_gpio.c`, `Core/Src/feedback.c`, `Core/Src/contactor.c`; không cần thêm file mới.

### Việc đã sửa

* Thêm `CONFIG_CONTACTOR_STABLE_FAULT_DELAY_MS` trong `config.h`.
* Sửa `contactor.c` để `CONTACTOR_STABLE_FAULT_DELAY_MS` dùng macro mới, không dùng trực tiếp `CONFIG_FEEDBACK_DEBOUNCE_MS`.

### Kết quả cuối cùng

* Build OK bằng `cmake --build build\Debug`.
* Không sửa file CubeMX sinh ra.
* Init/task order runtime chưa áp dụng vì chưa có app runtime thật. Khi triển khai `app`, thứ tự cần là `Feedback_Init(now_ms)` trước `Contactor_Init(now_ms)` và `Feedback_Update(now_ms)` trước `Contactor_Task(now_ms)`.

### File liên quan

* `Core/Inc/config.h`
* `Core/Src/contactor.c`
* `PROJECT_CONTEXT.md`
* `CURRENT_STATUS.md`
* `TASK_LOG.md`

## 2026-06-24 12:45 - Tạo PC unit test harness bước 1 cho feedback

### Mục tiêu

Tạo test harness C đơn giản chạy trên PC cho module `feedback`, đặt riêng ngoài project firmware tại `F:\Work\Project\MOCK_TEST\Feedback_TEST`.

### Việc đã làm

* Tạo `F:\Work\Project\MOCK_TEST\Feedback_TEST\CMakeLists.txt`.
* Tạo `test_main.c` và `test_feedback.c`.
* Tạo mock BSP tại `mocks/mock_bsp_gpio.c` và `mocks/mock_bsp_gpio.h`.
* Tạo assert macro đơn giản tại `support/test_assert.h`.
* Test project link thật `F:\Work\Project\DC_Combiner_MCU_IO_FW\Core\Src\feedback.c`.
* Test project không copy firmware source và không link thật `bsp_gpio.c`.
* Không sửa firmware logic, không sửa CubeMX generated file, không thêm test contactor/app.

### Testcase đã viết

* Init raw OFF/ON/MISMATCH thành stable ban đầu đúng.
* Raw mask bit mapping group 1, group 2, group 6.
* Glitch ON ngắn hơn debounce không đổi stable.
* Raw ON giữ đủ debounce thì stable ON.
* Mismatch ngắn không đổi stable.
* Mismatch giữ đủ debounce thì stable MISMATCH.
* ON/OFF/MISMATCH mask loại trừ nhau và OR đủ group mask.
* Invalid group trả fail-safe.
* Tick overflow debounce vẫn đúng.

### Kết quả build/test

* Đã thử `cmake -S . -B build` tại `F:\Work\Project\MOCK_TEST\Feedback_TEST`.
* Configure chưa chạy được vì CMake mặc định chọn `NMake Makefiles` nhưng không tìm thấy `nmake`.
* Đã thử generator `Visual Studio 17 2022`, nhưng CMake không tìm thấy Visual Studio instance.
* Đã kiểm tra PATH: chỉ thấy `arm-none-eabi-gcc`, chưa thấy compiler PC như GCC/Clang/MSVC.
* Chưa chạy được `test_feedback.exe` trên PC trong môi trường hiện tại.
* Đã dọn cache configure lỗi `build` và `build_vs` để lần sau chạy lại sạch.

### Lệnh dự kiến khi có compiler PC

```powershell
cd F:\Work\Project\MOCK_TEST\Feedback_TEST
cmake -S . -B build
cmake --build build
.\build\test_feedback.exe
```

Nếu dùng Visual Studio generator multi-config thì executable có thể nằm tại:

```powershell
.\build\Debug\test_feedback.exe
```

### File liên quan

* `F:\Work\Project\MOCK_TEST\Feedback_TEST\CMakeLists.txt`
* `F:\Work\Project\MOCK_TEST\Feedback_TEST\test_main.c`
* `F:\Work\Project\MOCK_TEST\Feedback_TEST\test_feedback.c`
* `F:\Work\Project\MOCK_TEST\Feedback_TEST\mocks\mock_bsp_gpio.c`
* `F:\Work\Project\MOCK_TEST\Feedback_TEST\mocks\mock_bsp_gpio.h`
* `F:\Work\Project\MOCK_TEST\Feedback_TEST\support\test_assert.h`
* `CURRENT_STATUS.md`
* `TASK_LOG.md`

## 2026-06-24 13:10 - Tạo PC unit test harness cho contactor

### Mục tiêu

Tạo test harness C đơn giản chạy trên PC cho module `contactor`, đặt riêng ngoài project firmware tại `F:\Work\Project\MOCK_TEST\Contactor_TEST`.

### Việc đã làm

* Tạo `F:\Work\Project\MOCK_TEST\Contactor_TEST\CMakeLists.txt`.
* Tạo `test_main.c` và `test_contactor.c`.
* Tạo mock feedback tại `mocks/mock_feedback.c` và `mocks/mock_feedback.h`.
* Tạo mock BSP GPIO tại `mocks/mock_bsp_gpio.c` và `mocks/mock_bsp_gpio.h`.
* Tạo assert macro đơn giản tại `support/test_assert.h`.
* Test project link thật `F:\Work\Project\DC_Combiner_MCU_IO_FW\Core\Src\contactor.c`.
* Test project không link thật `feedback.c` hoặc `bsp_gpio.c`.
* Không sửa firmware logic, không sửa root firmware CMake, không sửa file CubeMX generated.

### Testcase đã viết và chạy

* Test 1: Init feedback OFF -> state OFF.
* Test 2: Init feedback ON -> OFF_REQUESTED.
* Test 3: Init feedback ON timeout -> FAULT/STUCK_ON.
* Test 4: Request ON từ OFF -> ON_REQUESTED.
* Test 5: Feedback ON trước timeout -> ON_CONFIRMED.
* Test 6: Request ON timeout -> FAULT/ON_TIMEOUT.
* Test 7: Request OFF từ ON_CONFIRMED -> OFF_REQUESTED.
* Test 8: Feedback OFF trước timeout -> OFF.
* Test 9: Request OFF timeout feedback ON -> FAULT/STUCK_ON.
* Test 10: Mismatch trong transition timeout -> FAULT/FEEDBACK_MISMATCH.
* Test 11: ClearFault chỉ clear khi feedback OFF.
* Test 12: ClearFault không clear khi feedback ON/MISMATCH.
* Test 13: ClearAllFaults không tắt group ON bình thường.
* Test 14: AllOff gọi lặp không reset OFF timeout.
* Test 15: SetCommandMask xử lý OFF trước ON.
* Test 16: Tick overflow timeout vẫn đúng.

### Kết quả build/test

* Configure OK bằng MSYS2 UCRT64:

```bash
cd /f/Work/Project/MOCK_TEST/Contactor_TEST
cmake -S . -B build -G Ninja
```

* Build OK:

```bash
cmake --build build
```

* Run OK:

```bash
./build/test_contactor.exe
```

* Kết quả: 16 passed, 0 failed.

### File liên quan

* `F:\Work\Project\MOCK_TEST\Contactor_TEST\CMakeLists.txt`
* `F:\Work\Project\MOCK_TEST\Contactor_TEST\test_main.c`
* `F:\Work\Project\MOCK_TEST\Contactor_TEST\test_contactor.c`
* `F:\Work\Project\MOCK_TEST\Contactor_TEST\mocks\mock_feedback.c`
* `F:\Work\Project\MOCK_TEST\Contactor_TEST\mocks\mock_feedback.h`
* `F:\Work\Project\MOCK_TEST\Contactor_TEST\mocks\mock_bsp_gpio.c`
* `F:\Work\Project\MOCK_TEST\Contactor_TEST\mocks\mock_bsp_gpio.h`
* `F:\Work\Project\MOCK_TEST\Contactor_TEST\support\test_assert.h`
* `CURRENT_STATUS.md`
* `TASK_LOG.md`

## 2026-06-24 13:35 - Bổ sung testcase cho Contactor_TEST

### Mục tiêu

Bổ sung testcase cho PC unit test harness hiện có của `contactor`, không tạo project test mới và không sửa firmware logic.

### Việc đã làm

* Bổ sung counter `MockBSP_GetAllOutputsOffCount()` trong mock BSP để kiểm tra `Contactor_Init()` gọi `BSP_GPIO_AllOutputsOff()`.
* Bổ sung 10 testcase mới vào `F:\Work\Project\MOCK_TEST\Contactor_TEST\test_contactor.c`.
* Chạy lại toàn bộ test bằng MSYS2 UCRT64 + GCC + CMake + Ninja.
* Không sửa firmware logic, không sửa root firmware CMake, không sửa file CubeMX generated.

### Testcase mới đã thêm

* Test 17: `ON_CONFIRMED`, feedback OFF đủ delay -> `FAULT/CONTACTOR_FAULT_UNEXPECTED_OFF`.
* Test 18: `ON_CONFIRMED`, feedback MISMATCH đủ delay -> `FAULT/CONTACTOR_FAULT_FEEDBACK_MISMATCH`.
* Test 19: `ON_CONFIRMED`, feedback OFF ngắn hơn delay rồi ON lại -> vẫn `ON_CONFIRMED`, fault `NONE`.
* Test 20: `OFF`, feedback ON đủ delay -> `FAULT/CONTACTOR_FAULT_STUCK_ON`.
* Test 21: `OFF`, feedback MISMATCH đủ delay -> `FAULT/CONTACTOR_FAULT_FEEDBACK_MISMATCH`.
* Test 22: `OFF`, feedback ON ngắn hơn delay rồi OFF lại -> vẫn `OFF`, fault `NONE`.
* Test 23: `Contactor_ClearFault()` trên group không fault không đổi state/output.
* Test 24: Invalid group guard cho API public quan trọng.
* Test 25: Exact value cho `Contactor_GetCommandMask()`, `Contactor_GetOnConfirmedMask()`, `Contactor_GetFaultMask()`.
* Test 26: `Contactor_Init()` gọi `BSP_GPIO_AllOutputsOff()`.

### Kết quả build/test

```bash
cd /f/Work/Project/MOCK_TEST/Contactor_TEST
cmake -S . -B build -G Ninja
cmake --build build
./build/test_contactor.exe
```

* Kết quả old tests: 16/16 pass.
* Kết quả new tests: 10/10 pass.
* Tổng kết: 26 passed, 0 failed.

### File liên quan

* `F:\Work\Project\MOCK_TEST\Contactor_TEST\test_contactor.c`
* `F:\Work\Project\MOCK_TEST\Contactor_TEST\mocks\mock_bsp_gpio.c`
* `F:\Work\Project\MOCK_TEST\Contactor_TEST\mocks\mock_bsp_gpio.h`
* `CURRENT_STATUS.md`
* `TASK_LOG.md`

## 2026-06-24 15:30 - Bổ sung testcase fail-safe còn thiếu cho Contactor_TEST

### Mục tiêu

Bổ sung các testcase fail-safe còn thiếu cho PC unit test harness hiện có của `contactor`, tiếp tục dùng real `Core/Src/contactor.c` với mock `feedback` và mock `bsp_gpio`.

### Việc đã làm

* Bổ sung 10 testcase mới vào `F:\Work\Project\MOCK_TEST\Contactor_TEST\test_contactor.c`.
* Không tạo test project mới.
* Không sửa firmware logic vì các testcase mới đều pass, chưa chứng minh bug thật.
* Không sửa root firmware CMake và không sửa file CubeMX generated.
* Chạy lại toàn bộ `test_contactor.exe` bằng MSYS2 UCRT64 + GCC + CMake + Ninja.

### Testcase mới đã thêm

* Test 27: `RequestOn()` khi đang `OFF_REQUESTED` bị reject.
* Test 28: `SetCommandMask()` không bật group đang `OFF_REQUESTED`.
* Test 29: `RequestOn()` khi đang `FAULT` bị reject.
* Test 30: `SetCommandMask()` không bật group đang `FAULT`.
* Test 31: `RequestOff()` khi đang `FAULT` vẫn giữ output OFF và không clear fault ngoài ý muốn.
* Test 32: `SetCommandMask()` ignore bit ngoài `CONFIG_GROUP_ALL_MASK`.
* Test 33: `ClearAllFaults()` trả false nếu còn fault chưa clear được.
* Test 34: `ClearAllFaults()` clear được tất cả fault khi feedback đã OFF.
* Test 35: `RequestOff()` khi đang `OFF` là idempotent.
* Test 36: `RequestOn()` gọi lặp khi đang `ON_REQUESTED` không reset timeout.

### Kết quả build/test

```bash
cd /f/Work/Project/MOCK_TEST/Contactor_TEST
cmake -S . -B build -G Ninja
cmake --build build
./build/test_contactor.exe
```

* Kết quả previous tests: 26/26 pass.
* Kết quả new tests: 10/10 pass.
* Tổng kết: 36 passed, 0 failed.

### Phạm vi chưa test

* Chưa test `app`.
* Chưa test CAN.
* Chưa test integration trên board thật.

### File liên quan

* `F:\Work\Project\MOCK_TEST\Contactor_TEST\test_contactor.c`
* `CURRENT_STATUS.md`
* `TASK_LOG.md`

## 2026-06-24 15:50 - Tạo PC unit test harness cho App_TEST

### Mục tiêu

Tạo PC unit test harness riêng cho module `app`, đặt ngoài project firmware thật tại `F:\Work\Project\MOCK_TEST\App_TEST`.

### Việc đã làm

* Tạo `F:\Work\Project\MOCK_TEST\App_TEST\CMakeLists.txt`.
* Tạo `test_main.c` và `test_app.c`.
* Tạo mock `main.h` để stub `HAL_GetTick()` trên PC.
* Tạo mock HAL tại `mocks/mock_hal.c` và `mocks/mock_hal.h`.
* Tạo mock feedback tại `mocks/mock_feedback.c` và `mocks/mock_feedback.h`.
* Tạo mock contactor tại `mocks/mock_contactor.c` và `mocks/mock_contactor.h`.
* Tạo assert macro đơn giản tại `support/test_assert.h`.
* Test project link thật `F:\Work\Project\DC_Combiner_MCU_IO_FW\Core\Src\app.c`.
* Không link thật `feedback.c`, `contactor.c` hoặc `bsp_gpio.c`.
* Không sửa firmware logic, không sửa root firmware CMake, không sửa file CubeMX generated.

### Testcase đã thêm

* Test 1: `App_Init()` gọi `Feedback_Init()` trước `Contactor_Init()` và truyền cùng `now_ms`.
* Test 2: `App_Task()` chưa đủ `CONFIG_APP_TASK_PERIOD_MS` thì không gọi module task.
* Test 3: `App_Task()` đủ period thì gọi `Feedback_Update()` trước `Contactor_Task()`.
* Test 4: Sau khi task chạy, mốc period được cập nhật đúng.
* Test 5: Tick overflow vẫn chạy đúng period.
* Test 6: Gọi `App_Task()` nhiều lần trong cùng tick chỉ chạy tối đa một lần.

### Kết quả build/test

```bash
cd /f/Work/Project/MOCK_TEST/App_TEST
cmake -S . -B build -G Ninja
cmake --build build
./build/test_app.exe
```

* Tổng kết: 6 passed, 0 failed.

### Phạm vi chưa test

* Chưa test CAN.
* Chưa test integration.
* Chưa test board thật.

### File liên quan

* `F:\Work\Project\MOCK_TEST\App_TEST\CMakeLists.txt`
* `F:\Work\Project\MOCK_TEST\App_TEST\test_main.c`
* `F:\Work\Project\MOCK_TEST\App_TEST\test_app.c`
* `F:\Work\Project\MOCK_TEST\App_TEST\mocks\main.h`
* `F:\Work\Project\MOCK_TEST\App_TEST\mocks\mock_hal.c`
* `F:\Work\Project\MOCK_TEST\App_TEST\mocks\mock_hal.h`
* `F:\Work\Project\MOCK_TEST\App_TEST\mocks\mock_feedback.c`
* `F:\Work\Project\MOCK_TEST\App_TEST\mocks\mock_feedback.h`
* `F:\Work\Project\MOCK_TEST\App_TEST\mocks\mock_contactor.c`
* `F:\Work\Project\MOCK_TEST\App_TEST\mocks\mock_contactor.h`
* `F:\Work\Project\MOCK_TEST\App_TEST\support\test_assert.h`
* `CURRENT_STATUS.md`
* `TASK_LOG.md`

## 2026-06-24 16:15 - Tạo PC integration test harness cho app + feedback + contactor

### Mục tiêu

Tạo PC integration test harness riêng cho chuỗi runtime `app + feedback + contactor`, đặt ngoài project firmware thật tại `F:\Work\Project\MOCK_TEST\Integration_TEST`.

### Việc đã làm

* Tạo `F:\Work\Project\MOCK_TEST\Integration_TEST\CMakeLists.txt`.
* Tạo `test_main.c` và `test_integration.c`.
* Tạo mock `main.h` để stub `HAL_GetTick()` trên PC.
* Tạo mock HAL tại `mocks/mock_hal.c` và `mocks/mock_hal.h`.
* Tạo mock BSP GPIO tại `mocks/mock_bsp_gpio.c` và `mocks/mock_bsp_gpio.h`.
* Tạo assert macro đơn giản tại `support/test_assert.h`.
* Test project link thật `Core/Src/app.c`, `Core/Src/feedback.c` và `Core/Src/contactor.c`.
* Không link thật `bsp_gpio.c`; mock chỉ giữ raw feedback mask và output mask.
* Không sửa firmware logic, không sửa root firmware CMake, không sửa file CubeMX generated.
* Không sửa `Core/Src/main.c` hoặc `Core/Inc/main.h`.

### Testcase đã thêm

* Test 1: Boot feedback OFF toàn bộ -> tất cả contactor OFF.
* Test 2: Boot feedback ON group 0 -> `OFF_REQUESTED`, sau timeout -> `FAULT/STUCK_ON`.
* Test 3: Request ON thành công với feedback debounce thật.
* Test 4: Request ON nhưng feedback không ON -> `FAULT/ON_TIMEOUT`.
* Test 5: Request OFF thành công với feedback debounce thật.
* Test 6: `ON_CONFIRMED` mất feedback OFF đủ delay -> `FAULT/UNEXPECTED_OFF`.
* Test 7: `ON_CONFIRMED` bị glitch OFF ngắn hơn debounce -> không fault.
* Test 8: `OFF` bị feedback ON đủ debounce và stable delay -> `FAULT/STUCK_ON`.
* Test 9: `Contactor_SetCommandMask()` ON/OFF nhiều group.
* Test 10: Tick overflow trong `App_Task`, feedback debounce và contactor timeout vẫn đúng.

### Kết quả build/test

```bash
cd /f/Work/Project/MOCK_TEST/Integration_TEST
cmake -S . -B build -G Ninja
cmake --build build
./build/test_integration.exe
```

* Tổng kết: 10 passed, 0 failed.

### Phạm vi chưa test

* Chưa test CAN.
* Chưa test board thật.
* Chưa test hardware polarity thực tế của feedback/DIP.

### File liên quan

* `F:\Work\Project\MOCK_TEST\Integration_TEST\CMakeLists.txt`
* `F:\Work\Project\MOCK_TEST\Integration_TEST\test_main.c`
* `F:\Work\Project\MOCK_TEST\Integration_TEST\test_integration.c`
* `F:\Work\Project\MOCK_TEST\Integration_TEST\mocks\main.h`
* `F:\Work\Project\MOCK_TEST\Integration_TEST\mocks\mock_hal.c`
* `F:\Work\Project\MOCK_TEST\Integration_TEST\mocks\mock_hal.h`
* `F:\Work\Project\MOCK_TEST\Integration_TEST\mocks\mock_bsp_gpio.c`
* `F:\Work\Project\MOCK_TEST\Integration_TEST\mocks\mock_bsp_gpio.h`
* `F:\Work\Project\MOCK_TEST\Integration_TEST\support\test_assert.h`
* `CURRENT_STATUS.md`
* `TASK_LOG.md`

## 2026-06-24 16:35 - Kiểm tra firmware thật trước khi code CAN

### Mục tiêu

Kiểm tra lại firmware project thật trước khi chuyển sang code CAN, chỉ kiểm tra `main.c`, root `CMakeLists.txt`, trạng thái build và hook runtime hiện tại.

### Việc đã kiểm tra

* `Core/Src/main.c` có `#include "app.h"` trong vùng `USER CODE BEGIN Includes`.
* `App_Init()` được gọi sau `MX_GPIO_Init()` và `MX_CAN_Init()`.
* `App_Init()` nằm trong vùng `USER CODE BEGIN 2`.
* `App_Task()` được gọi trong vòng `while (1)`.
* `App_Task()` nằm trong vùng `USER CODE BEGIN 3`.
* `main.c` không gọi trực tiếp `Feedback_Init()`, `Feedback_Update()`, `Contactor_Init()`, `Contactor_Task()`, `BSP_GPIO_SetOutput()` hoặc `BSP_GPIO_ReadFeedbackMask()`.
* Root `CMakeLists.txt` include đủ:
  * `Core/Src/app.c`
  * `Core/Src/bsp_gpio.c`
  * `Core/Src/feedback.c`
  * `Core/Src/contactor.c`
* Root `CMakeLists.txt` không include file test hoặc folder `F:\Work\Project\MOCK_TEST`.

### Kết quả build

```powershell
cd F:\Work\Project\DC_Combiner_MCU_IO_FW
cmake --build build\Debug
```

* Kết quả: build OK (`ninja: no work to do.`).

### Thay đổi code

* Không sửa firmware logic.
* Không sửa `main.c`.
* Không sửa root `CMakeLists.txt`.
* Không code CAN trong bước này.

### File liên quan

* `Core/Src/main.c`
* `CMakeLists.txt`
* `CURRENT_STATUS.md`
* `TASK_LOG.md`

## 2026-06-24 17:05 - Code module CAN communication tối thiểu

### Mục tiêu

Code `can_comm.h/.c` tối thiểu cho firmware thật: nhận command CAN, apply command trong task, gửi status định kỳ. Chưa code lost CAN timeout, chưa code heartbeat và chưa tạo PC CAN test harness.

### Việc đã làm

* Code `Core/Inc/can_comm.h` với API public:
  * `CanComm_Init(uint32_t now_ms)`
  * `CanComm_Task(uint32_t now_ms)`
  * `CanComm_SendStatus(uint32_t now_ms)`
  * getter NodeID, last RX time, pending command, counter RX/TX/error và started state.
* Code `Core/Src/can_comm.c`.
* `CanComm_Init()` đọc NodeID bằng `BSP_GPIO_ReadNodeId()`, tính ID bằng helper trong `config.h`.
* Config CAN filter exact Standard ID cho command ID `0x500 + NodeID`.
* Gọi `HAL_CAN_ConfigFilter()`, `HAL_CAN_Start()` và `HAL_CAN_ActivateNotification(CAN_IT_RX_FIFO0_MSG_PENDING)`.
* Implement `HAL_CAN_RxFifo0MsgPendingCallback()` để chỉ lưu pending command, không gọi contactor trực tiếp trong interrupt callback.
* `CanComm_Task()` copy/clear pending command trong critical section ngắn rồi gọi `Contactor_SetCommandMask(mask, now_ms)`.
* `CanComm_Task()` gửi status định kỳ theo `CONFIG_STATUS_PERIOD_MS`, dùng tick subtraction chống overflow.
* `CanComm_SendStatus()` gửi Standard ID `0x510 + NodeID`, DLC 8.
* Status payload:
  * `Data[0] = BSP_GPIO_GetOutputMask()`
  * `Data[1] = Feedback_GetRawMask()` low byte
  * `Data[2] = Feedback_GetRawMask()` high byte
  * `Data[3] = Contactor_GetFaultMask()`
  * `Data[4] = global status bitmask`
  * `Data[5..7] = 0`
* Tích hợp `CanComm_Init()` và `CanComm_Task()` vào `Core/Src/app.c`.
* Thêm `Core/Src/can_comm.c` vào root `CMakeLists.txt`.

### Thiết kế đã giữ

* Không code lost CAN timeout.
* Không gọi `Contactor_AllOff()` vì mất CAN.
* Không code heartbeat TX.
* Không block/wait trong TX.
* Không gọi `Contactor_SetCommandMask()` trong interrupt callback.
* Không sửa `main.c`, `main.h`, `can.c`, `bsp_gpio.c`, `feedback.c` hoặc `contactor.c`.

### Kết quả build

```powershell
cd F:\Work\Project\DC_Combiner_MCU_IO_FW
cmake --build build\Debug
```

* Kết quả: build OK.
* Memory sau build: RAM 1880 B / 20 KB, FLASH 13880 B / 64 KB.

### Ghi chú test

* Chưa tạo PC CAN unit test harness trong bước này theo yêu cầu.
* Sau khi `app.c` gọi `CanComm_Init/Task`, các PC test cũ có link `app.c` sẽ cần cập nhật mock/link CAN trước khi chạy lại.

### File liên quan

* `Core/Inc/can_comm.h`
* `Core/Src/can_comm.c`
* `Core/Src/app.c`
* `CMakeLists.txt`
* `CURRENT_STATUS.md`
* `TASK_LOG.md`

## 2026-06-24 17:35 - Tạo PC unit test harness cho can_comm

### Mục tiêu

Tạo PC unit test harness riêng cho module `can_comm`, đặt ngoài project firmware thật tại `F:\Work\Project\MOCK_TEST\CanComm_TEST`.

### Việc đã làm

* Tạo `F:\Work\Project\MOCK_TEST\CanComm_TEST\CMakeLists.txt`.
* Tạo `test_main.c` và `test_can_comm.c`.
* Tạo mock HAL/CAN tối thiểu:
  * `mocks/main.h`
  * `mocks/can.h`
  * `mocks/stm32f1xx_hal.h`
  * `mocks/mock_hal_can.c`
  * `mocks/mock_hal_can.h`
* Tạo mock BSP GPIO:
  * `mocks/mock_bsp_gpio.c`
  * `mocks/mock_bsp_gpio.h`
* Tạo mock feedback:
  * `mocks/mock_feedback.c`
  * `mocks/mock_feedback.h`
* Tạo mock contactor:
  * `mocks/mock_contactor.c`
  * `mocks/mock_contactor.h`
* Tạo assert macro đơn giản tại `support/test_assert.h`.
* Test project link thật `F:\Work\Project\DC_Combiner_MCU_IO_FW\Core\Src\can_comm.c`.
* Test project không link thật `app.c`, `bsp_gpio.c`, `feedback.c`, `contactor.c` hoặc `can.c`.
* Không sửa firmware logic, không sửa `main.c`, không sửa `main.h` thật, không sửa `can.c/can.h` CubeMX thật.

### Testcase đã thêm

* Test 1: Init success.
* Test 2: Init fail ở `HAL_CAN_ConfigFilter()`.
* Test 3: Init fail ở `HAL_CAN_Start()`.
* Test 4: Init fail ở `HAL_CAN_ActivateNotification()`.
* Test 5: RX command hợp lệ set pending.
* Test 6: RX sai ID bị ignore.
* Test 7: RX extended ID bị ignore.
* Test 8: RX remote frame bị ignore.
* Test 9: RX DLC = 0 bị ignore.
* Test 10: `CanComm_Task()` apply pending command.
* Test 11: Nhiều RX trước task giữ command mới nhất.
* Test 12: Không pending thì task không gọi contactor.
* Test 13: Chưa đủ period thì chưa gửi status.
* Test 14: Đủ period thì gửi status đúng payload.
* Test 15: Không có fault thì clear bit `ANY_FAULT`.
* Test 16: TX error không block và tăng counter.
* Test 17: Status period xử lý đúng khi tick overflow.
* Test 18: `CanComm_SendStatus()` khi CAN chưa started trả false.

### Kết quả build/test

```bash
cd /f/Work/Project/MOCK_TEST/CanComm_TEST
cmake -S . -B build -G Ninja
cmake --build build
./build/test_can_comm.exe
```

* Tổng kết: 18 passed, 0 failed.

### Phạm vi chưa test

* Chưa test board thật với USB-CAN-B.
* Chưa test lost CAN timeout.
* Chưa test heartbeat.

### File liên quan

* `F:\Work\Project\MOCK_TEST\CanComm_TEST\CMakeLists.txt`
* `F:\Work\Project\MOCK_TEST\CanComm_TEST\test_main.c`
* `F:\Work\Project\MOCK_TEST\CanComm_TEST\test_can_comm.c`
* `F:\Work\Project\MOCK_TEST\CanComm_TEST\mocks\main.h`
* `F:\Work\Project\MOCK_TEST\CanComm_TEST\mocks\can.h`
* `F:\Work\Project\MOCK_TEST\CanComm_TEST\mocks\stm32f1xx_hal.h`
* `F:\Work\Project\MOCK_TEST\CanComm_TEST\mocks\mock_hal_can.c`
* `F:\Work\Project\MOCK_TEST\CanComm_TEST\mocks\mock_hal_can.h`
* `F:\Work\Project\MOCK_TEST\CanComm_TEST\mocks\mock_bsp_gpio.c`
* `F:\Work\Project\MOCK_TEST\CanComm_TEST\mocks\mock_bsp_gpio.h`
* `F:\Work\Project\MOCK_TEST\CanComm_TEST\mocks\mock_feedback.c`
* `F:\Work\Project\MOCK_TEST\CanComm_TEST\mocks\mock_feedback.h`
* `F:\Work\Project\MOCK_TEST\CanComm_TEST\mocks\mock_contactor.c`
* `F:\Work\Project\MOCK_TEST\CanComm_TEST\mocks\mock_contactor.h`
* `F:\Work\Project\MOCK_TEST\CanComm_TEST\support\test_assert.h`
* `CURRENT_STATUS.md`
* `TASK_LOG.md`

## 2026-06-24 17:55 - Bổ sung testcase RX/error guard cho CanComm_TEST

### Mục tiêu

Bổ sung các testcase còn thiếu cho PC unit test harness `CanComm_TEST` hiện có, tập trung vào đường lỗi RX callback và trạng thái CAN chưa started.

### Việc đã làm

* Bổ sung 5 testcase mới vào `F:\Work\Project\MOCK_TEST\CanComm_TEST\test_can_comm.c`.
* Không tạo test project mới.
* Không sửa firmware logic vì các testcase mới đều pass, chưa chứng minh bug thật.
* Không sửa `main.c`, `main.h` thật, `can.c/can.h` thật hoặc root firmware CMake.
* Không sửa mock vì mock hiện có đã đủ cho các testcase mới.

### Testcase mới đã thêm

* Test 19: RX callback với CAN handle khác `&hcan` bị ignore.
* Test 20: `HAL_CAN_GetRxMessage()` trả `HAL_ERROR` thì tăng `rx_error_count`.
* Test 21: `CanComm_Task()` khi CAN chưa started không gọi contactor và không gửi status.
* Test 22: `CanComm_Init()` đọc NodeID đúng 1 lần.
* Test 23: RX recover được sau một lần HAL RX error.

### Kết quả build/test

```bash
cd /f/Work/Project/MOCK_TEST/CanComm_TEST
cmake -S . -B build -G Ninja
cmake --build build
./build/test_can_comm.exe
```

* Kết quả previous tests: 18/18 pass.
* Kết quả new tests: 5/5 pass.
* Tổng kết: 23 passed, 0 failed.

### Phạm vi chưa test

* Chưa test board thật với USB-CAN-B.
* Chưa test lost CAN timeout.
* Chưa test heartbeat.

### File liên quan

* `F:\Work\Project\MOCK_TEST\CanComm_TEST\test_can_comm.c`
* `CURRENT_STATUS.md`
* `TASK_LOG.md`

## 2026-06-25 - Thêm Clear Fault qua CAN

### Mục tiêu

Thêm chức năng Clear Fault qua CAN để test board không cần rút/cắm nguồn sau mỗi lần fault, nhưng vẫn giữ điều kiện an toàn: chỉ clear fault khi command mask đang yêu cầu OFF toàn bộ output.

### Việc đã làm

* Thêm macro protocol trong `Core/Inc/config.h`:
  * `CONFIG_CAN_COMMAND_MASK_INDEX = 0U`
  * `CONFIG_CAN_COMMAND_FLAGS_INDEX = 1U`
  * `CONFIG_CAN_COMMAND_CLEAR_FAULT_MASK = (1U << 0)`
  * `CONFIG_CAN_CLEAR_FAULT_ENABLE = 1U`
* Cập nhật `Core/Src/can_comm.c` để đọc thêm `Data[1]` làm command flags nếu frame có đủ byte.
* Giữ tương thích frame command cũ: frame chỉ có `Data[0]` vẫn được xử lý như output command mask.
* Nếu `Data[1] bit0` yêu cầu Clear Fault và `Data[0] == 0x00`, firmware gọi `Contactor_AllOff()`, `Contactor_ClearAllFaults()`, rồi `Contactor_AllOff()` lại để giữ output OFF.
* Nếu frame vừa yêu cầu ON vừa yêu cầu Clear Fault, ví dụ `Data[0] = 0x01`, `Data[1] = 0x01`, firmware không clear fault, không apply command ON và xử lý fail-safe all-off.
* Không sửa logic `contactor.c`; dùng API an toàn đã có sẵn `Contactor_ClearAllFaults()` và `Contactor_AllOff()`.
* Cập nhật `CanComm_TEST` với testcase Clear Fault hợp lệ, Clear Fault không hợp lệ khi mask khác 0, command cũ với flags 0, status sau clear fault, và macro disable.
* Thêm binary test riêng `test_can_comm_clear_disabled.exe` trong cùng `CanComm_TEST` để compile với `CONFIG_CAN_CLEAR_FAULT_ENABLE=0U`.
* Cập nhật mock `CanComm` cho `App_TEST` và `Integration_TEST` để các PC test cũ link lại được sau khi `app.c` đã tích hợp CAN runtime.
* Cập nhật `CURRENT_STATUS.md` và `PROJECT_CONTEXT.md` theo protocol mới.

### Protocol hiện tại

* PC gửi command xuống STM32:
  * Standard Data frame.
  * CAN ID = `0x500 + NodeID`.
  * `Data[0]` = output command mask.
  * `Data[1] bit0` = Clear Fault request.
* Clear Fault hợp lệ:
  * `Data[0] == 0x00`.
  * `Data[1] & CONFIG_CAN_COMMAND_CLEAR_FAULT_MASK` khác 0.
* Clear Fault không hợp lệ:
  * `Data[0] != 0x00` và `Data[1] bit0` set.
  * Firmware không clear fault, không bật output trong frame đó, và gọi all-off.
* Lệnh Clear Fault Node 0:
  * CAN ID = `0x500`.
  * CANID bytes = `00 00 05 00`.
  * Data = `00 01 00 00 00 00 00 00`.

### Kết quả PC test

```bash
cd /f/Work/Project/MOCK_TEST/CanComm_TEST
cmake -S . -B build -G Ninja
cmake --build build
./build/test_can_comm.exe
./build/test_can_comm_clear_disabled.exe
```

* `CanComm_TEST`: 28 passed, 0 failed.
* `CanComm_TEST` macro disable: 1 passed, 0 failed.

Chạy lại toàn bộ PC test hiện có:

* `Feedback_TEST`: 9 passed, 0 failed.
* `Contactor_TEST`: 36 passed, 0 failed.
* `App_TEST`: 6 passed, 0 failed.
* `Integration_TEST`: 10 passed, 0 failed.
* `CanComm_TEST`: 28 passed, 0 failed.
* `test_can_comm_clear_disabled.exe`: 1 passed, 0 failed.

### Kết quả firmware build

```powershell
cd F:\Work\Project\DC_Combiner_MCU_IO_FW
cmake --build build\Debug
```

* Build OK.
* RAM: 1880 B / 20 KB.
* FLASH: 14460 B / 64 KB.

### File liên quan

* `Core/Inc/config.h`
* `Core/Src/can_comm.c`
* `PROJECT_CONTEXT.md`
* `CURRENT_STATUS.md`
* `TASK_LOG.md`
* `F:\Work\Project\MOCK_TEST\CanComm_TEST\test_can_comm.c`
* `F:\Work\Project\MOCK_TEST\CanComm_TEST\test_can_comm_clear_disabled.c`
* `F:\Work\Project\MOCK_TEST\CanComm_TEST\mocks\mock_contactor.c`
* `F:\Work\Project\MOCK_TEST\CanComm_TEST\mocks\mock_contactor.h`
* `F:\Work\Project\MOCK_TEST\CanComm_TEST\CMakeLists.txt`
* `F:\Work\Project\MOCK_TEST\App_TEST\mocks\mock_can_comm.c`
* `F:\Work\Project\MOCK_TEST\App_TEST\mocks\mock_can_comm.h`
* `F:\Work\Project\MOCK_TEST\App_TEST\test_app.c`
* `F:\Work\Project\MOCK_TEST\App_TEST\CMakeLists.txt`
* `F:\Work\Project\MOCK_TEST\Integration_TEST\mocks\mock_can_comm.c`
* `F:\Work\Project\MOCK_TEST\Integration_TEST\CMakeLists.txt`

### Phạm vi chưa test

* Chưa test Clear Fault trên board thật với USB-CAN-B.
* Chưa code lost CAN timeout.
* Chưa code heartbeat TX.

### Ghi chú

* CẦN XÁC NHẬN: polarity feedback active LOW cần test lại bằng board thật.

## 2026-06-26 - Implement Lost CAN timeout fail-safe

### Mục tiêu

Implement Lost CAN timeout fail-safe theo firmware document: controller phải gửi command frame `0x500 + NodeID` định kỳ để duy trì điều khiển; nếu MCU đã từng nhận command hợp lệ rồi quá `CONFIG_CAN_COMMAND_TIMEOUT_MS` không nhận thêm command hợp lệ, firmware phải all-off và báo fail-safe/global fault.

### Việc đã làm

* Cập nhật `Core/Src/can_comm.c`:
  * Thêm state nội bộ `has_received_command`.
  * Thêm state nội bộ `can_failsafe_active`.
  * Chỉ command frame hợp lệ mới update `last_rx_ms` và set `has_received_command`.
  * Frame sai handle, sai ID, extended ID, remote frame, DLC = 0 hoặc HAL RX error không refresh timeout.
  * Khi timeout, gọi `Contactor_AllOff(now_ms)` và set fail-safe active.
  * Dùng phép trừ `uint32_t` qua helper `CanComm_IsElapsed()` để chống tick overflow.
  * Không báo lost CAN ngay sau boot nếu chưa từng nhận command hợp lệ.
* Cập nhật `Core/Inc/can_comm.h`:
  * Thêm getter `CanComm_IsFailSafeActive()` để test/diagnostic đọc trạng thái fail-safe nội bộ.
* Cập nhật status global:
  * Nếu fail-safe active, `Data[4]` set `CONFIG_STATUS_FAILSAFE_ACTIVE_MASK`.
  * Lost CAN được xem là global fault/fail-safe nên `Data[4]` cũng set `CONFIG_STATUS_ANY_FAULT_MASK`.
  * Nếu chỉ lost CAN và không có group fault, `Data[3] = 0x00`, `Data[4] = 0x07`.
* Cập nhật logic command khi đang fail-safe:
  * Command ON không được bật output và không tự thoát fail-safe.
  * Chỉ OFF all (`Data[0] = 0x00`, `Data[1] = 0x00`) hoặc Clear Fault hợp lệ (`Data[0] = 0x00`, `Data[1] bit0`) mới thoát fail-safe.
  * Khi thoát fail-safe vẫn giữ output OFF, không tự bật output trong cùng frame.
* Cập nhật `CanComm_TEST`:
  * Bổ sung Test 29 đến Test 37 cho Lost CAN timeout, refresh timeout, tick overflow, fail-safe exit, invalid frame không refresh timeout.
  * Cập nhật mock `Contactor_SetCommandMask()` và `Contactor_AllOff()` để đồng bộ output mask mock theo behavior thật.
* Cập nhật `CURRENT_STATUS.md` và `PROJECT_CONTEXT.md`.

### Behavior hiện tại

* Controller muốn giữ Group 1 ON phải gửi lặp lại trước timeout:

```text
CAN ID = 0x500 + NodeID
Data   = 01 00 00 00 00 00 00 00
```

* Nếu controller chỉ gửi ON một lần rồi im lặng:
  * Sau `CONFIG_CAN_COMMAND_TIMEOUT_MS`, MCU gọi all-off.
  * `Data[0]` về 0 nếu không có output nào đang ON.
  * `Data[3]` không set group fault riêng nếu chỉ lost CAN.
  * `Data[4]` có `CAN_ONLINE | ANY_FAULT | FAILSAFE_ACTIVE`, tức `0x07`.
* Khi đang fail-safe:
  * `Data = 01 00 00 00 00 00 00 00` không bật lại output.
  * `Data = 00 00 00 00 00 00 00 00` thoát fail-safe và giữ output OFF.
  * `Data = 00 01 00 00 00 00 00 00` thoát fail-safe, gọi clear fault theo điều kiện hiện có, và giữ output OFF.

### Testcase mới đã thêm

* Test 29: Boot chưa từng nhận command thì không enter fail-safe dù quá timeout.
* Test 30: Command hợp lệ trước timeout không enter fail-safe.
* Test 31: Command hợp lệ rồi quá timeout thì enter fail-safe, all-off, status có `0x07`.
* Test 32: Command định kỳ refresh timeout.
* Test 33: Lost CAN timeout xử lý đúng khi tick overflow.
* Test 34: Đang fail-safe, command ON bị block.
* Test 35: Thoát fail-safe bằng OFF all.
* Test 36: Thoát fail-safe bằng Clear Fault hợp lệ.
* Test 37: Invalid frames không refresh Lost CAN timeout.

### Kết quả PC test

```bash
cd /f/Work/Project/MOCK_TEST/CanComm_TEST
cmake -S . -B build -G Ninja
cmake --build build
./build/test_can_comm.exe
./build/test_can_comm_clear_disabled.exe
```

* `CanComm_TEST`: 37 passed, 0 failed.
* `CanComm_TEST` macro disable: 1 passed, 0 failed.

Chạy lại toàn bộ PC test hiện có:

* `Feedback_TEST`: 9 passed, 0 failed.
* `Contactor_TEST`: 36 passed, 0 failed.
* `App_TEST`: 6 passed, 0 failed.
* `Integration_TEST`: 10 passed, 0 failed.
* `CanComm_TEST`: 37 passed, 0 failed.
* `test_can_comm_clear_disabled.exe`: 1 passed, 0 failed.

### Kết quả firmware build

```powershell
cd F:\Work\Project\DC_Combiner_MCU_IO_FW
cmake --build build\Debug
```

* Build OK.
* RAM: 1880 B / 20 KB.
* FLASH: 14784 B / 64 KB.

### Board test guide

NodeID = 0:

* ON Group 1:

```text
CANID = 00 00 05 00
Data  = 01 00 00 00 00 00 00 00
```

* Nếu feedback Group 1 đúng trước timeout, status kỳ vọng:

```text
Data[0] = 01
Data[4] = 01
```

* Dừng gửi command quá `CONFIG_CAN_COMMAND_TIMEOUT_MS`, status kỳ vọng nếu chỉ lost CAN:

```text
Data[0] = 00
Data[3] = 00
Data[4] = 07
```

* Khi đang fail-safe, gửi ON lại:

```text
Data = 01 00 00 00 00 00 00 00
```

Kỳ vọng output vẫn OFF và `Data[4]` vẫn có bit fail-safe.

* Thoát fail-safe bằng OFF all:

```text
Data = 00 00 00 00 00 00 00 00
```

Kỳ vọng output vẫn OFF và `Data[4]` không còn bit fail-safe nếu không có lỗi khác.

### File liên quan

* `Core/Inc/can_comm.h`
* `Core/Src/can_comm.c`
* `CURRENT_STATUS.md`
* `PROJECT_CONTEXT.md`
* `TASK_LOG.md`
* `F:\Work\Project\MOCK_TEST\CanComm_TEST\test_can_comm.c`
* `F:\Work\Project\MOCK_TEST\CanComm_TEST\mocks\mock_contactor.c`
* `F:\Work\Project\MOCK_TEST\CanComm_TEST\mocks\mock_contactor.h`

### Phạm vi chưa test

* Chưa test Lost CAN timeout trên board thật với USB-CAN-B.
* Chưa implement heartbeat/diagnostic frame `0x520 + NodeID`.

## 2026-06-26 - Implement Heartbeat/Diagnostic frame 0x520 + NodeID

### Muc tieu

Implement Heartbeat/Diagnostic CAN frame de charger controller biet MCU I/O module con song. Heartbeat la frame MCU -> Controller, khong thay the Lost CAN timeout va khong refresh Lost CAN timeout.

### Viec da lam

* Cap nhat `Core/Inc/config.h`:
  * `CONFIG_CAN_HEARTBEAT_ENABLE = 1U`.
  * `CONFIG_CAN_HEARTBEAT_PERIOD_MS = CONFIG_HEARTBEAT_PERIOD_MS`.
  * `CONFIG_FW_VERSION_MAJOR = 1U`.
  * `CONFIG_FW_VERSION_MINOR = 0U`.
  * `CONFIG_FW_VERSION_PATCH = 0U`.
* Cap nhat `Core/Src/can_comm.c`:
  * Dung `can_heartbeat_id = 0x520 + NodeID`.
  * Them `last_heartbeat_tx_ms`.
  * Them `heartbeat_sequence`.
  * Them heartbeat TX trong `CanComm_Task(now_ms)` theo `CONFIG_CAN_HEARTBEAT_PERIOD_MS`.
  * Heartbeat TX non-blocking qua `HAL_CAN_AddTxMessage()`.
  * Neu TX fail thi tang `tx_error_count`, khong block, thu lai o chu ky heartbeat sau.
  * Heartbeat va status dung chung helper global status nen `Data[4]` dong nhat.
* Khong thay doi layout command frame `0x500 + NodeID`.
* Khong thay doi layout status frame `0x510 + NodeID`.
* Khong lam thay doi Lost CAN timeout: Lost CAN chi refresh boi command frame hop le `0x500 + NodeID` tu controller.
* Cap nhat `CanComm_TEST`:
  * Them TX history trong mock HAL CAN de verify status va heartbeat song song.
  * Them Test 38 den Test 51 cho heartbeat.
* Cap nhat `CURRENT_STATUS.md` va `PROJECT_CONTEXT.md`.

### CAN frame summary

```text
0x500 + NodeID: Command frame, Controller -> MCU
0x510 + NodeID: Status frame, MCU -> Controller
0x520 + NodeID: Heartbeat/Diagnostic frame, MCU -> Controller
```

### Heartbeat payload

```text
Data[0] = firmware major version
Data[1] = firmware minor version
Data[2] = firmware patch version
Data[3] = NodeID
Data[4] = global status, same as Status Frame Data[4]
Data[5] = valid command RX counter low byte
Data[6] = CAN TX fail/error counter low byte
Data[7] = heartbeat sequence counter
```

### Behavior hien tai

* Binh thuong, khong fault: status `Data[4] = 0x01`, heartbeat `Data[4] = 0x01`.
* Co group fault: status `Data[4] = 0x03`, heartbeat `Data[4] = 0x03`.
* Lost CAN fail-safe active: status `Data[4] = 0x07`, heartbeat `Data[4] = 0x07`.
* Heartbeat sequence counter tang moi lan heartbeat gui thanh cong va tu wrap 0..255 theo kieu `uint8_t`.
* Heartbeat khong refresh Lost CAN timeout.

### Testcase moi da them

* Test 38: Heartbeat ID NodeID 0 la `0x520`.
* Test 39: Heartbeat ID dung NodeID khac, vi du `0x521`, `0x527`.
* Test 40: Heartbeat khong gui truoc period.
* Test 41: Heartbeat gui dung sau period.
* Test 42: Heartbeat payload co FW version va NodeID.
* Test 43: Heartbeat global status binh thuong la `0x01`.
* Test 44: Heartbeat global status khi group fault la `0x03`.
* Test 45: Heartbeat global status khi Lost CAN fail-safe la `0x07`.
* Test 46: Heartbeat sequence counter tang.
* Test 47: Heartbeat valid command RX counter dung low byte cua valid command count.
* Test 48: Heartbeat TX fail counter phan anh low byte cua TX error count.
* Test 49: Heartbeat khong refresh Lost CAN timeout.
* Test 50: Heartbeat period dung khi tick overflow.
* Test 51: Status va Heartbeat la hai frame doc lap, status layout khong doi.

### Ket qua PC test

```bash
cd /f/Work/Project/MOCK_TEST/CanComm_TEST
cmake -S . -B build -G Ninja
cmake --build build
./build/test_can_comm.exe
./build/test_can_comm_clear_disabled.exe
```

* `CanComm_TEST`: 51 passed, 0 failed.
* `CanComm_TEST` macro disable: 1 passed, 0 failed.

Chay lai toan bo PC test hien co:

* `Feedback_TEST`: 9 passed, 0 failed.
* `Contactor_TEST`: 36 passed, 0 failed.
* `App_TEST`: 6 passed, 0 failed.
* `Integration_TEST`: 10 passed, 0 failed.
* `CanComm_TEST`: 51 passed, 0 failed.
* `test_can_comm_clear_disabled.exe`: 1 passed, 0 failed.

### Ket qua firmware build

```powershell
cd F:\Work\Project\DC_Combiner_MCU_IO_FW
cmake --build build\Debug
```

* Build OK.
* RAM: 1880 B / 20 KB.
* FLASH: 15104 B / 64 KB.

### Board test guide

NodeID = 0:

* Sau boot, khong gui command, status ky vong:

```text
CAN ID = 0x510
Data   = 00 00 00 00 01 00 00 00
```

* Heartbeat ky vong:

```text
CAN ID = 0x520
DLC    = 8
Data   = 01 00 00 00 01 xx xx seq
```

Trong do:

* `Data[5]` la valid command RX counter low byte.
* `Data[6]` la CAN TX fail/error counter low byte.
* `Data[7]` tang moi heartbeat.

Lost CAN:

* Gui ON G1 va feedback dung, sau do ngung gui command qua `CONFIG_CAN_COMMAND_TIMEOUT_MS`.
* Status `0x510` ky vong `Data[4] = 0x07`.
* Heartbeat `0x520` ky vong `Data[4] = 0x07`.

NodeID khac:

* NodeID = 1: Command ID `0x501`, Status ID `0x511`, Heartbeat ID `0x521`.

### File lien quan

* `Core/Inc/config.h`
* `Core/Src/can_comm.c`
* `CURRENT_STATUS.md`
* `PROJECT_CONTEXT.md`
* `TASK_LOG.md`
* `F:\Work\Project\MOCK_TEST\CanComm_TEST\test_can_comm.c`
* `F:\Work\Project\MOCK_TEST\CanComm_TEST\mocks\mock_hal_can.c`
* `F:\Work\Project\MOCK_TEST\CanComm_TEST\mocks\mock_hal_can.h`

### Pham vi chua test

* Chua test Heartbeat/Diagnostic frame tren board that voi USB-CAN-B.

## 2026-06-26 - Refactor Heartbeat/Diagnostic sang diagnostic.h/.c

### Muc tieu

Tach phan build payload va heartbeat sequence counter cua frame `0x520 + NodeID` ra khoi `can_comm.c`, khong thay doi CAN ID, payload, period, Lost CAN, Clear Fault hay status frame.

### Viec da lam

* Code `Core/Inc/diagnostic.h`:
  * Them `diagnostic_heartbeat_input_t`.
  * Them API `Diagnostic_Init()`.
  * Them API `Diagnostic_BuildHeartbeatPayload()`.
  * Them API `Diagnostic_OnHeartbeatTxSuccess()`.
  * Them API `Diagnostic_GetHeartbeatSequence()`.
* Code `Core/Src/diagnostic.c`:
  * Quan ly `heartbeat_sequence`.
  * `Diagnostic_Init()` reset sequence ve 0.
  * `Diagnostic_BuildHeartbeatPayload()` build dung 8 byte payload heartbeat.
  * `Diagnostic_OnHeartbeatTxSuccess()` tang sequence chi khi heartbeat TX thanh cong.
  * Sequence tu wrap 255 -> 0 theo `uint8_t`.
* Refactor `Core/Src/can_comm.c`:
  * Remove `heartbeat_sequence` khoi `can_comm.c`.
  * Goi `Diagnostic_Init()` trong reset state CAN.
  * Tao `diagnostic_heartbeat_input_t` khi gui heartbeat.
  * Goi `Diagnostic_BuildHeartbeatPayload()` truoc khi `HAL_CAN_AddTxMessage()`.
  * Goi `Diagnostic_OnHeartbeatTxSuccess()` sau khi heartbeat TX thanh cong.
* Cap nhat root `CMakeLists.txt` de include `Core/Src/diagnostic.c`.
* Cap nhat `F:\Work\Project\MOCK_TEST\CanComm_TEST\CMakeLists.txt` de link that `diagnostic.c`.
* Tao `F:\Work\Project\MOCK_TEST\Diagnostic_TEST` de test rieng module `diagnostic`.

### Module responsibility sau refactor

* `can_comm.c`: CAN filter/start/RX/TX, command/status/heartbeat ID, heartbeat period, HAL CAN TX.
* `diagnostic.c`: heartbeat payload 8 byte va heartbeat sequence counter.
* `diagnostic.c` khong goi `HAL_CAN_AddTxMessage()`, khong include `can.h`, khong goi `BSP_GPIO_*`, khong goi `Contactor_*`.

### Behavior giu nguyen

* Command frame `0x500 + NodeID` khong doi.
* Status frame `0x510 + NodeID` khong doi.
* Heartbeat frame `0x520 + NodeID` khong doi.
* Heartbeat payload khong doi:

```text
Data[0] = firmware major version
Data[1] = firmware minor version
Data[2] = firmware patch version
Data[3] = NodeID
Data[4] = global status, same as Status Frame Data[4]
Data[5] = valid command RX counter low byte
Data[6] = CAN TX fail/error counter low byte
Data[7] = heartbeat sequence counter
```

* Clear Fault qua CAN khong doi.
* Lost CAN timeout khong doi.
* Heartbeat khong refresh Lost CAN timeout.

### Ket qua PC test

```bash
cd /f/Work/Project/MOCK_TEST/Diagnostic_TEST
cmake -S . -B build -G Ninja
cmake --build build
./build/test_diagnostic.exe
```

* `Diagnostic_TEST`: 6 passed, 0 failed.

```bash
cd /f/Work/Project/MOCK_TEST/CanComm_TEST
cmake -S . -B build -G Ninja
cmake --build build
./build/test_can_comm.exe
./build/test_can_comm_clear_disabled.exe
```

* `CanComm_TEST`: 51 passed, 0 failed.
* `CanComm_TEST` macro disable: 1 passed, 0 failed.

Chay lai toan bo PC test hien co:

* `Feedback_TEST`: 9 passed, 0 failed.
* `Contactor_TEST`: 36 passed, 0 failed.
* `App_TEST`: 6 passed, 0 failed.
* `Integration_TEST`: 10 passed, 0 failed.
* `CanComm_TEST`: 51 passed, 0 failed.
* `test_can_comm_clear_disabled.exe`: 1 passed, 0 failed.
* `Diagnostic_TEST`: 6 passed, 0 failed.

### Ket qua firmware build

```powershell
cd F:\Work\Project\DC_Combiner_MCU_IO_FW
cmake --build build\Debug
```

* Build OK.
* RAM: 1888 B / 20 KB.
* FLASH: 15296 B / 64 KB.

### File lien quan

* `Core/Inc/diagnostic.h`
* `Core/Src/diagnostic.c`
* `Core/Src/can_comm.c`
* `CMakeLists.txt`
* `CURRENT_STATUS.md`
* `PROJECT_CONTEXT.md`
* `TASK_LOG.md`
* `F:\Work\Project\MOCK_TEST\Diagnostic_TEST`
* `F:\Work\Project\MOCK_TEST\CanComm_TEST\CMakeLists.txt`

## 2026-06-26 - Tao bao cao ky thuat firmware MCU I/O

### Muc tieu

Doc source code hien tai, test harness va Word spec `F:\Work\Project\MCU_IO_Module_Spec_Detailed R1.0.docx`, sau do viet bao cao ky thuat Markdown cho firmware `DC_Combiner_MCU_IO_FW`.

### Viec da lam

* Doc Word spec ban dau va doi chieu voi source code hien tai.
* Doc cac module firmware chinh: `config`, `app`, `bsp_gpio`, `feedback`, `contactor`, `can_comm`, `diagnostic`, `fault`, `main`, `gpio`, `can`.
* Chay lai PC tests:
  * `Feedback_TEST`: 9 passed, 0 failed.
  * `Contactor_TEST`: 36 passed, 0 failed.
  * `App_TEST`: 6 passed, 0 failed.
  * `Integration_TEST`: 10 passed, 0 failed.
  * `Diagnostic_TEST`: 6 passed, 0 failed.
  * `CanComm_TEST`: 51 passed, 0 failed.
  * `test_can_comm_clear_disabled.exe`: 1 passed, 0 failed.
* Build firmware bang `cmake --build build\Debug --clean-first`: build OK, RAM 1888 B, FLASH 15296 B.
* Tao file bao cao:
  * `DC_Combiner_MCU_IO_FW_Report.md`.

### Noi dung chinh trong bao cao

* Tong quan project MCU I/O board DC Combiner 6 group.
* Mapping output, feedback, DIP NodeID.
* Feedback debounce va contactor state machine.
* CAN command/status/heartbeat protocol hien tai.
* Clear Fault qua CAN, Lost CAN fail-safe va heartbeat diagnostic.
* Bang testcase va ket qua pass.
* Huong dan Controller code theo.
* Diem khac biet so voi Word spec ban dau, dac biet status frame dung 2 byte feedback raw.
