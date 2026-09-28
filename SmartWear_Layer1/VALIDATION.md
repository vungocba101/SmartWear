# Kết quả kiểm tra trước bàn giao

Ngày: 28/09/2026.

| Kiểm tra đã thực hiện | Kết quả |
|---|---|
| Đọc sheet `Trang tính1` và tính lại các dòng có đơn giá | 1.175.149 VNĐ; còn hai dòng thiếu đơn giá |
| So sánh `Config.h` và `Sensing.h` giữa hai sketch | Nội dung giống nhau, biến thể chọn bằng `CAP_NODE` |
| Kiểm tra tập chân camera AI Thinker so với I2C MPU cap | Không trùng GPIO13/14 |
| Kiểm tra I2C wrist so với force pins | GPIO21/22 không trùng GPIO32–35, force đều thuộc ADC1 |
| Biên dịch hàm `formatSample` trích nguyên từ source bằng g++ C++11, `-Wall -Wextra -Werror` cho cả hai nhánh | Thành công; đây chỉ là host test hàm format, **không phải biên dịch firmware ESP32** |
| Parse JSON đầu ra với Epoch millisecond 13 chữ số, seq=4294967295, giá trị IMU âm và ADC=4095 | Đúng schema, int64 timestamp không bị mất chữ số; wrist 137 B, cap 107 B, nhỏ hơn buffer JSON 256 B và MQTT 384 B kể cả topic/header |
| Rà logic ISR và framebuffer | ISR không đọc bus/ADC/MQTT; có đường trả framebuffer sau send và lỗi |

Chưa thực hiện: cross-compile bằng Arduino-ESP32 3.0.7, flash board, kiểm tra driver/hardware SCCB thực tế, đo timer jitter, sai số NTP/camera alignment, FPS, rò bộ nhớ khi chạy lâu, dòng nguồn, cân khối lượng. Các phép thử và tiêu chí nằm ở mục 10 của README.

Không diễn giải kết quả host test hoặc rà mã thành chứng nhận 30 FPS, <10 ms, <40 g hay BOM cuối cùng dưới 1.500.000 VNĐ.
