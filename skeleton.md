# Vulkan Skeleton และแผน Feature จาก Mixwell

เอกสารนี้อธิบาย (1) โครง Vulkan เริ่มต้นที่เปิดหน้าต่างได้ และ (2) feature ที่วางแผนไว้
จากการอ่านเปเปอร์ *Mixwell: Sharp 2D Fluid Brushes for Progressive Physics-Based
Mixing* (James & James, ACM TOG 45(4), Article 79, 2026,
<https://doi.org/10.1145/3811312>) ฉบับ `mixwell-2026-paper.pdf`

> ส่วนที่ 2 เป็น **แผน** ที่ยังไม่ได้ implement เลขสมการ/อัลกอริทึมที่อ้างถึงมาจากเปเปอร์
> ที่อ่าน ส่วนตัวเลขประสิทธิภาพของเรายังไม่มี ต้องวัดเองในภายหลัง

## 1. Vulkan skeleton

### สิ่งที่ทำได้ตอนนี้

- เปิดหน้าต่าง 1280x720 (GLFW, ไม่มี OpenGL context) ปรับขนาดได้ ย่อ/ขยายได้
- สร้าง Vulkan 1.3 instance, surface, device, swapchain
- เคลียร์ภาพ swapchain เป็นสีเข้มทุกเฟรมด้วย dynamic rendering แล้ว present
- ปิดด้วยปุ่ม `Esc` หรือปิดหน้าต่าง และ cleanup ทรัพยากรตามลำดับที่ถูกต้อง
- ยังไม่มี shader, pipeline, texture, brush หรือ UI ใดๆ

### โครงไฟล์

| ไฟล์ | หน้าที่ |
| --- | --- |
| `CMakeLists.txt` | build ด้วย C++20, หา Vulkan SDK, ดึง GLFW 3.4 ผ่าน `FetchContent` |
| `src/main.cpp` | อ่าน argument (`--validation`, `--frames N`) และจับ exception |
| `src/app/Window.*` | ครอบ GLFW: หน้าต่าง, event, สร้าง surface, flag resize |
| `src/app/Application.*` | frame loop, command buffer, semaphore/fence, recreate swapchain |
| `src/vulkan/VulkanContext.*` | instance, validation layer, surface, เลือก GPU, logical device, queue |
| `src/vulkan/Swapchain.*` | swapchain และ image view ของทุก image |
| `src/vulkan/VulkanCheck.h` | `vkCheck` โยน exception พร้อมชื่อคำสั่งและ `VkResult` |

โฟลเดอร์ `shaders/` ยังไม่สร้างเพราะยังไม่มี shader (ตาม AGENT.md ไม่สร้างโฟลเดอร์ล่วงหน้า)

### ลำดับการทำงาน

1. `Window` เริ่ม GLFW และสร้างหน้าต่าง
2. `VulkanContext` สร้าง instance (API 1.3, เปิด `VK_LAYER_KHRONOS_validation` เมื่อขอและมีในเครื่อง)
   → สร้าง surface → เลือก GPU → สร้าง device
3. `Swapchain` สร้าง swapchain และ image view
4. `Application` สร้าง resource ต่อเฟรม (2 frames in flight) และ `renderFinished` semaphore
   ต่อ swapchain image
5. ทุกเฟรมใน `drawFrame()`:
   `wait fence → acquire image → reset fence → record → submit → present`
   - record: barrier `UNDEFINED → COLOR_ATTACHMENT_OPTIMAL`, `vkCmdBeginRendering` แบบ clear,
     `vkCmdEndRendering`, barrier → `PRESENT_SRC_KHR`
   - `vkAcquireNextImageKHR` หรือ present คืน `OUT_OF_DATE`/`SUBOPTIMAL`
     หรือหน้าต่างถูก resize จะ recreate swapchain
   - หน้าต่างถูกย่อ (ขนาด 0) จะหยุดรอ event ไม่ render

จุดที่เตรียมไว้ให้ต่อยอด: ใน `Application::recordCommands` ระหว่าง
`vkCmdBeginRendering` และ `vkCmdEndRendering` (มี comment กำกับ)

### การตัดสินใจหลัก

- **Vulkan 1.3 core**: ใช้ dynamic rendering และ synchronization2 จึงไม่ต้องมี render pass/framebuffer
  และ barrier อ่านง่ายกว่า ตัวเลือก GPU ต้องรองรับสอง feature นี้ ไม่เช่นนั้นโปรแกรมแจ้ง error
- **เลือก GPU**: ต้องมี API 1.3, `VK_KHR_swapchain`, และ queue family เดียวที่รองรับทั้ง graphics และ present
  ถ้ามีหลายตัวเลือกจะเลือก discrete GPU (เครื่องนี้เลือก RTX 4060 แทน Intel Iris Xe)
  ยังไม่ขอ optional feature ใดเพิ่ม (ตามหลัก query ก่อนเปิด)
- **Swapchain**: เลือก `B8G8R8A8_SRGB` (สำรอง `R8G8B8A8_SRGB`) color space sRGB nonlinear
  ค่าสีที่เขียนจึงถือเป็น linear และ hardware แปลงเป็น sRGB ให้ ใช้ present mode `FIFO`
  (รองรับทุกเครื่อง, vsync) ซึ่งต้องระวังเมื่อวัด FPS แต่เราจะวัดด้วย GPU timestamp อยู่แล้ว
- **Synchronization**: `imageAvailable` semaphore และ fence ผูกกับ frame-in-flight
  ส่วน `renderFinished` ผูกกับ swapchain image เพราะ presentation อาจยังถือ semaphore
  หลัง fence ของเฟรมนั้นสัญญาณแล้ว ตอน recreate swapchain จะสร้าง semaphore ชุดใหม่หลัง `vkDeviceWaitIdle`
- **Validation**: เปิดเป็นค่าเริ่มต้นใน Debug build ส่วน Release เปิดด้วย `--validation`
  ถ้าไม่พบ layer จะเตือนแล้วทำงานต่อโดยไม่มี validation
- **GLFW ผ่าน FetchContent**: ไม่มี path เฉพาะเครื่อง แต่ configure ครั้งแรกต้องต่อเน็ต
- **RAII**: แต่ละคลาสเป็นเจ้าของ handle ของตัวเองและทำลายใน destructor
  (ถ้า constructor ล้มเหลวกลางทางก็ cleanup ส่วนที่สร้างไปแล้ว)

### Build และรัน

ต้องมี CMake 3.24+, Vulkan SDK 1.3+ (`VULKAN_SDK` ตั้งค่าแล้ว), คอมไพเลอร์ C++20, และ Git
(สำหรับดึง GLFW)

ที่ทดสอบจริงในเครื่องนี้ (Windows, Ninja, clang++ 23 target MSVC):

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ `
  "-DCMAKE_MT=C:/Program Files/LLVM/bin/llvm-mt.exe"
cmake --build build
.\build\FluidBrushCG.exe
```

`CMAKE_MT` จำเป็นเฉพาะเมื่อใช้ clang โดยไม่มี Visual Studio (ไม่มี `mt.exe`)
ถ้ามี Visual Studio ใช้ generator ปกติได้ (ยังไม่ได้ทดสอบ):
`cmake -S . -B build && cmake --build build --config Debug`

ตัวเลือกตอนรัน: `--validation` และ `--frames N` (ออกเองหลัง present ครบ N เฟรม ใช้ทดสอบอัตโนมัติ)

### สิ่งที่ตรวจสอบแล้ว

| รายการ | ผล |
| --- | --- |
| Build Debug ด้วย `-Wall -Wextra -Wpedantic` | ผ่าน ไม่มี warning |
| รัน `--frames 600` พร้อม validation | จบด้วย exit code 0 ไม่มีข้อความ validation |
| อาร์กิวเมนต์ผิด (`--bogus`) | แสดงวิธีใช้ exit code 2 |
| Resize 4 ขนาด, ย่อ, ขยาย แล้วสั่งปิดหน้าต่าง (validation เปิด) | ปิดปกติ ไม่มี validation error |
| รัน Release build, GPU ตัวอื่น (Intel), driver อื่น | **ยังไม่ได้ทดสอบ** |
| การมองเห็นสีที่เคลียร์ด้วยตา | **ยังไม่ได้ตรวจ** (ตรวจจากการไม่มี error เท่านั้น) |

## 2. แผน feature จากเปเปอร์

### สรุปแนวคิดของ Mixwell ที่เกี่ยวข้อง

- **Mixwell brush (§4)**: velocity field ของหัวแปรงทรงกระบอกรัศมี ε ในรูป Kelvinlet
  `K(r) = A(r) I + B(r) r rᵀ` โดย `A = 1 − r(r²+2ε²)/r_ε³`, `B = ε²/(r r_ε³)`,
  `r_ε = sqrt(r²+ε²)` (สมการ 26–27) ใช้งานง่าย ไม่ต้องใช้ elliptic integral
- **Reverse-Drift Function, RDF (§5)**: field ของการเคลื่อนที่ย้อนเวลา
  `p += RDF(p)` และเรียงต่อกันย้อนจากการกระทำสุดท้ายไปแรกสุด
  จากนั้น **สุ่ม texture ครั้งเดียวตอนท้าย** จึงไม่เบลอจากการ resample ซ้ำ
- **`rdSegment` (Algorithm 1)**: จำลองอนุภาคย้อนเวลาตามเส้นตรงจาก a ไป b ด้วย adaptive midpoint
  ขั้นละ `ΔL = min(L, α·max(ε, r))` เปเปอร์ใช้ α = 1/10
- **Progressive rendering (§6.1)**: คำนวณต่อพิกเซลอิสระต่อกัน jitter ตำแหน่งตัวอย่างด้วย
  ลำดับ Roberts R2 และสะสมค่าข้ามเฟรม (เปเปอร์ใช้ compute shader หนึ่ง pass ต่อ RDF)
- **ความเป็น resolution-independent**: ทุกตัวอย่างคำนวณเดี่ยวๆ จากพิกัด ไม่ต้องมี grid หรือ global solve
  จึงเลือกความละเอียดผลลัพธ์ได้ตามต้องการ

### Feature ที่เสนอให้ทำ (เรียงตามลำดับ)

| ลำดับ | Feature | อ้างอิง | หมายเหตุ |
| --- | --- | --- | --- |
| 1 | Procedural marble field แยก pass ของตัวเอง ประเมินจากพิกัดโดเมนตามความละเอียดที่เลือก (deterministic) | งานของเราเอง (AGENT.md §6) | เปเปอร์ไม่ได้กำหนดสูตรหินอ่อนนี้ ห้ามอ้างว่าเป็นผลงานของเปเปอร์ |
| 2 | Compute pass แสดงผล marble ลงภาพ แล้ว present (ยังไม่มีแปรง) | skeleton ต่อยอด | พิสูจน์ pipeline compute → swapchain และ timestamp query |
| 3 | `rdSegment` บน GPU สำหรับหนึ่งเส้น (a → b, ε) | Algorithm 1, สมการ 26–27, 34 | เก็บพิกัด body-frame `r = p − x` เพื่อลดปัญหา floating-point ใกล้หัวแปรง |
| 4 | ลากเมาส์เป็น polyline เก็บ segment ใน storage buffer แล้วเรียง RDF ย้อนลำดับ | §5.2, §5.5 | ปัจจุบัน cost โตตามจำนวน segment ต่อพิกเซล |
| 5 | Progressive accumulation + jitter R2 | §6.1, สมการ 37–38 | เฟรมแรกอาจมี noise แล้วลู่เข้าเมื่อสะสม |
| 6 | พารามิเตอร์ปรับได้: รัศมี ε, ความละเอียด, α, เลข segment | — | ส่งเป็น push constants |
| 7 | Timestamp query ต่อ pass และส่งออกข้อมูลวัด | AGENT.md §7 | |
| 8 (ถ้ามีเวลา) | Localization mask ตัดผลที่ระยะ γε (เปเปอร์ใช้ γ = 10) | §5.4.2 | ลดงานต่อพิกเซลแต่เป็นการประมาณ ต้องวัดความต่าง |
| 9 (ถ้ามีเวลา) | `rdLine` สำหรับเส้นยาวไม่สิ้นสุด / comb | §5.3 | ต้องใช้ฟังก์ชัน ξ(η) ซึ่งเปเปอร์ให้ค่าประมาณแบบอนุกรมในภาคผนวก B ต้องตรวจสอบก่อน |

### สิ่งที่ไม่ทำ (ตามขอบเขต AGENT.md)

- Elliptic integral / Newton time-inversion drift solver (§3, supplement)
- Affine brush (pinch/twist), rigid boundary approximation, Houdini/OSL integration
- Periodic RDF composition (§6.2) ยกเว้นเหลือเวลาจริงๆ
- Fluid simulation เต็มรูปแบบ, layers, undo/redo, หลายตระกูลแปรง

### แนวทางประเมินผล (เสนอ ยังไม่ได้ทำ)

- **Baseline**: advect texture ด้วย velocity ของแปรงแบบ time-stepping ลงในภาพที่ความละเอียดคงที่ทีละ stroke
  (วิธีที่เกิด blur จาก resample) เทียบกับ RDF ที่สุ่ม texture ครั้งเดียว
  ต้องใช้ input เดียวกัน (ลำดับ stroke, สถานะเริ่มต้น, ฮาร์ดแวร์)
- **Ablation variables** (อย่างน้อย 2): ความละเอียดผลลัพธ์ และจำนวน segment/ความซับซ้อนของ stroke
  ตัวเลือกเพิ่ม: α (ขนาดก้าว) และ γ (ระยะตัด)
- **เวลา**: GPU timestamp ต่อ pass, warm-up, ทำซ้ำ, รายงาน median และการกระจาย พร้อมบันทึก GPU/driver
- **คุณภาพ**: เทียบกับ reference ที่ α เล็กมากหรือ supersample สูง ด้วย PSNR/SSIM
  โดยกำหนดโดเมนร่วมและ color space ให้ชัด
- เปเปอร์รายงานว่า 1920x1080 บน RTX 4080 ใช้ ~10.4 ms ต่อตัวอย่างต่อพิกเซลสำหรับลวดลายที่ซับซ้อนมาก (§6.1)
  เป็นตัวเลขของผู้เขียนบนเครื่องของเขา **ห้ามนำมาอ้างเป็นผลของเรา**

### ความเสี่ยงและข้อสังเกต

- cost ของ `rdSegment` เพิ่มตามจำนวน segment ต่อพิกเซล ควรวางแผน cap หรือ localization ตั้งแต่ต้น
- ใกล้หัวแปรงสนามเป็นแบบ cusp ต้องใช้ก้าวเล็ก ตัวเลือก α ส่งผลทั้งความแม่นและเวลา
- เปเปอร์ระบุข้อจำกัดว่าเป็น potential flow ไม่มี inertia/turbulence และไม่จำลองสีจริง
  จึงควรรายงานว่าเป็นการ advect texture ไม่ใช่การจำลองสี
- ก่อนอ้างว่าเปเปอร์เป็น Best Paper ของ SIGGRAPH 2026 ให้ตรวจสอบบันทึกรางวัลทางการ (ตาม AGENT.md)
- ภาคผนวกและเอกสาร supplemental (`mixwell-2026-supplemental.pdf`) ยังไม่ได้อ่านละเอียด
  ต้องตรวจสอบสูตร ξ และค่าประมาณอนุกรมก่อนนำไปใช้
