#include <Labo_5.h>
#include <Arduino_OV767X.h>
#include <stdint.h>
#include <stdlib.h>

/* Constants --------------------------------------------------------------- */
#define EI_CAMERA_RAW_FRAME_BUFFER_COLS 160
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS 120
#define DWORD_ALIGN_PTR(a) (((a) & 0x3) ? (((uintptr_t)(a) + 0x4) & ~(uintptr_t)0x3) : (a))

/* Function Prototypes ----------------------------------------------------- */
void RespondToDetection(float background_score, float mask_score, float no_mask_score);
bool ei_camera_init(void);
void ei_camera_deinit(void);
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf);
int calculate_resize_dimensions(uint32_t out_width, uint32_t out_height, uint32_t *resize_col_sz, uint32_t *resize_row_sz, bool *do_resize);
void cropImage(int srcWidth, int srcHeight, uint8_t *srcImage, int startX, int startY, int dstWidth, int dstHeight, uint8_t *dstImage, int iBpp);

/* Camera Class ------------------------------------------------------------ */
class OV7675 : public OV767X {
public:
    int begin(int resolution, int format, int fps);
    void readFrame(void* buffer);
};

/* Structs ----------------------------------------------------------------- */
typedef struct {
    size_t width;
    size_t height;
} ei_device_resize_resolutions_t;

/* Global Variables -------------------------------------------------------- */
static OV7675 Cam;
static bool is_initialised = false;
static uint8_t *ei_camera_capture_out = NULL;
uint32_t resize_col_sz;
uint32_t resize_row_sz;
bool do_resize = false;
bool do_crop = false;
static bool debug_nn = false;

/* Arduino Setup ----------------------------------------------------------- */
void setup() {
    Serial.begin(115200);
    while (!Serial);
    Serial.println("Edge Impulse Inferencing Demo");
    ei_printf("Inferencing settings:\n");
    ei_printf("\tImage resolution: %dx%d\n", EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT);
    ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0]));
}

/* Main Loop ---------------------------------------------------------------- */
void loop() {
    while (true) {
        ei_printf("\nStarting inferencing in 2 seconds...\n");
        if (ei_sleep(2000) != EI_IMPULSE_OK) break;
        ei_printf("Taking photo...\n");
        
        if (!ei_camera_init()) {
            ei_printf("ERR: Failed to initialize image sensor\r\n");
            break;
        }

        if (calculate_resize_dimensions(EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT, &resize_col_sz, &resize_row_sz, &do_resize)) {
            ei_printf("ERR: Failed to calculate resize dimensions\r\n");
            break;
        }

        uint8_t *snapshot_buf = (uint8_t *)ei_malloc(resize_col_sz * resize_row_sz * 2);
        if (!snapshot_buf) {
            ei_printf("ERR: Memory allocation failed\r\n");
            break;
        }
        
        if (!ei_camera_capture(EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT, snapshot_buf)) {
            ei_printf("ERR: Failed to capture image\r\n");
            ei_free(snapshot_buf);
            break;
        }

        ei::signal_t signal = { EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT, &ei_camera_cutout_get_data };
        ei_impulse_result_t result = { 0 };
        
        if (run_classifier(&signal, &result, debug_nn) != EI_IMPULSE_OK) {
            ei_printf("Failed to run impulse\n");
            ei_free(snapshot_buf);
            break;
        }

        ei_printf("Predictions:\r\n");
        for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
            ei_printf("  %s: %.5f\r\n", ei_classifier_inferencing_categories[i], result.classification[i].value);
        }

        RespondToDetection(result.classification[0].value, result.classification[1].value, result.classification[2].value);

        ei_free(snapshot_buf);
    }
    ei_camera_deinit();
}

/* Camera Functions -------------------------------------------------------- */
bool ei_camera_init() {
    if (is_initialised) return true;
    if (!Cam.begin(QQVGA, RGB565, 1)) {
        ei_printf("ERR: Failed to initialize camera\r\n");
        return false;
    }
    is_initialised = true;
    return true;
}

void ei_camera_deinit() {
    if (is_initialised) {
        Cam.end();
        is_initialised = false;
    }
}

bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf) {
    if (!is_initialised || !out_buf) {
        ei_printf("ERR: Invalid parameters\r\n");
        return false;
    }
    Cam.readFrame(out_buf);
    return true;
}

/* Utility Functions ------------------------------------------------------- */
int calculate_resize_dimensions(uint32_t out_width, uint32_t out_height, uint32_t *resize_col_sz, uint32_t *resize_row_sz, bool *do_resize) {
    const ei_device_resize_resolutions_t list[] = { {42, 32}, {128, 96} };
    *resize_col_sz = EI_CAMERA_RAW_FRAME_BUFFER_COLS;
    *resize_row_sz = EI_CAMERA_RAW_FRAME_BUFFER_ROWS;
    *do_resize = false;
    for (const auto& res : list) {
        if (out_width <= res.width && out_height <= res.height) {
            *resize_col_sz = res.width;
            *resize_row_sz = res.height;
            *do_resize = true;
            break;
        }
    }
    return 0;
}

int ei_camera_cutout_get_data(size_t offset, size_t length, float *out_ptr) {
    size_t pixel_ix = offset * 2;
    size_t out_ptr_ix = 0;

    while (length--) {
        uint16_t pixel = (ei_camera_capture_out[pixel_ix] << 8) | ei_camera_capture_out[pixel_ix + 1];
        uint8_t r = ((pixel >> 11) & 0x1F) << 3;
        uint8_t g = ((pixel >> 5) & 0x3F) << 2;
        uint8_t b = (pixel & 0x1F) << 3;

        out_ptr[out_ptr_ix++] = (r << 16) + (g << 8) + b;
        pixel_ix += 2;
    }

    return 0;
}

#ifdef __ARM_FEATURE_SIMD32
#include <device.h>
#endif

#define FRAC_BITS 14
#define FRAC_VAL (1 << FRAC_BITS)
#define FRAC_MASK (FRAC_VAL - 1)


void resizeImage(int srcWidth, int srcHeight, uint8_t *srcImage,
                 int dstWidth, int dstHeight, uint8_t *dstImage, int iBpp) {
    if (iBpp != 8 && iBpp != 16) return;

    uint32_t src_x_accum = FRAC_VAL / 2;
    uint32_t src_y_accum = FRAC_VAL / 2;
    uint32_t src_x_frac = (srcWidth * FRAC_VAL) / dstWidth;
    uint32_t src_y_frac = (srcHeight * FRAC_VAL) / dstHeight;

    for (int y = 0; y < dstHeight; y++) {
        int ty = src_y_accum >> FRAC_BITS;
        uint32_t y_frac = src_y_accum & FRAC_MASK;
        uint32_t ny_frac = FRAC_VAL - y_frac;
        src_y_accum += src_y_frac;

        uint8_t *s = &srcImage[ty * srcWidth];
        uint8_t *d = &dstImage[y * dstWidth];

        src_x_accum = FRAC_VAL / 2;
        for (int x = 0; x < dstWidth; x++) {
            int tx = src_x_accum >> FRAC_BITS;
            uint32_t x_frac = src_x_accum & FRAC_MASK;
            uint32_t nx_frac = FRAC_VAL - x_frac;
            src_x_accum += src_x_frac;

            uint32_t p00 = s[tx], p10 = s[tx + 1];
            uint32_t p01 = s[tx + srcWidth], p11 = s[tx + srcWidth + 1];

            uint32_t interp_x0 = (p00 * nx_frac + p10 * x_frac + FRAC_VAL / 2) >> FRAC_BITS;
            uint32_t interp_x1 = (p01 * nx_frac + p11 * x_frac + FRAC_VAL / 2) >> FRAC_BITS;
            uint32_t interp = (interp_x0 * ny_frac + interp_x1 * y_frac + FRAC_VAL / 2) >> FRAC_BITS;

            *d++ = (uint8_t)interp;
        }
    }
}

void cropImage(int srcWidth, int srcHeight, uint8_t *srcImage,
               int startX, int startY, int dstWidth, int dstHeight,
               uint8_t *dstImage, int iBpp) {
    if (startX < 0 || startY < 0 || startX + dstWidth > srcWidth || startY + dstHeight > srcHeight) return;
    if (iBpp != 8 && iBpp != 16) return;

    for (int y = 0; y < dstHeight; y++) {
        uint8_t *s = &srcImage[srcWidth * (y + startY) + startX];
        uint8_t *d = &dstImage[dstWidth * y];

        for (int x = 0; x < dstWidth; x++) {
            *d++ = *s++;
        }
    }
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_CAMERA
#error "Invalid model for current sensor"
#endif

#include <Arduino.h>
#include <Wire.h>

#define digitalPinToBitMask(P) (1 << (digitalPinToPinName(P) % 32))
#define portInputRegister(P) ((P == 0) ? &NRF_P0->IN : &NRF_P1->IN)

int OV7675::begin(int resolution, int format, int fps) {
    pinMode(OV7670_VSYNC, INPUT);
    pinMode(OV7670_HREF, INPUT);
    pinMode(OV7670_PLK, INPUT);
    pinMode(OV7670_XCLK, OUTPUT);

    vsyncPort = portInputRegister(digitalPinToPort(OV7670_VSYNC));
    vsyncMask = digitalPinToBitMask(OV7670_VSYNC);
    hrefPort = portInputRegister(digitalPinToPort(OV7670_HREF));
    hrefMask = digitalPinToBitMask(OV7670_HREF);
    pclkPort = portInputRegister(digitalPinToPort(OV7670_PLK));
    pclkMask = digitalPinToBitMask(OV7670_PLK);

    bool ret = OV767X::begin(VGA, format, fps);
    width = OV767X::width();
    height = OV767X::height();
    bytes_per_pixel = OV767X::bytesPerPixel();
    bytes_per_row = width * bytes_per_pixel;
    resize_height = 2;

    buf_mem = nullptr;
    raw_buf = nullptr;

    return ret;
}

int OV7675::allocate_scratch_buffs() {
    buf_rows = height / resize_row_sz * resize_height;
    buf_size = bytes_per_row * buf_rows;

    buf_mem = ei_malloc(buf_size);
    if (!buf_mem) return -1;

    raw_buf = reinterpret_cast<uint8_t *>(DWORD_ALIGN_PTR(reinterpret_cast<uintptr_t>(buf_mem)));
    return 0;
}

int OV7675::deallocate_scratch_buffs() {
    ei_free(buf_mem);
    buf_mem = nullptr;
    return 0;
}

void OV7675::readFrame(void* buffer) {
    allocate_scratch_buffs();
    uint8_t* out = static_cast<uint8_t*>(buffer);
    noInterrupts();

    while ((*vsyncPort & vsyncMask) == 0);
    while ((*vsyncPort & vsyncMask) != 0);

    interrupts();
}
