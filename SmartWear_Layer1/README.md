# SmartWear AI — Layer 1: Sensing Layer

Ngày bàn giao: 28/09/2026. Nguồn phần cứng duy nhất: `Bao_Cao_Mua_Linh_Kien_Phan_Cung.xlsx`, sheet `Trang tính1`, dòng 2–12. Không sửa file BOM gốc. Tài liệu hãng được dùng để kiểm tra giao tiếp/API, không dùng để thay thế hay tự bổ sung danh mục mua hàng.

## 1. Kết luận thiết kế và những điểm chưa thể nghiệm thu

Đây là bộ firmware tham chiếu đầy đủ cho hai sketch Arduino độc lập, pinout và quy trình triển khai. **Chưa thể khẳng định hệ thống đáp ứng toàn bộ yêu cầu chỉ dựa vào BOM.** Các điểm cần xác nhận:

| Yêu cầu | Thiết kế hiện tại / điều kiện |
|---|---|
| Chỉ thu thập và truyền dữ liệu | Không AI, không fusion, không SD/SPIFFS, không ghi dữ liệu cảm biến vào flash. Chỉ có buffer truyền tạm trong RAM/PSRAM. |
| ESP32-CAM + OV2640 | BOM có linh kiện này nhưng không ghi biến thể PCB. Pinout bên dưới chỉ đúng nếu bo tương thích AI Thinker và có PSRAM hoạt động. |
| 4 kênh lực | BOM ghi “sEMG cơ bắp / FSR402”; cần xác nhận là bốn kênh FSR có mạch chuyển điện trở sang điện áp. Firmware này **không phải firmware sEMG**. |
| Timer 50 Hz | Timer phần cứng 1 MHz, báo ngắt mỗi 20.000 µs; timestamp chốt trong ISR. I2C/ADC thực hiện trong task được đánh thức ngay sau ISR. Không lấy timestamp khi publish. |
| Thời điểm chuyển đổi ADC | ADC one-shot được đọc sau ngắt, không phải DMA chuyển đổi đồng thời bốn kênh. Giới hạn trễ được kiểm tra; xem mục 6. |
| Sai lệch <10 ms | Là tiêu chí đo nghiệm thu, chưa được bảo đảm bởi `configTime()`. Không có PPS/PTP hay đường trigger đồng bộ trong BOM. |
| Video ≥30 FPS | QVGA JPEG, hai framebuffer PSRAM, lấy frame mới, Wi-Fi không sleep. Đây là cấu hình hướng tới mục tiêu; không bảo đảm 30 FPS liên tục trong mọi ánh sáng/mạng. |
| Cụm trán <40 g | BOM không có khối lượng. Chỉ đặt camera + GY-521 + gá/dây ở trán, chuyển pin/nguồn ra sau mũ hoặc túi. Phải cân cụm hoàn chỉnh. |
| Tổng BOM <1.500.000 VNĐ | Giá đã biết 1.175.149 VNĐ; hai dòng thiếu đơn giá, chưa tính phí giao hàng và hạng mục thiếu. Chưa thể chốt tổng. |

`HARDWARE_CONFIRMED=false` ở cả hai sketch: firmware biên dịch được nhưng dừng ở Serial trước khi chạy cảm biến/camera. Chỉ đổi thành `true` sau khi kiểm tra đúng phần cứng. Đây không phải yêu cầu mua thêm; nó tránh áp pinout giả định lên bo chưa được định danh.

**Giới hạn tín hiệu:** 50 Hz có Nyquist 25 Hz; không đủ để gọi là thu thập mọi cử động vi mô hay sEMG thô. sEMG cần một cấu hình analog/băng thông/tốc độ khác mà BOM chưa mô tả. MPU6050 không tự xuất góc nghiêng; ở Layer 1 chỉ gửi gia tốc và vận tốc góc, xử lý góc để Layer 2 làm. FSR đo tác động tiếp xúc, không trực tiếp đo hoạt động cơ và không tự cho lực Newton.

## 2. Trích xuất BOM và ngân sách

Các thành tiền dưới đây tính lại bằng số lượng × đơn giá, không dựa vào cache công thức Excel. Dòng trống không được xem là giá 0 đã xác nhận.

| Dòng Excel | Linh kiện nguyên văn | SL | Đơn giá (VNĐ) | Thành tiền đã biết (VNĐ) | Trạng thái |
|---|---|---:|---:|---:|---|
| 2 | ESP32-CAM (Kèm Camera OV2640 2MP) | 1 | 280.152 | 280.152 | Pending |
| 3 | Bo mạch ESP32-WROOM-32D (Dual-Core) | 1 | Chưa điền | Chưa xác định | Done — A Bá có |
| 4 | Cảm biến IMU 6-trục MPU6050 (GY-521) | 2 | 89.665 | 179.330 | Pending |
| 5 | Cảm biến Lực sEMG cơ bắp / FSR402 | 4 | 94.100 | 376.400 | Pending |
| 6 | Pin LiPo 3.7V 1000mAh (Nhỏ gọn) | 2 | 75.155 | 150.310 | Not Yet |
| 7 | Mạch Sạc TP4056 + Step-Up 5V | 1 | 40.653 | 40.653 | Pending |
| 8 | Phụ kiện: Công tắc. Dây cắm Dupont. Quick-clip | 1 | Chưa điền | Chưa xác định | Pending |
| 9 | Breadboard 400 lỗ | 3 | 27.948 | 83.844 | Pending |
| 10 | Dây cắm đực - đực | 1 | 33.460 | 33.460 | Pending |
| 11 | Dây cắm đực - cái | 1 | 16.000 | 16.000 | Pending |
| 12 | Công tắc nút bấm | 1 | 15.000 | 15.000 | Pending |
| | **Tổng phần đã có giá** | | | **1.175.149** | |

Khoảng còn lại tới trần là **324.851 VNĐ**. Vì yêu cầu là *dưới* 1.500.000, tổng tất cả khoản chưa biết phải **nhỏ hơn 324.851 VNĐ**. Nếu tính theo tiền mua mới, WROOM đã có có thể ghi nhận 0 chi mới sau khi chủ dự án xác nhận. Nếu tính toàn bộ giá trị BOM thì vẫn cần giá bo này.

Không âm thầm cộng linh kiện ngoài BOM. Những thiếu sót cần xử lý khi chốt mua:

- FSR402 rời có hai chân là điện trở biến thiên; không thể nối một chân ADC rồi mong có số đo ổn định. BOM không ghi điện trở đo hoặc tên mạch điều hòa analog. Nếu gói mua có sẵn chúng, cần xác nhận. Nếu không, thiết kế 4 kênh lực còn thiếu phần cứng và phải sửa BOM trước khi lắp.
- Một TP4056/step-up chỉ đủ cho một nhánh pin như mô tả. Không đủ để mặc định có hai thiết bị chạy hai pin độc lập. Có thể kiểm thử lần lượt hoặc dùng một nhánh nguồn chung qua dây **chỉ khi** thông số dòng đã được xác nhận; không ghép hai LiPo để “bù” thiếu mạch nguồn.
- Chưa thấy USB–UART/đế nạp CAM, nguồn thử, đồng hồ đo và dụng cụ cân trong BOM. Nếu đã có hoặc mượn thì là dụng cụ triển khai; nếu cần mua phải đưa chi phí vào ngân sách phù hợp. WROOM-32D là tên module; chưa xác nhận bo đang có là DevKit USB hay module trần.
- “Công tắc nút bấm” không nhất thiết là công tắc nguồn tự giữ hoặc chịu được dòng tải; không tự gán cho nó chức năng ngắt nguồn.

## 3. Pinout — Smart-Wristband

Áp dụng bo ESP32 nguyên bản, module WROOM-32D, GPIO được đưa ra header. Số ghi dưới đây là **GPIO**, không phải số thứ tự chân trên hàng header.

| Chân cảm biến/nguồn | Nối tới WROOM | Giao tiếp | Ghi chú |
|---|---|---|---|
| GY-521 VCC | 3V3 | Nguồn | Xác nhận module hoạt động với cấp 3,3 V; pull-up SDA/SCL chỉ lên 3,3 V |
| GY-521 GND | GND | Nguồn | Mass chung với analog |
| GY-521 SDA | GPIO21 | I2C0 SDA, 400 kHz | `Wire` |
| GY-521 SCL | GPIO22 | I2C0 SCL, 400 kHz | `Wire` |
| GY-521 AD0 | GND | Địa chỉ | `0x68` |
| GY-521 INT | Không nối | — | Thiết kế dùng timer; không tuyên bố có hardware trigger MPU |
| GY-521 XDA/XCL | Không nối | — | Không dùng auxiliary bus |
| FSR kênh 0, analog OUT | GPIO32 | ADC1_CH4 | `force[0]`, ngón cái theo quy ước lắp |
| FSR kênh 1, analog OUT | GPIO33 | ADC1_CH5 | `force[1]`, ngón trỏ |
| FSR kênh 2, analog OUT | GPIO34 | ADC1_CH6 | `force[2]`, ngón giữa; GPIO chỉ input |
| FSR kênh 3, analog OUT | GPIO35 | ADC1_CH7 | `force[3]`, ngón áp út; GPIO chỉ input |
| 4 mạch analog VCC | 3V3 | Nguồn | Chỉ nếu đúng module hỗ trợ 3,3 V |
| 4 mạch analog GND | GND | Nguồn | Không để OUT floating |
| Nguồn 5 V ổn định | Chân 5V/VIN của **DevKit** | Nguồn | Chỉ khi tài liệu bo xác nhận nhận 5 V; module WROOM trần không nhận 5 V |

ADC1 được chọn để tránh xung đột ADC2 với Wi-Fi. Không SPI ở wrist. GPIO34/35 không có pull-up/down nội dùng được cho mạch chia áp FSR.

### Nếu linh kiện là FSR402 rời

Mạch cần có quan hệ nối: `3V3 → FSR → nút đo → điện trở đo → GND`; nút đo nối ADC. Mỗi kênh cần một điện trở đo riêng. Công thức `Vout = 3.3 × Rđo / (RFSR + Rđo)`. Giá trị Rđo phải chọn theo lực làm việc và dải ADC; **BOM hiện chưa có giá trị này nên không có phương án nối trực tiếp hoàn chỉnh được xác nhận**. Không tận dụng pull-down nội để thay điện trở đo, không coi sEMG là FSR.

Đối với mạch FSR đã có chia áp: xác nhận chân OUT là analog (AO), không dùng DO/comparator. Đo OUT trước khi cắm ESP32: không vượt 3,3 V; cố gắng giữ dải hữu ích dưới khoảng 3,1 V để tránh bão hòa ADC 11 dB trên ESP32. Raw 4095 có thể là clipping, không đồng nghĩa lực đã đạt một trị số Newton. Không dùng nguồn 5 V cho module có đầu ra analog 5 V.

## 4. Pinout — Smart-Cap

**Chỉ dùng sau khi đối chiếu bo thực tế với AI Thinker ESP32-CAM.** Không thể suy ra pinout chính xác chỉ từ tên “ESP32-CAM” trong Excel.

| Chân GY-521 | ESP32-CAM | Ghi chú |
|---|---|---|
| VCC | 3V3 | Kiểm tra nguồn và pull-up 3,3 V trên module |
| GND | GND | Mass chung |
| SDA | GPIO13 | I2C0, 400 kHz |
| SCL | GPIO14 | I2C0, 400 kHz |
| AD0 | GND | `0x68`; hai MPU ở hai board nên không xung đột địa chỉ |
| INT, XDA, XCL | Không nối | Không dùng |

Tháo thẻ microSD, không gọi SD/SD_MMC. GPIO13/14 được dùng cho IMU nên không dùng thẻ. Không dùng GPIO12 do rủi ro strap điện áp flash; không lấy GPIO0 làm I2C vì nó vừa là boot strap vừa là camera XCLK. Không dùng GPIO16/17 vì PSRAM trên biến thể này. GPIO4 là LED flash, không bật trong firmware.

Camera OV2640 nối bằng cáp FPC có sẵn; bảng dưới để kiểm tra cấu hình, **không đi dây Dupont vào bus camera**:

| Tín hiệu OV2640 | GPIO ESP32-CAM |
|---|---:|
| D0 / Y2 | 5 |
| D1 / Y3 | 18 |
| D2 / Y4 | 19 |
| D3 / Y5 | 21 |
| D4 / Y6 | 36 |
| D5 / Y7 | 39 |
| D6 / Y8 | 34 |
| D7 / Y9 | 35 |
| XCLK | 0 |
| PCLK | 22 |
| VSYNC | 25 |
| HREF | 23 |
| SCCB SDA | 26 |
| SCCB SCL | 27 |
| PWDN | 32 |
| RESET | -1, không điều khiển bằng GPIO |

OV2640 dùng bus pixel song song và SCCB cấu hình, không SPI. IMU dùng I2C controller 0 (`Wire`); camera dùng SCCB controller 1 theo cấu hình camera đi kèm core được nhắm tới. Khi đổi core phải kiểm tra `CONFIG_SCCB_HARDWARE_I2C_PORT` và log khởi tạo, tránh camera và IMU chiếm cùng controller.

## 5. Nguồn và bố trí cơ khí

Chưa có mã chính xác của step-up, dòng ra, loại bảo vệ pin, dòng sạc đặt trên TP4056, nên không thể duyệt nguồn đeo hoàn chỉnh.

Một nhánh nguồn hợp lệ về cấu trúc: **một cell LiPo 1S → mạch sạc/bảo vệ đã xác nhận → step-up đặt đúng 5,0 V → đầu vào 5 V của board phù hợp**. Với TP4056 có bảo vệ, pin vào B+/B−, tải dùng OUT+/OUT− theo đúng nhãn; nếu không có chân/khối bảo vệ tương ứng thì phải đọc sơ đồ module, không suy đoán. TP4056 tự nó không phải bộ tăng áp và không mặc định có power-path để vừa sạc vừa dùng.

Trước cấp nguồn: kiểm tra cực pin, đo 5,0 V đầu ra khi chưa nối board, xác nhận nguồn chịu được đỉnh dòng Wi-Fi/camera; lấy mức 5 V/1 A mỗi bo làm mục tiêu nguồn thử có dự phòng, không coi đây là thông số đã biết của step-up trong BOM. Không cấp LiPo trực tiếp vào 3V3; không đưa 5 V vào GPIO; không ghép song song/nối tiếp hai cell với một bộ sạc 1S. Dòng sạc cần khớp datasheet cell, không mặc định 1 A chỉ vì module có thể được đặt như vậy. Tháo nguồn/pin khi đổi dây; không sạc khi đang đeo. Không vô hiệu hóa brownout để che nguồn yếu.

Đặt ESP32-CAM + OV2640 + GY-521 ở trán, gá sát và đánh dấu hướng trục MPU. Pin, step-up, sạc và phần dây dư đặt ra sau mũ/túi, cố định chống kéo. Không mang breadboard 400 lỗ lên trán. Cân **toàn bộ cụm trán gồm bo, camera/FPC, IMU, dây tại cụm, vỏ, gá/clip**; phải <40 g. Khối lượng chưa có trong BOM nên chưa được điền số ước lượng giả. Nếu quick-clip hiện có không đủ cố định/cách điện thì cần chốt lại phụ kiện và chi phí.

## 6. Firmware, thời gian và bộ nhớ

### Tệp và dependencies

- `SmartWrist/SmartWrist.ino`, `Config.h`, `Sensing.h`.
- `SmartCap/SmartCap.ino`, `Config.h`, `Sensing.h`, `CameraStream.h`.
- Arduino IDE 2.x; Boards Manager **esp32 by Espressif Systems 3.0.7** là phiên bản nhắm tới. Không dùng core 2.x vì API timer khác. Bộ mã này chưa được cross-compile với toolchain Espressif trong môi trường bàn giao.
- Library Manager: **PubSubClient by Nick O'Leary 2.8**.
- `Arduino`, `WiFi`, `Wire`, `esp_camera`, `esp_http_server`, `esp_timer`, `esp_sntp`, FreeRTOS có trong core; không cài thêm thư viện camera trùng tên.
- Không cần ArduinoJson, Adafruit MPU6050, MPU6050_light. MPU được đọc bằng register I2C; JSON dùng `snprintf` có kiểm tra kích thước.

Hai bản `Sensing.h` và `Config.h` giống nhau để mỗi thư mục sketch mở độc lập được trong IDE; sửa cấu hình ở **cả hai** thư mục.

### Luồng thực thi

Khởi tạo Serial → kiểm tra xác nhận hardware → khởi tạo IMU/ADC → (cap: camera + PSRAM) → Wi-Fi → `configTime(0,0,"192.168.4.1")` → chờ callback NTP hợp lệ → tạo task/queue → bật timer → (cap: mở server cổng 81).

Không có mạng/NTP lúc khởi động thì chờ, không phát Epoch giả bằng `millis()`. Sau khi chạy, mất đồng bộ quá 120 giây thì ngừng phát dữ liệu có timestamp, video trả lỗi/đóng phiên. SNTP yêu cầu cập nhật mỗi 60 giây. Tuổi bản sync không chứng minh độ chính xác của sync; đây chỉ là cơ chế chống dùng mốc quá cũ.

ISR đọc đồng hồ monotonic µs và cộng offset Epoch được SNTP cập nhật dưới critical section. Gói giữ nguyên timestamp của ISR. ISR chỉ đặt tick vào queue một phần tử; không I2C, ADC, JSON, MQTT, Serial hay cấp phát bộ nhớ. Task cảm biến core 1 priority 4 đọc MPU một burst 14 byte và ADC tuần tự. Task MQTT core 0 priority 1 có thể bị chặn bởi kết nối TCP mà không chặn task cảm biến. Server MJPEG core 0 priority 2 chỉ phục vụ một consumer Layer 2.

Các ngưỡng kỹ thuật: task thức muộn >2 ms hoặc tổng đọc từ ISR tới hoàn tất >5 ms thì bỏ mẫu; MQTT không phát mẫu đã quá 40 ms khi bắt đầu gửi. Mẫu mới thay mẫu cũ trong queue một phần tử. `seq` tăng mỗi ngắt, kể cả ngắt không có gói hợp lệ, nên Layer 2 nhận biết khoảng mất. Không gửi bù hàng loạt, không ghi backlog; TCP vẫn có thể làm gói tới muộn sau khi publish bắt đầu, Layer 2 phải dùng timestamp để loại mẫu quá hạn.

### Điều timestamp bảo đảm và không bảo đảm

`t_ms = (monotonic_us_tại_ISR + epoch_offset_us)/1000`. Điều này gắn nhãn **thời điểm phục vụ ngắt**, không biến I2C/ADC thành cảm biến được trigger đồng thời. Ngắt có thể đến trễ khi CPU mask interrupt. Kiểm tra trễ task không đo được toàn bộ trễ phục vụ ISR.

MPU đặt ±4g, ±500 °/s, DLPF=1, SMPLRT_DIV=0, cập nhật thanh ghi nội bộ 1 kHz rồi poll 50 Hz. Mục tiêu là giảm tuổi mẫu tự do so với đặt MPU tự chạy 50 Hz không khóa pha với timer. DLPF này không chống alias ở Nyquist 25 Hz. Muốn vừa giữ băng thông cử động vi mô vừa chống alias có thể phải tăng sample rate/hợp đồng Layer 2; không được tự đổi yêu cầu 50 Hz rồi tuyên bố tương thích. Firmware không tích phân, không hiệu chỉnh bias, không lọc thêm hoặc đổi hệ trục; chuyển scale LSB sang đơn vị trong hợp đồng JSON.

Sai số thực tế gồm NTP/asymmetry Wi-Fi, trôi đồng hồ giữa hai lần sync, trễ IRQ/task, tuổi/group delay mẫu MPU, khoảng đọc ADC giữa bốn kênh và timestamp camera. **Không thể chứng minh tổng <10 ms chỉ bằng timer 20 ms và `configTime()`.** Có thể đánh giá đạt trong cấu hình thử cụ thể; nếu hợp đồng đòi hard guarantee mọi tình huống thì BOM/giao thức hiện tại chưa đủ.

SNTP có thể step đồng hồ. Firmware bỏ mẫu đi lùi so với mốc đã publish; bỏ mẫu đang chờ nếu thế hệ clock thay đổi. Sau hiệu chỉnh lùi lớn có thể tạm ngừng gói tới khi Epoch vượt mốc cũ; không clamp timestamp vì sẽ tạo nhãn sai. Layer 2 cần xử lý discontinuity, reconnect và reset `seq`; `seq` là uint32, vòng lại sau khoảng 994 ngày ở 50 Hz, reset về 0 khi reboot. Hai thiết bị không có cùng pha timer chỉ vì dùng chung NTP.

### RAM/PSRAM và 30 FPS

JPEG được OV2640 tạo sẵn, QVGA 320×240, quality=15, XCLK 20 MHz, `fb_count=2`, `CAMERA_FB_IN_PSRAM`, `CAMERA_GRAB_LATEST`. Không chuyển JPEG sang RGB hoặc base64, không copy nguyên frame sang SRAM, luôn trả framebuffer khi gửi xong hoặc lỗi. Chưa có PSRAM thì dừng, không fallback âm thầm sang SRAM.

Các vùng chủ động nhỏ: hai task sensor/MQTT mỗi stack 4 KiB; HTTP stack 6 KiB ở cap; MQTT buffer 384 B; JSON 256 B; một Tick và một Sample trong hai queue. Wi-Fi/TCP, camera DMA và runtime cần SRAM riêng nên không thể từ các số này suy ra toàn hệ thống chắc chắn không tràn. `heap`, `minHeap`, `psramFree` được log để đo. Không tạo task hay queue mới mỗi lần reconnect.

30 FPS là đầu ra cần đo ở gateway; không dùng `delay(33)` để giả lập, không gửi lặp frame. Ánh sáng yếu làm tăng exposure; JPEG lớn và Wi-Fi yếu làm giảm throughput. Nếu không đạt, thử ánh sáng ổn định, AP gần, chỉ một consumer; có thể tăng số quality để giảm kích thước JPEG hoặc thử QQVGA trong một profile được Layer 2 chấp thuận. Không tuyên bố QVGA/2MP đạt 30 FPS chỉ vì camera quảng cáo 2MP.

## 7. Hợp đồng dữ liệu Layer 2

| Thiết bị/dịch vụ | Địa chỉ mặc định |
|---|---|
| Gateway / AP, broker, NTP | 192.168.4.1 |
| Smart-Cap | 192.168.4.21 |
| Smart-Wrist | 192.168.4.22 |
| Subnet | 255.255.255.0 |
| MQTT | TCP 1883, QoS 0, retain=false |
| NTP | UDP 123, gateway offline |
| MJPEG | `http://192.168.4.21:81/stream` |

Wi-Fi SSID/password xác định mạng cần kết nối; IP gateway không thay được SSID/password. AP phải hỗ trợ 2,4 GHz, không client isolation chặn peer/gateway; loại `.21/.22` khỏi DHCP pool để tránh trùng IP.

Topic wrist: `wearable/user01/wrist/data`

```json
{"t_ms":1727443800125,"seq":1042,"acc":[0.02,0.98,0.15],"gyro":[-1.2,0.5,0.1],"force":[1240,890,450,110]}
```

Topic cap: `wearable/user01/cap/imu`

```json
{"t_ms":1727443800120,"seq":891,"acc":[0.01,0.02,0.99],"gyro":[0.0,0.1,-0.1]}
```

Đây là ví dụ schema, không phải timestamp hardcode. `t_ms`: int64 Epoch UTC millisecond; `seq`: số tick kể từ boot; `acc`: [X,Y,Z] theo trục cảm biến, g; `gyro`: [X,Y,Z], °/s; `force`: bốn mã ADC 12 bit 0–4095, không phải Newton. Layer 2 cần giữ metadata scale, hướng lắp, mapping ngón và calibration bên ngoài topic data. Chưa có fifth-finger channel trong BOM. Một MPU cổ tay không đo độc lập góc tất cả khớp ngón.

PubSubClient publish dùng QoS 0; tham số `false` là **retain**, không phải QoS. QoS của subscriber không nâng được QoS publisher. Mạng tắc có thể mất gói; 50 Hz là lịch lấy mẫu, không bảo đảm mọi gói đến broker đúng khoảng 20 ms.

### Timestamp video bắt buộc cho alignment

Mỗi part MJPEG có `Content-Length`, `X-Timestamp-Ms` và `X-Frame-Seq`. Lấy timestamp monotonic của `camera_fb_t`, chuyển bằng offset NTP sang Epoch; không dùng thời điểm HTTP send/receive. Camera driver định nghĩa timestamp ở đầu quá trình nhận frame/DMA, **không phải chính xác tâm phơi sáng của mọi pixel**. Rolling shutter/exposure và pipeline cần đo trước khi claim alignment <10 ms.

Layer 2 phải parse multipart header theo từng frame và giữ `X-Timestamp-Ms`; OpenCV `VideoCapture` thông thường có thể bỏ các header này. `X-Frame-Seq` reset khi kết nối HTTP mới và đếm frame gửi, không đại diện mọi frame vật lý bị driver bỏ. Gateway chỉ một kết nối stream; khi mất Wi-Fi, NTP hết hạn hoặc clock đi lùi, phải mở lại stream. Ring buffer căn theo timestamp nguồn, không theo thứ tự đến mạng. Việc thêm header không đổi định dạng JSON hay topic.

Offline NTP cần đồng hồ gateway được đặt đúng và NTP thực sự trả lời trên UDP 123 cho subnet. `configTime` không làm cho đồng hồ gateway sai trở thành đúng. Cùng sai một Epoch có thể vẫn tương đối gần nhau nhưng không đạt độ chính xác thời gian tuyệt đối.

## 8. Cài đặt, sửa cấu hình và flash

1. Giải nén toàn bộ gói; giữ tên thư mục trùng với `.ino` (`SmartWrist`, `SmartCap`).
2. Trong Arduino IDE, thêm Boards Manager URL của Espressif: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`. Cài core 3.0.7 và PubSubClient 2.8.
3. Mở từng `.ino`. Trong `Config.h` của **cả hai** sketch, sửa `WIFI_SSID`, `WIFI_PASSWORD`, `MQTT_USER`, `MQTT_PASSWORD` theo broker. Với subnet mặc định, giữ `GATEWAY_HOST="192.168.4.1"`. Nếu đổi subnet, sửa cả `IPAddress ip/gateway/mask` trong `Sensing.h` của hai sketch và URL in ra trong `CameraStream.h`.
4. Đối chiếu pinout thật, PSRAM, nguồn, FSR AO. Chỉ sau đó đặt `HARDWARE_CONFIRMED=true` cho thiết bị đã kiểm tra.
5. WROOM DevKit: chọn **ESP32 Dev Module**, CPU 240 MHz, flash size theo bo (không suy ra dung lượng từ tên WROOM), partition có app đủ lớn, PSRAM disabled nếu không có; upload speed 115200 để bắt đầu. Nếu module trần, chưa được cắm 5 V hay giả định có USB; cần xác nhận mạch nguồn/boot/programming thực tế trước.
6. CAM AI Thinker: chọn **AI Thinker ESP32-CAM**, CPU 240 MHz, PSRAM bật theo profile, flash/partition theo bo (thường profile 4 MB, chọn Huge APP nếu cần). Không chọn ESP32-S3/C3 thay thế.
7. Nhấn **Verify** cho cả hai sketch trước Upload. Lưu log biên dịch, phiên bản core, library, lựa chọn board. Môi trường bàn giao không có `arduino-cli`/toolchain ESP32, nên bước Verify thật này vẫn là cổng nghiệm thu bắt buộc.

### Nạp ESP32-CAM bằng USB–UART nếu có sẵn

| USB–UART / thao tác | ESP32-CAM |
|---|---|
| TX logic 3,3 V | U0R / GPIO3 |
| RX logic 3,3 V | U0T / GPIO1 |
| GND | GND chung |
| Nguồn 5 V đủ dòng | Chân 5V của CAM, theo cấu trúc nguồn đã xác nhận |
| GPIO0 nối GND khi reset | Vào bootloader |

Không dùng TX 5 V. Không mặc định đầu ra 3,3 V yếu của USB–UART đủ cấp camera. Không cấp đồng thời hai nguồn 5 V gây backfeed. Tắt nguồn trước khi đấu; nối GPIO0–GND, bật nguồn/reset, Upload; tắt nguồn, tháo cầu GPIO0–GND, bật/reset để chạy camera. Không để GPIO0 bị kéo GND trong chế độ chạy vì camera cần XCLK trên chân đó.

WROOM DevKit có USB: cắm USB, chọn đúng COM, Upload; nếu dừng ở Connecting thì giữ BOOT khi bắt đầu kết nối rồi thả sau khi nạp bắt đầu. Mở Serial Monitor 115200. `STOP` là lỗi cần xử lý, không phải firmware đang stream.

## 9. Trình tự chạy và xử lý lỗi

1. Cho gateway/AP, Mosquitto và NTP chạy trước. Xác nhận broker cho phép đúng user/password trên port 1883; cấu hình broker thuộc Layer 2, firmware không tự chỉnh nó.
2. Chạy wrist trước. Serial phải vượt qua kiểm tra MPU, Wi-Fi, NTP rồi có `pub` tăng khoảng 250 mỗi 5 giây khi mạng khỏe.
3. Ở gateway dùng `mosquitto_sub -h 192.168.4.1 -p 1883 -q 0 -t 'wearable/user01/+/+' -v` (thêm `-u/-P` nếu broker cần). Kiểm tra đủ hai topic, không ép hai `seq` giống nhau.
4. Chạy cap và mở stream một client duy nhất. Khi gateway đã ingest thì không mở thêm browser để tránh chiếm consumer.
5. Ấn lần lượt từng FSR, kiểm tra chỉ đúng vị trí `force[]` thay đổi. Xoay IMU, kiểm tra trục và đơn vị; để tĩnh thì độ lớn acc gần 1g, gyro gần 0 nhưng chưa bù bias.

| Biểu hiện | Việc kiểm tra |
|---|---|
| Dừng `HARDWARE_CONFIRMED` | Đọc và xác nhận bo/mạch analog, sửa config đúng sketch |
| `MPU6050 WHO_AM_I` | SDA/SCL, mass, 3V3, AD0=GND, địa chỉ 0x68; không đổi sang camera SCCB |
| Treo chờ Wi-Fi | SSID/password 2,4 GHz, IP tĩnh, xung đột DHCP |
| Chờ NTP không hết | Gateway time, UDP 123, dịch vụ NTP đang trả lời, firewall |
| `pub=0` dù sync=1 | MQTT credential, listener 1883, client ID trùng với một board khác |
| `late` tăng | CPU/camera load, cáp I2C, log quá nhiều; không bỏ kiểm tra để che lỗi |
| `i2c` tăng | Cáp ngắn, nguồn/pull-up, nhiễu, MPU bị rút; lỗi đọc làm bỏ gói, không gửi dữ liệu cũ |
| PSRAM/camera init lỗi | Board profile, PSRAM, FPC, nguồn; không tắt brownout |
| Force luôn 0/4095 hoặc nhiễu | FSR thiếu chia áp, dùng nhầm DO, OUT vượt dải, ADC floating |
| FPS thấp | Ánh sáng, RSSI, tải Wi-Fi, JPEG size, một consumer; xem cả FPS gateway |

Nếu IMU reset/mất cấu hình do nguồn, khắc phục nguồn và reboot để khởi tạo lại; bản này không có phục hồi tự cấu hình MPU khi đang chạy. Khi broker/mạng mất thì tự thử kết nối lại; không có lưu offline.

## 10. Nghiệm thu phải làm trên thiết bị thật

| Phép thử | Cách đo / tiêu chí |
|---|---|
| Compile và boot | Verify cả hai sketch đúng core/library, ghi log flash/board; boot đúng WHO_AM_I, OV2640, PSRAM; không reset lặp |
| Timer và gói tin | Thu tối thiểu 10 phút ở Layer 2; check schema/units, chuỗi `seq`, `t_ms`. Với seq liên tiếp và không sync step, chênh t_ms quanh 20 ms; phân biệt jitter nguồn và jitter arrival. Mục tiêu lý tưởng 30.000 tick/10 phút mỗi board; ghi tỷ lệ mất thực, không suy từ TCP |
| Mạng đồng thời | Hai MQTT + một MJPEG cùng chạy; đo CPU/heap thấp nhất, heap có suy giảm theo thời gian không; unplug/reconnect AP và broker nhiều lần |
| Timestamp/clock | Đo offset hai board với cùng tham chiếu độc lập, ở đầu/cuối và qua nhiều lần NTP. Tốt nhất đưa cùng tín hiệu tham chiếu vào hai GPIO trống bằng dụng cụ phòng lab, dùng firmware đo riêng chốt timestamp, so với gateway/reference; không chỉ trừ thời gian nhận MQTT |
| Align vật lý video–IMU–lực | Ghi sự kiện cơ học/quang chung với reference đủ nhanh; so timestamp mẫu và frame kèm exposure/rolling shutter. Video 30 FPS cách nhau 33,3 ms nên không thể dùng mỗi quan sát mắt trên video để chứng minh sai lệch <10 ms. Dụng cụ tham chiếu là thiết bị đo mượn/có sẵn, không giả định nằm trong BOM |
| 30 FPS | Đếm JPEG hoàn chỉnh tại gateway mỗi cửa sổ 1/10/60 giây, đo frame timestamp/gap và ánh sáng thực. Nếu bất kỳ tiêu chí FPS tối thiểu đã thống nhất không đạt thì đánh dấu không đạt, không chỉ báo trung bình cao |
| NTP fail | Chặn UDP 123: dữ liệu timestamp ngừng sau TTL 120 s; phục hồi rồi sync/gửi lại. Test clock step tiến/lùi trên gateway thử nghiệm, xác nhận Layer 2 xử lý discontinuity |
| ADC và cơ khí | Test zero/ấn từng kênh, clipping, hướng trục, strain relief; cân cụm trán <40 g; chốt mọi khoản giá còn thiếu |

Firmware log `late`, `i2c`, `pubFail`, `pub`, heap và FPS hỗ trợ tìm lỗi; các counter này không thống kê mọi gói mất trong mạng. `seq` tại Layer 2 mới là căn cứ tổng thể cho stream cảm biến.

**Trạng thái bàn giao:** đã đối chiếu dữ liệu Excel, rà pinout/logic/định dạng/giới hạn RAM chủ động. Chưa cross-compile, chưa flash, chưa đo 30 FPS, <10 ms, công suất hoặc <40 g. Không có kết quả bench giả định trong gói này. Để chốt thiết kế sản xuất cần ảnh/mã chính xác của ESP32-CAM, bo WROOM, module FSR và bộ nguồn, cùng kết quả nghiệm thu.

## 11. Tài liệu kỹ thuật đối chiếu

- [Espressif Arduino Timer API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/timer.html): API core 3.x, timer alarm.
- [Espressif migration 2.x → 3.0](https://docs.espressif.com/projects/arduino-esp32/en/latest/migration_guides/2.x_to_3.0.html): thay đổi API timer.
- [Espressif Arduino ADC](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html): raw ADC, độ phân giải, dải attenuation.
- [ESP-IDF System Time](https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32/api-reference/system/system_time.html): SNTP, callback và thay đổi system time.
- [Espressif esp32-camera](https://github.com/espressif/esp32-camera): JPEG/framebuffer/PSRAM; [camera header](https://github.com/espressif/esp32-camera/blob/master/driver/include/esp_camera.h), [camera timestamp implementation](https://github.com/espressif/esp32-camera/blob/master/driver/cam_hal.c), [Kconfig SCCB](https://github.com/espressif/esp32-camera/blob/master/Kconfig). Nhánh master là tài liệu đối chiếu, không thay thế việc khóa core 3.0.7 và kiểm tra driver đi kèm khi build.
- [CameraWebServer camera pins](https://github.com/espressif/arduino-esp32/blob/master/libraries/ESP32/examples/Camera/CameraWebServer/camera_pins.h): đối chiếu profile AI Thinker.
- [PubSubClient repository](https://github.com/knolleary/pubsubclient), [API](https://pubsubclient.knolleary.net/api): publish QoS 0, buffer và timeout.
- [Interlink FSR 400 Series Datasheet](https://www.interlinkelectronics.com/downloads/datasheets/fsr-400-series-datasheet.pdf), [Integration Guide](https://www.interlinkelectronics.com/downloads/integration-guides/fsr-400-series-integration-guide.pdf): FSR cần điện trở đo/mạch đọc, không phải sEMG. Không khẳng định sản phẩm trong link mua là Interlink chính hãng.

