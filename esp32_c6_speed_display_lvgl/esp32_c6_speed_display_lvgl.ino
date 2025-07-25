#include <lvgl.h>
#include "esp_lcd_touch_axs5106l.h"  // Assuming this is your touch controller library


// For ESP-NOW data structure 
#ifndef DATA_TYPES_H
#define DATA_TYPES_H
#include <stdint.h>
#define SPEED_NULL_VALUE -1
#define RPM_NULL_VALUE -1
typedef struct {
  int16_t speed;  // Speed value (e.g., in km/h or mph).
  int16_t rpm;  // RPM value.
} __attribute__((packed)) EspNowData;
#endif  // DATA_TYPES_H


/*To use the built-in examples and demos of LVGL uncomment the includes below respectively.
*You also need to copy `lvgl/examples` to `lvgl/src/examples`. Similarly for the demos `lvgl/demos` to `lvgl/src/demos`.
*Note that the `lv_examples` library is for LVGL v7 and you shouldn't install it for this version (since LVGL v8)
*as the examples and demos are now part of the main LVGL library. */

// #include <examples/lv_examples.h>
// #include <demos/lv_demos.h> // Keep commented unless you want to run LVGL demos

// #define DIRECT_RENDER_MODE // Uncomment to enable full frame buffer

#include <Arduino_GFX_Library.h>

#define ROTATION 3
// #define ROTATION 1
// #define ROTATION 2
// #define ROTATION 3

#define GFX_BL 23  // Backlight pin

#define Touch_I2C_SDA 18
#define Touch_I2C_SCL 19
#define Touch_RST 20
#define Touch_INT 21

#define LEDC_FREQ 5000
#define LEDC_TIMER_10_BIT 10

#define SHIFT_1 4000
#define SHIFT_2 4700
#define SHIFT_3 5400
#define SHIFT_4 6100

// LV_FONT_DECLARE(font_dseg14_84_4bpp);
// LV_FONT_DECLARE(font_opensans_96_4bpp);
LV_FONT_DECLARE(font_opensans_120_4bpp);


// Global LVGL objects for the UI elements
lv_obj_t *digit_labels[3]; // Array for hundreds, tens, and units digits
lv_obj_t *speed_unit_label;
lv_obj_t *shift_lights[6];  // Array for 6 shift light rectangles

// Variables to hold the current values to display, updated by simulated ESP-NOW data
// These should ideally be updated by your actual ESP-NOW receiver callback
volatile int16_t current_speed_val = 0;
volatile int16_t current_rpm_val = 0;

// Variables to store the last known good values (for handling NULLs)
static int16_t last_known_speed = 0;
static int16_t last_known_rpm = 0;

// State for flashing shift lights
static bool is_flashing_active = false;

// Forward declarations
void lvgl_init_custom_ui(void);
void update_ui_values(int16_t speed_val, int16_t rpm_val);
void update_shift_lights(int16_t rpm_val_for_lights);
void flash_timer_cb(lv_timer_t *timer);
void adjust_brightness_event_cb(lv_event_t *e); // Forward declaration for brightness control

// Arduino_GFX setup
Arduino_DataBus *bus = new Arduino_HWSPI(15 /* DC */, 14 /* CS */, 1 /* SCK */, 2 /* MOSI */);

Arduino_GFX *gfx = new Arduino_ST7789(
  bus, 22 /* RST */, 0 /* rotation */, false /* IPS */,
  172 /* width */, 320 /* height */,  // Note: width and height might be swapped depending on rotation
  34 /*col_offset1*/, 0 /*uint8_t row_offset1*/,
  34 /*col_offset2*/, 0 /*row_offset2*/);

void lcd_reg_init(void)
{
  static const uint8_t init_operations[] = {
    BEGIN_WRITE,
    WRITE_COMMAND_8, 0x11,  // 2: Out of sleep mode, no args, w/delay
    END_WRITE,
    DELAY, 120,

    BEGIN_WRITE,
    WRITE_C8_D16, 0xDF, 0x98, 0x53,
    WRITE_C8_D8, 0xB2, 0x23,

    WRITE_COMMAND_8, 0xB7,
    WRITE_BYTES, 4,
    0x00, 0x47, 0x00, 0x6F,

    WRITE_COMMAND_8, 0xBB,
    WRITE_BYTES, 6,
    0x1C, 0x1A, 0x55, 0x73, 0x63, 0xF0,

    WRITE_C8_D16, 0xC0, 0x44, 0xA4,
    WRITE_C8_D8, 0xC1, 0x16,

    WRITE_COMMAND_8, 0xC3,
    WRITE_BYTES, 8,
    0x7D, 0x07, 0x14, 0x06, 0xCF, 0x71, 0x72, 0x77,

    WRITE_COMMAND_8, 0xC4,
    WRITE_BYTES, 12,
    0x00, 0x00, 0xA0, 0x79, 0x0B, 0x0A, 0x16, 0x79, 0x0B, 0x0A, 0x16, 0x82,

    WRITE_COMMAND_8, 0xC8,
    WRITE_BYTES, 32,
    0x3F, 0x32, 0x29, 0x29, 0x27, 0x2B, 0x27, 0x28, 0x28, 0x26, 0x25, 0x17, 0x12, 0x0D, 0x04, 0x00, 0x3F, 0x32, 0x29, 0x29, 0x27, 0x2B, 0x27, 0x28, 0x28, 0x26, 0x25, 0x17, 0x12, 0x0D, 0x04, 0x00,

    WRITE_COMMAND_8, 0xD0,
    WRITE_BYTES, 5,
    0x04, 0x06, 0x6B, 0x0F, 0x00,

    WRITE_C8_D16, 0xD7, 0x00, 0x30,
    WRITE_C8_D8, 0xE6, 0x14,
    WRITE_C8_D8, 0xDE, 0x01,

    WRITE_COMMAND_8, 0xB7,
    WRITE_BYTES, 5,
    0x03, 0x13, 0xEF, 0x35, 0x35,

    WRITE_COMMAND_8, 0xC1,
    WRITE_BYTES, 3,
    0x14, 0x15, 0xC0,

    WRITE_C8_D16, 0xC2, 0x06, 0x3A,
    WRITE_C8_D16, 0xC4, 0x72, 0x12,
    WRITE_C8_D8, 0xBE, 0x00,
    WRITE_C8_D8, 0xDE, 0x02,

    WRITE_COMMAND_8, 0xE5,
    WRITE_BYTES, 3,
    0x00, 0x02, 0x00,

    WRITE_COMMAND_8, 0xE5,
    WRITE_BYTES, 3,
    0x01, 0x02, 0x00,

    WRITE_C8_D8, 0xDE, 0x00,
    WRITE_C8_D8, 0x35, 0x00,
    WRITE_C8_D8, 0x3A, 0x05,

    WRITE_COMMAND_8, 0x2A,
    WRITE_BYTES, 4,
    0x00, 0x22, 0x00, 0xCD,

    WRITE_COMMAND_8, 0x2B,
    WRITE_BYTES, 4,
    0x00, 0x00, 0x01, 0x3F,

    WRITE_C8_D8, 0xDE, 0x02,

    WRITE_COMMAND_8, 0xE5,
    WRITE_BYTES, 3,
    0x00, 0x02, 0x00,

    WRITE_C8_D8, 0xDE, 0x00,
    WRITE_C8_D8, 0x36, 0x00,
    WRITE_COMMAND_8, 0x21,
    END_WRITE,

    DELAY, 10,

    BEGIN_WRITE,
    WRITE_COMMAND_8, 0x29,  // 5: Main screen turn on, no args, w/delay
    END_WRITE
  };
  bus->batchOperation(init_operations, sizeof(init_operations));
}

uint32_t screenWidth;
uint32_t screenHeight;
uint32_t bufSize;
lv_disp_draw_buf_t draw_buf;
lv_color_t *disp_draw_buf;
lv_disp_drv_t disp_drv;

#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char *buf)
{
  Serial.printf(buf);
  Serial.flush();
}
#endif

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
#ifndef DIRECT_RENDER_MODE
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif
#endif  // #ifndef DIRECT_RENDER_MODE

  lv_disp_flush_ready(disp_drv);
}

/*Read the touchpad*/
void touchpad_read_cb(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
  touch_data_t touch_data;
  uint8_t touchpad_cnt = 0;

  /* Read touch controller data */
  bsp_touch_read();
  /* Get coordinates */
  bool touchpad_pressed = bsp_touch_get_coordinates(&touch_data);

  if (touchpad_pressed)
  {
    data->point.x = touch_data.coords[0].x;
    data->point.y = touch_data.coords[0].y;
    data->state = LV_INDEV_STATE_PRESSED;
  }
  else
  {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

// Function to update the shift lights' colors based on RPM and flashing state
void update_shift_lights(int16_t rpm_val_for_lights) {
  lv_color_t off_color = lv_color_make(0x00, 0x00, 0x00);  // Black
  lv_color_t yellow_light = lv_color_make(0xFF, 0xEF, 0x00);  // Gold
  lv_color_t orange_light = lv_color_make(0xFF, 0x60, 0x00);  // Orange
  lv_color_t red_light = lv_color_make(0xFF, 0x00, 0x00);  // Red
  lv_color_t blue_light = lv_color_make(0x00, 0x80, 0xFF);  // Red

  // Determine the active color based on flashing state for redline
  is_flashing_active = !is_flashing_active;
  lv_color_t current_blue_color = is_flashing_active ? blue_light : off_color;

  // Set all lights to off color initially
  for (int i = 0; i < 6; i++) {
    lv_obj_set_style_bg_color(shift_lights[i], off_color, LV_STATE_DEFAULT);
  }

  if (rpm_val_for_lights >= SHIFT_4) {
    // Flash all lights blue
    for (int i = 0; i < 6; i++) {
      lv_obj_set_style_bg_color(shift_lights[i], current_blue_color, LV_STATE_DEFAULT);
    }
  } else if (rpm_val_for_lights >= SHIFT_3) {
    // All 6 lights yellow
    for (int i = 0; i < 6; i++) {
      lv_obj_set_style_bg_color(shift_lights[i], red_light, LV_STATE_DEFAULT);
    }
  } else if (rpm_val_for_lights >= SHIFT_2) {
    // Lights 0, 5, 1, 4 are yellow (outer two pairs)
    lv_obj_set_style_bg_color(shift_lights[0], orange_light, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(shift_lights[5], orange_light, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(shift_lights[1], orange_light, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(shift_lights[4], orange_light, LV_STATE_DEFAULT);
  } else if (rpm_val_for_lights >= SHIFT_1) {
    // Lights 0 and 5 are yellow (outermost pair)
    lv_obj_set_style_bg_color(shift_lights[0], yellow_light, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(shift_lights[5], yellow_light, LV_STATE_DEFAULT);
  }
}

// Function to initialize the custom LVGL UI elements
void lvgl_init_custom_ui(void)
{
  // Create a screen
  lv_obj_t *scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr, lv_color_black(), LV_STATE_DEFAULT);  // Dark background
  lv_obj_set_style_radius(scr, 0, LV_STATE_DEFAULT);  // No rounded corners for screen itself
  lv_obj_set_style_border_width(scr, 0, LV_STATE_DEFAULT);  // No border
  lv_scr_load(scr);

  // --- Shift Lights (3 on each side, stacked vertically) ---
  int light_width = 15;
  int light_height = 44;
  int vertical_spacing = 10; // Space between stacked lights
  int horizontal_margin = 10; // Margin from screen edge
  int bottom_y_offset = 10; // Offset from the bottom of the screen

  // Calculate Y positions for the stacked lights
  int bottom_y = screenHeight - bottom_y_offset - light_height;
  int middle_y = bottom_y - light_height - vertical_spacing;
  int top_y = middle_y - light_height - vertical_spacing;

  // Create Shift Lights (rectangles) and position them
  for (int i = 0; i < 6; i++) {
    shift_lights[i] = lv_obj_create(scr); // Create directly on screen, no container
    lv_obj_set_size(shift_lights[i], light_width, light_height);
    lv_obj_set_style_radius(shift_lights[i], LV_RADIUS_CIRCLE, LV_STATE_DEFAULT); // Pill shape
    lv_obj_set_style_border_width(shift_lights[i], 0, LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(shift_lights[i], 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(shift_lights[i], 0, LV_STATE_DEFAULT);  // Remove padding
    lv_obj_set_style_bg_color(shift_lights[i], lv_color_black(), LV_STATE_DEFAULT); // Initial off color
  }

  // Position left side lights (indices 0, 1, 2 from bottom to top)
  lv_obj_set_pos(shift_lights[0], horizontal_margin, bottom_y); // Bottom-left
  lv_obj_set_pos(shift_lights[1], horizontal_margin, middle_y); // Middle-left
  lv_obj_set_pos(shift_lights[2], horizontal_margin, top_y);    // Top-left

  // Position right side lights (indices 5, 4, 3 from bottom to top)
  lv_obj_set_pos(shift_lights[5], screenWidth - horizontal_margin - light_width, bottom_y); // Bottom-right
  lv_obj_set_pos(shift_lights[4], screenWidth - horizontal_margin - light_width, middle_y); // Middle-right
  lv_obj_set_pos(shift_lights[3], screenWidth - horizontal_margin - light_width, top_y);    // Top-right
  // --- Speed Display (3 Digits) ---
  // Create a style for the digits
  static lv_style_t digit_style;
  lv_style_init(&digit_style);
  lv_style_set_bg_color(&digit_style, lv_color_black()); // Black background for digits
  lv_style_set_text_color(&digit_style, lv_color_white()); // White text
  lv_style_set_text_font(&digit_style, &font_opensans_120_4bpp); // Large font
  lv_style_set_text_align(&digit_style, LV_TEXT_ALIGN_CENTER); // Center align text within each digit's area

  int digit_width = 50; // Example width for a digit, adjust based on font and desired appearance
  int digit_spacing = 20;      // Space between digits

  // Create the digit labels using a loop and assign to the array
  for (int i = 0; i < 3; i++) {
      digit_labels[i] = lv_label_create(scr);
      lv_obj_add_style(digit_labels[i], &digit_style, 0);
      lv_obj_set_width(digit_labels[i], digit_width);
      // Adjust height based on font line height to ensure digits fit well
      lv_obj_set_height(digit_labels[i], lv_font_get_line_height(&font_opensans_120_4bpp) + 10);
      lv_label_set_text(digit_labels[i], "0"); // Initial text
  }

  // Position the digits side by side and center them horizontally on the screen
  // The first digit (hundreds) is positioned relative to the center, then others relative to the previous one
  // Total width of digits and spacing: 3 * digit_width + 2 * digit_spacing
  // Align the leftmost digit (digit_labels[0]) to the center, then shift left by half of the total width
  lv_obj_align(digit_labels[0], LV_ALIGN_CENTER, -((3 * digit_width + 2 * digit_spacing) / 2), 0);
  lv_obj_align_to(digit_labels[1], digit_labels[0], LV_ALIGN_OUT_RIGHT_MID, digit_spacing, 0);
  lv_obj_align_to(digit_labels[2], digit_labels[1], LV_ALIGN_OUT_RIGHT_MID, digit_spacing, 0);


  // --- Speed Unit Label ---
  speed_unit_label = lv_label_create(scr);
  lv_label_set_text(speed_unit_label, "km/h");
  lv_obj_set_style_text_font(speed_unit_label, &lv_font_montserrat_20, LV_STATE_DEFAULT);  // Font size 20
  lv_obj_set_style_text_color(speed_unit_label, lv_color_make(0x80, 0x80, 0x80), LV_STATE_DEFAULT);  // Gray
  // Align below the middle digit of the speed display
  lv_obj_align_to(speed_unit_label, digit_labels[2], LV_ALIGN_OUT_RIGHT_MID, 10, 30);


  // Add a transparent overlay to capture touch events over the entire screen for brightness
  lv_obj_t *touch_overlay = lv_obj_create(scr);
  lv_obj_set_size(touch_overlay, LV_PCT(100), LV_PCT(100));
  lv_obj_center(touch_overlay);
  lv_obj_set_style_bg_opa(touch_overlay, LV_OPA_TRANSP, LV_STATE_DEFAULT);  // Make it transparent
  lv_obj_set_style_border_width(touch_overlay, 0, LV_STATE_DEFAULT);  // Remove border
  lv_obj_set_style_pad_all(touch_overlay, 0, LV_STATE_DEFAULT);  // Remove padding
  lv_obj_add_event_cb(touch_overlay, adjust_brightness_event_cb, LV_EVENT_CLICKED, NULL);  // Re-enabled brightness control

  // Initial UI update
  update_ui_values(current_speed_val, current_rpm_val);
}

// Function to update the LVGL display with new values
void update_ui_values(int16_t speed_val, int16_t rpm_val)
{
  // Update last known good values, handling NULLs
  if (speed_val != SPEED_NULL_VALUE) {
    last_known_speed = speed_val;
  }
  if (rpm_val != RPM_NULL_VALUE) {
    last_known_rpm = rpm_val;
  }

  // Set current display values (use last known if current is NULL)
  current_speed_val = (speed_val == SPEED_NULL_VALUE) ? last_known_speed : speed_val;
  current_rpm_val = (rpm_val == RPM_NULL_VALUE) ? last_known_rpm : rpm_val;

  // --- Update Speed Digits ---
  // Ensure speed is within 0-999 range for 3 digits
  int display_speed = current_speed_val;
  if (display_speed < 0) display_speed = 0;
  if (display_speed > 999) display_speed = 999;

  // Extract individual digits
  int hundreds = display_speed / 100;
  int tens = (display_speed / 10) % 10;
  int units = display_speed % 10;

  // Update the text of each label based on speed value for leading zeros
  if (display_speed < 10) {
      lv_label_set_text(digit_labels[0], " "); // Hundreds: space
      lv_label_set_text(digit_labels[1], " "); // Tens: space
      lv_label_set_text_fmt(digit_labels[2], "%d", units); // Units: actual digit
  } else if (display_speed < 100) {
      lv_label_set_text(digit_labels[0], " "); // Hundreds: space
      lv_label_set_text_fmt(digit_labels[1], "%d", tens);   // Tens: actual digit
      lv_label_set_text_fmt(digit_labels[2], "%d", units);  // Units: actual digit
  } else {
      lv_label_set_text_fmt(digit_labels[0], "%d", hundreds); // Hundreds: actual digit
      lv_label_set_text_fmt(digit_labels[1], "%d", tens);     // Tens: actual digit
      lv_label_set_text_fmt(digit_labels[2], "%d", units);    // Units: actual digit
  }

  // Update Shift Lights based on the displayed RPM
  update_shift_lights(current_rpm_val);

  lv_task_handler();  // Manually refresh LVGL display if not using a timer for this
}


void setup()
{
#ifdef DEV_DEVICE_INIT
  DEV_DEVICE_INIT();
#endif

  Serial.begin(115200);
  Serial.println("Arduino_GFX LVGL_Arduino_v8 Custom UI Example");
  String LVGL_Arduino = String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.println(LVGL_Arduino);

  // Init Display
  if (!gfx->begin())
  {
    Serial.println("gfx->begin() failed!");
  }
  lcd_reg_init();
  gfx->setRotation(ROTATION);
  gfx->fillScreen(RGB565_BLACK);

#ifdef GFX_BL
  ledcAttach(GFX_BL, LEDC_FREQ, LEDC_TIMER_10_BIT);
  ledcWrite(GFX_BL, (1 << LEDC_TIMER_10_BIT) / 100 * 80);  // Initial brightness 80%
#endif

  // Init touch device
  Wire.begin(Touch_I2C_SDA, Touch_I2C_SCL);
  bsp_touch_init(&Wire, Touch_RST, Touch_INT, gfx->getRotation(), gfx->width(), gfx->height());
  lv_init();

#if LV_USE_LOG != 0
  lv_log_register_print_cb(my_print); /* register print function for debugging */
#endif

  screenWidth = gfx->width();
  screenHeight = gfx->height();

#ifdef DIRECT_RENDER_MODE
  bufSize = screenWidth * screenHeight;
#else
  bufSize = screenWidth * 40;  // Use a smaller buffer for partial updates
#endif

#ifdef ESP32
#if defined(DIRECT_RENDER_MODE) && (defined(CANVAS) || defined(RGB_PANEL) || defined(DSI_PANEL))
  disp_draw_buf = (lv_color_t *)gfx->getFramebuffer();
#else  // !(defined(DIRECT_RENDER_MODE) && (defined(CANVAS) || defined(RGB_PANEL) || defined(DSI_PANEL)))
  disp_draw_buf = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!disp_draw_buf)
  {
    // remove MALLOC_CAP_INTERNAL flag try again
    disp_draw_buf = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_8BIT);
  }
#endif  // !(defined(DIRECT_RENDER_MODE) && (defined(CANVAS) || defined(RGB_PANEL) || defined(DSI_PANEL)))
#else  // !ESP32
  Serial.println("LVGL disp_draw_buf heap_caps_malloc failed! malloc again...");
  disp_draw_buf = (lv_color_t *)malloc(bufSize * 2);
#endif  // !ESP32
  if (!disp_draw_buf)
  {
    Serial.println("LVGL disp_draw_buf allocate failed!");
  }
  else
  {
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, bufSize);

    /* Initialize the display */
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
#ifdef DIRECT_RENDER_MODE
    disp_drv.direct_mode = true;
#endif
    lv_disp_drv_register(&disp_drv);

    /* Initialize the input device driver */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read_cb;
    lv_indev_drv_register(&indev_drv);

    // Initialize your custom UI
    lvgl_init_custom_ui();

    // Create a timer for the flashing effect (runs every 100ms)
    // lv_timer_create(flash_timer_cb, 10, NULL);
  }

  Serial.println("Setup done");
}

void loop()
{
  // Simulate incoming data for speed and RPM
  static int simulated_speed_counter = 0;
  static int simulated_rpm_counter = 0;

  // Simulate speed: 0 to 165 km/h
  if (simulated_speed_counter > 220) {
    simulated_speed_counter = 0;
  }
  int16_t simulated_speed = simulated_speed_counter;
  simulated_speed_counter += 1;  // Increment speed slowly

  // Simulate RPM: Cycle through values to test shift lights and flashing
  if (simulated_rpm_counter > 6600) {  // Max RPM for simulation
    simulated_rpm_counter = 0;
  }
  int16_t simulated_rpm = simulated_rpm_counter;
  simulated_rpm_counter += 30;  // Increment RPM faster

  // Simulate NULL values occasionally (e.g., every 100th speed update)
  if (simulated_speed_counter % 100 == 0) {
    simulated_speed = SPEED_NULL_VALUE;
  }
  if (simulated_rpm_counter % 200 == 0) {
    simulated_rpm = RPM_NULL_VALUE;
  }


  // Update the UI with simulated values
  update_ui_values(simulated_speed, simulated_rpm);

  lv_timer_handler(); /* let the GUI do its work */

#ifdef DIRECT_RENDER_MODE
#if defined(CANVAS) || defined(RGB_PANEL) || defined(DSI_PANEL)
  gfx->flush();
#else  // !(defined(CANVAS) || defined(RGB_PANEL) || defined(DSI_PANEL))
#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(0, 0, (uint16_t *)disp_draw_buf, screenWidth, screenHeight);
#else
  gfx->draw16bitRGBBitmap(0, 0, (uint16_t *)disp_draw_buf, screenWidth, screenHeight);
#endif
#endif  // !(defined(CANVAS) || defined(RGB_PANEL) || defined(DSI_PANEL))
#else  // !DIRECT_RENDER_MODE
#ifdef CANVAS
  gfx->flush();
#endif
#endif  // !DIRECT_RENDER_MODE

  delay(30);  // Small delay to allow other tasks to run
}

// Function to adjust brightness (from original code, kept for reference)
void adjust_brightness_event_cb(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED)
  {
    static int currentBrightness = 80;  // Initial brightness, make static to retain state
    if (currentBrightness == 100)
    {
      currentBrightness = 1;  // Go to 1% if already at 100%
    }
    else if (currentBrightness == 1)
    {
      currentBrightness = 10;  // Go to 20% if already at 1%
    }
    else
    {
      currentBrightness += 10;  // Increase by 20%
      if (currentBrightness > 100)
      {
        currentBrightness = 100;  // Cap at 100%
      }
    }
    ledcWrite(GFX_BL, (1 << LEDC_TIMER_10_BIT) / 100 * currentBrightness);
    Serial.printf("Brightness set to: %d%%\n", currentBrightness);  // For debugging
  }
}
