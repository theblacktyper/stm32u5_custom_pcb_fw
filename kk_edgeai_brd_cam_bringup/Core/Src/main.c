/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dcmi.h"
#include "gpdma.h"
#include "i2c.h"
#include "icache.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"
#include "app_x-cube-ai.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "linked_list.h"
#include "b_u585i_iot02a_camera.h"
#include "fonts.h"
#include "st7789.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// Frame header structure for synchronization
typedef struct __attribute__((packed)) {
    uint32_t sync_word;      // 0xDEADBEEF for synchronization
    uint16_t width;
    uint16_t height;
    uint16_t format;         // 0 = RGB565  1 = RGB888
    uint32_t frame_size;     // Total bytes
    uint32_t checksum;       // Simple checksum for validation
} FrameHeader_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ON  1
#define OFF 0

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint32_t g_sysTicks;

extern DMA_QListTypeDef DCMIQueue;

#ifdef USE_RGB565
  __ALIGNED(32) uint8_t CameraBuf[320*240*2];  // *2 = RGB565
#endif
#ifdef USE_RGB888
  __ALIGNED(32) uint8_t CameraBuf[320U * 240U * 3U];  // *3 = RGB888
#endif
//volatile uint8_t frameFlag;
uint8_t led_ready = 0;
uint32_t cam_tick = 0;

volatile uint8_t proc_frame = 0;

// Display buffer for scaled frame (240x135 RGB565)
__ALIGNED(32) uint16_t DisplayBuf[240*135];

uint32_t frameNum = 0;

float inference_res = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
//static uint32_t calculate_checksum(uint8_t *data, uint32_t len);
//static int32_t send_frame_uart(UART_HandleTypeDef *huart, uint8_t *frame_data, uint16_t width, uint16_t height);
static void scale_and_display_frame(uint8_t *src_frame, uint16_t src_width, uint16_t src_height, 
                                     uint16_t *dst_buffer, uint16_t dst_width, uint16_t dst_height);
static void crop_center_scale_and_display_square_rgb888(uint8_t *src_frame,
                                                        uint16_t src_width, uint16_t src_height,
                                                        uint16_t screen_w, uint16_t screen_h);
void crop_center_128x128_rgb888(uint8_t *src_frame, uint16_t src_width, uint16_t src_height,
                                uint8_t *dst_buffer);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t *get_camera_buf(void)
{
	return &CameraBuf[0];
}

// Function to get camera buffer for AI processing (extern declaration in app_x-cube-ai.c)
uint8_t *get_camera_frame_for_ai(void)
{
	return &CameraBuf[0];
}

void BSP_CAMERA_FrameEventCallback(uint32_t Instance)
{
  BSP_CAMERA_Suspend(0);
  //frameFlag = 1;
  
  proc_frame = 1;

  // Scale and display frame on ST7789 display
//  scale_and_display_frame(get_camera_buf(), 320, 240, DisplayBuf, 240, 135);
  
  // Also send frame to UART
//  send_frame_uart(&huart1, get_camera_buf(), 320, 240);
  //memset(CameraBuf, 0, sizeof(CameraBuf));
//  BSP_CAMERA_Resume(0);
}

uint32_t get_ticks(void)
{
	return g_sysTicks;
}
uint32_t msec_since(uint32_t refTick)
{
	uint32_t tick_now = g_sysTicks;
	return (tick_now - refTick);
}

void set_green_led_state(uint8_t en)
{
	if (en)
		HAL_GPIO_WritePin(GRN_LED1_GPIO_Port, GRN_LED1_Pin, GPIO_PIN_RESET);
	else
		HAL_GPIO_WritePin(GRN_LED1_GPIO_Port, GRN_LED1_Pin, GPIO_PIN_SET);
}
void set_red_led_state(uint8_t en)
{
	if (en)
		HAL_GPIO_WritePin(RED_LED2_GPIO_Port, RED_LED2_Pin, GPIO_PIN_RESET);
	else
		HAL_GPIO_WritePin(RED_LED2_GPIO_Port, RED_LED2_Pin, GPIO_PIN_SET);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_GPDMA1_Init();
  MX_DCMI_Init();
  MX_I2C3_Init();
  MX_ICACHE_Init();
  MX_USART1_UART_Init();
  MX_SPI1_Init();
  MX_X_CUBE_AI_Init();
  /* USER CODE BEGIN 2 */
  MX_DCMIQueue_Config();
  HAL_DMAEx_List_LinkQ(&handle_GPDMA1_Channel12, &DCMIQueue);
  __HAL_LINKDMA(&hdcmi, DMA_Handle, handle_GPDMA1_Channel12);

  // Print
//  HAL_UART_Transmit(&huart1, (uint8_t *)"\r\nKenneth's Edge AI Board Bring-up Test\r\n", strlen("\r\nKenneth's Edge AI Board Bring-up Test\r\n"), 1000);

  // Set all LEDs <off>
  set_green_led_state(OFF);
  set_red_led_state(OFF);

  // Initialize Display
  ST7789_Init();
  ST7789_Fill_Color(BLUE);
  uint16_t x, y;
  y = x = 8;
  ST7789_WriteString(x, y, "EDGE AI BOARD", Font_16x26, YELLOW, BLUE);
  ST7789_WriteString(x, y+32, "Starting Person Detection DEMO", Font_7x10, WHITE, BLUE);
  ST7789_WriteString(x, y+52, "Camera Initializing", Font_11x18, RED, BLACK);

  // Initialize camera
#ifdef USE_RGB565
  if (BSP_CAMERA_Init(0, CAMERA_R320x240, CAMERA_PF_RGB565) != BSP_ERROR_NONE)
#endif
#ifdef USE_RGB888
  if (BSP_CAMERA_Init(0, CAMERA_R320x240, CAMERA_PF_RGB888) != BSP_ERROR_NONE)
#endif
  {
    set_red_led_state(ON);
  }
  else
  {

    HAL_Delay(1000); // give the camera time to return good images
//	uint32_t lightMode, colorEffect, mirrorFlip;
//	int32_t brightness, sat, contr, hue;
//	BSP_CAMERA_GetLightMode(0, &lightMode);
//	BSP_CAMERA_GetColorEffect(0, &colorEffect);
//	BSP_CAMERA_GetBrightness(0, &brightness);
//	BSP_CAMERA_GetSaturation(0, &sat);
//	BSP_CAMERA_GetContrast(0, &contr);
//	BSP_CAMERA_GetHueDegree(0, &hue);
//	BSP_CAMERA_GetMirrorFlip(0, &mirrorFlip);
//	char strBuf[150] = {0};
//	sprintf(strBuf, "\r\nLight Mode: %lu\r\nColor Effect: %lu\r\nBrightness: %ld\r\nSaturation: %ld\r\nContrast: %ld\r\nHueDeg: %ld\r\nMirror Flip: %lu\r\n", lightMode, colorEffect, brightness, sat, contr, hue, mirrorFlip);
//	HAL_UART_Transmit(&huart1, (uint8_t *)strBuf, strlen(strBuf), 1000);

	// Set Camera params
	BSP_CAMERA_EnableNightMode(0);
//	BSP_CAMERA_SetBrightness(0, -1);  // 2nd arg = brightness level [-4 , 4]
	HAL_Delay(50);
//	BSP_CAMERA_GetBrightness(0, &brightness);
//	sprintf(strBuf, "\r\nBrightness set %ld\r\n", brightness);
//	HAL_UART_Transmit(&huart1, (uint8_t *)strBuf, strlen(strBuf), 1000);
//	BSP_CAMERA_SetLightMode(0, CAMERA_LIGHT_OFFICE);
//	HAL_Delay(50);
	BSP_CAMERA_SetMirrorFlip(0, CAMERA_MIRRORFLIP_MIRROR);
	HAL_Delay(50);

    // Take snapshot
//    frameFlag = 0;
	ST7789_Fill_Color(BLUE);
	ST7789_WriteString(x, y,    "Press Button 2", Font_11x18, BLACK, YELLOW);
	ST7789_WriteString(x, y+18, "To Begin DEMO!", Font_11x18, BLACK, YELLOW);
    while (GPIO_PIN_RESET == HAL_GPIO_ReadPin(BUTTON2_GPIO_Port, BUTTON2_Pin));

    // Start Camera
    BSP_CAMERA_Start(0, get_camera_buf(), CAMERA_MODE_CONTINUOUS);

    cam_tick = get_ticks();
//    led_ready = 1;
//    set_green_led_state(ON);
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	if (proc_frame) {
	  // Run AI inference on the new frame (crops center 128x128 internally)
	  MX_X_CUBE_AI_Process();

	  // Scale and display frame on ST7789 display (uses row-by-row DMA, no full buffer needed)
	  crop_center_scale_and_display_square_rgb888(get_camera_buf(), 320, 240, 240, 135);
	  // (Opt.) Also send frame to UART mainly for debug purpose
//	  send_frame_uart(&huart1, get_camera_buf(), 320, 240);
//	  set_green_led_state(OFF);
      proc_frame = 0;
    }
	else if (msec_since(cam_tick) > MSEC_BTWN_IMG_CAPTURES) {
	  // UNCOMMENT these two lines to debug camera FPS:
//	  set_green_led_state(OFF);
//	  set_red_led_state(OFF);
      BSP_CAMERA_Resume(0);
      cam_tick = get_ticks();
//      set_green_led_state(ON);
    }
    /* USER CODE END WHILE */

//  MX_X_CUBE_AI_Process();
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_0;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV4;
  RCC_OscInitStruct.PLL.PLLM = 3;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 1;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLLVCIRANGE_1;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
// Simple checksum calculation
//static inline uint32_t calculate_checksum(uint8_t *data, uint32_t len)
//{
//    uint32_t sum = 0;
//    for(uint32_t i = 0; i < len; i++) {
//        sum += data[i];
//    }
//    return sum;
//}

// Send frame over UART
//static int32_t send_frame_uart(UART_HandleTypeDef *huart, uint8_t *frame_data, uint16_t width, uint16_t height)
//{
////    FrameHeader_t header;
////    header.sync_word = 0xDEADBEEF;
////    header.width = width;
////    header.height = height;
////    header.format = 0;  // RGB565
//////    header.format = 1;  // RGB888
////    header.frame_size = width * height * 2;  // *2 for RGB565
//////    header.frame_size = 320 * 240 * 3;  // 230,400 bytes
//////    header.frame_size = 480 * 272 * 3;
////    header.checksum = calculate_checksum(frame_data, header.frame_size);
////
////    // Send header
////    HAL_UART_Transmit(huart, (uint8_t*)&header, sizeof(FrameHeader_t), HAL_MAX_DELAY);
//
//    // Send frame data in chunks to avoid timeout
//	uint32_t data_len = sizeof(CameraBuf) - 4;
//    uint32_t bytes_sent = 0;
//    uint32_t chunk_size = 4096;  // Adjust based on your UART buffer
//
//    // Send Sync Word
//    // fill in SYNC WORD
//    uint8_t syncBuf[4];
//    *((uint32_t *)syncBuf) = 0xDEADBEEF;
//    HAL_UART_Transmit(huart, syncBuf, sizeof(syncBuf), 1000);
//
//    while(bytes_sent < data_len) {
//        uint32_t remaining = data_len - bytes_sent;
//        uint32_t to_send = (remaining > chunk_size) ? chunk_size : remaining;
//
//        HAL_StatusTypeDef status = HAL_UART_Transmit(huart,
//                                                      &frame_data[bytes_sent],
//                                                      to_send,
//                                                      1000);
//        if(status != HAL_OK) {
//            // Handle error
//        	set_green_led_state(OFF);
//            return -1;
//        }
//        bytes_sent += to_send;
//    }
//
//    set_green_led_state(OFF);
//    return 0;
//}

/**
 * @brief Scale RGB565 frame from source resolution to destination resolution
 * @param src_frame Pointer to source frame data (RGB565, 2 bytes per pixel)
 * @param src_width Source frame width
 * @param src_height Source frame height
 * @param dst_buffer Pointer to destination buffer (RGB565, 2 bytes per pixel)
 * @param dst_width Destination width
 * @param dst_height Destination height
 * @return None
 */
static void scale_and_display_frame(uint8_t *src_frame, uint16_t src_width, uint16_t src_height,
                                    uint16_t *dst_buffer, uint16_t dst_width, uint16_t dst_height)
{
    uint16_t x, y;
    uint16_t src_x, src_y;

    for (y = 0; y < dst_height; y++) {
        src_y = (uint32_t)y * src_height / dst_height;

        for (x = 0; x < dst_width; x++) {
            src_x = (uint32_t)x * src_width / dst_width;

            // RGB888: R,G,B (3 bytes per pixel)
            uint32_t src_idx = ((uint32_t)src_y * src_width + src_x) * 3;

//            uint8_t r = src_frame[src_idx + 0];
//            uint8_t g = src_frame[src_idx + 1];
//            uint8_t b = src_frame[src_idx + 2];

            uint8_t b = src_frame[src_idx + 0];
            uint8_t g = src_frame[src_idx + 1];
            uint8_t r = src_frame[src_idx + 2];

            // Convert RGB888 -> RGB565 (in CPU-native endian)
            uint16_t pixel565 =
                (uint16_t)(((r & 0xF8) << 8) |
                           ((g & 0xFC) << 3) |
                           ((b & 0xF8) >> 3));

            // ST7789 expects MSB first on the wire; HAL_SPI_Transmit sends bytes in memory order
            dst_buffer[y * dst_width + x] = __REV16(pixel565);
        }
    }

    ST7789_DrawImage(0, 0, dst_width, dst_height, dst_buffer);

    static char osd_buf[16];
    sprintf(osd_buf, "FRAME #%lu", ++frameNum);
    ST7789_WriteString(4, 4, osd_buf, Font_11x18, WHITE, BLACK);
//#ifdef USE_RGB565
//    ST7789_WriteString(170, 113, "RGB565", Font_11x18, BLACK, YELLOW);
//#endif
//#ifdef USE_RGB888
//    ST7789_WriteString(170, 113, "RGB888", Font_11x18, BLACK, YELLOW);
//#endif
}

/**
 * @brief Crop center square region, scale, and display using row-by-row DMA
 * @param src_frame Pointer to source frame data (RGB888, 3 bytes per pixel, BGR888 format from DCMI)
 * @param src_width Source frame width (320)
 * @param src_height Source frame height (240)
 * @param screen_w Screen width (240)
 * @param screen_h Screen height (135)
 * @return None
 * 
 * This function uses row-by-row processing to minimize RAM usage.
 * It processes one row at a time and sends it via SPI DMA.
 */
static void crop_center_scale_and_display_square_rgb888(uint8_t *src_frame,
                                                        uint16_t src_width, uint16_t src_height,
                                                        uint16_t screen_w, uint16_t screen_h)
{
    // 1) Center crop parameters: 320x240 -> 128x128 (matches AI model input exactly)
    const uint16_t crop_w = 128;
    const uint16_t crop_h = 128;

    uint16_t x_off = (src_width  - crop_w) / 2;  // 96 for 320->128
    uint16_t y_off = (src_height - crop_h) / 2;  // 56 for 240->128

    // 2) Display size: scale 128x128 to 135x135 (to fit on screen)
    const uint16_t display_size = 135;  // Display as 135x135 square

    // Center the square on screen
    uint16_t dst_x0 = (screen_w - display_size) / 2; // (240-135)/2 = 52
    uint16_t dst_y0 = (screen_h - display_size) / 2; // (135-135)/2 = 0

    // 3) Row buffer for DMA (one row = screen width)
    static uint16_t row_buf[ST7789_WIDTH];  // 240 pixels = 480 bytes
    
    // 4) Set address window for full screen (replicate logic from ST7789_SetAddressWindow)
    ST7789_Select();
    {
        uint16_t x_start = 0 + 40, x_end = (screen_w - 1) + 40;  // X_SHIFT = 40 for rotation 1
        uint16_t y_start = 0 + 52, y_end = (screen_h - 1) + 52;  // Y_SHIFT = 52 for rotation 1
        
        /* Column Address set */
        ST7789_DC_Clr();
        uint8_t cmd = 0x2A;  // ST7789_CASET
        HAL_SPI_Transmit(&ST7789_SPI_PORT, &cmd, 1, HAL_MAX_DELAY);
        ST7789_DC_Set();
        {
            uint8_t data[] = {x_start >> 8, x_start & 0xFF, x_end >> 8, x_end & 0xFF};
            HAL_SPI_Transmit(&ST7789_SPI_PORT, data, sizeof(data), HAL_MAX_DELAY);
        }
        
        /* Row Address set */
        ST7789_DC_Clr();
        cmd = 0x2B;  // ST7789_RASET
        HAL_SPI_Transmit(&ST7789_SPI_PORT, &cmd, 1, HAL_MAX_DELAY);
        ST7789_DC_Set();
        {
            uint8_t data[] = {y_start >> 8, y_start & 0xFF, y_end >> 8, y_end & 0xFF};
            HAL_SPI_Transmit(&ST7789_SPI_PORT, data, sizeof(data), HAL_MAX_DELAY);
        }
        
        /* Write to RAM */
        ST7789_DC_Clr();
        cmd = 0x2C;  // ST7789_RAMWR
        HAL_SPI_Transmit(&ST7789_SPI_PORT, &cmd, 1, HAL_MAX_DELAY);
        ST7789_DC_Set();
    }

    // 5) Process and send row by row
    const uint16_t DMA_MIN_SIZE = 16;  // Minimum size for DMA transfer
    for (uint16_t screen_y = 0; screen_y < screen_h; screen_y++) {
        // Clear row buffer to black (for areas outside the square)
        for (uint16_t x = 0; x < screen_w; x++) {
            row_buf[x] = 0x0000;  // Black (RGB565, byte-swapped)
        }

        // If this row is within the square region, process it
        if (screen_y >= dst_y0 && screen_y < (dst_y0 + display_size)) {
            uint16_t display_y = screen_y - dst_y0;  // Position within display square (0..134)
            uint16_t cy = (uint32_t)display_y * crop_h / display_size;  // Source crop Y (0..127)
            uint16_t sy = y_off + cy;  // Source frame Y

            // Process pixels in this row
            for (uint16_t screen_x = 0; screen_x < display_size; screen_x++) {
                uint16_t cx = (uint32_t)screen_x * crop_w / display_size; // Source crop X (0..127)
                uint16_t sx = x_off + cx;  // Source frame X

                // Get source pixel (BGR888 format from DCMI)
                uint32_t src_idx = ((uint32_t)sy * src_width + sx) * 3;
                uint8_t b = src_frame[src_idx + 0];  // Blue from DCMI
                uint8_t g = src_frame[src_idx + 1];  // Green from DCMI
                uint8_t r = src_frame[src_idx + 2];  // Red from DCMI

                // Convert RGB888 -> RGB565
                uint16_t pixel565 = (uint16_t)(((r & 0xF8) << 8) |
                                               ((g & 0xFC) << 3) |
                                               ((b & 0xF8) >> 3));

                // Swap bytes for ST7789 SPI byte-stream and place in row buffer
                row_buf[dst_x0 + screen_x] = __REV16(pixel565);
            }
        }

        // Send row via DMA (replicate ST7789_WriteData logic)
        size_t buff_size = sizeof(uint16_t) * screen_w;
        uint8_t *buff = (uint8_t *)row_buf;
        
        while (buff_size > 0) {
            uint16_t chunk_size = buff_size > 65535 ? 65535 : buff_size;
            if (DMA_MIN_SIZE <= buff_size) {
                HAL_SPI_Transmit_DMA(&ST7789_SPI_PORT, buff, chunk_size);
                while (ST7789_SPI_PORT.hdmatx->State != HAL_DMA_STATE_READY) {}
            } else {
                HAL_SPI_Transmit(&ST7789_SPI_PORT, buff, chunk_size, HAL_MAX_DELAY);
            }
            buff += chunk_size;
            buff_size -= chunk_size;
        }
    }

    ST7789_UnSelect();

    // 6) Draw OSD text
    static char osd_buf[16];
    sprintf(osd_buf, "FRAME");
    ST7789_WriteString(4, 4, osd_buf, Font_7x10, WHITE, BLACK);
    sprintf(osd_buf, "#%lu", ++frameNum);
    ST7789_WriteString(4, 16, osd_buf, Font_7x10, WHITE, BLACK);
//#ifdef USE_RGB565
//    ST7789_WriteString(170, 113, "RGB565", Font_11x18, BLACK, YELLOW);
//#endif
//#ifdef USE_RGB888
//    ST7789_WriteString(170, 113, "RGB888", Font_11x18, BLACK, YELLOW);
//#endif
    sprintf(osd_buf, "Person: %u%%", (unsigned int)(inference_res*100.f));
    ST7789_WriteString((240 - (16*11 + 4)), 113, osd_buf, Font_11x18, BLACK, YELLOW);
}

/**
 * @brief Crop center 128x128 region from RGB888 source frame for AI model input
 * @param src_frame Pointer to source frame data (RGB888, 3 bytes per pixel)
 *                  Note: DCMI stores as BGR888 (B, G, R), but we convert to RGB888 for model
 * @param src_width Source frame width (320)
 * @param src_height Source frame height (240)
 * @param dst_buffer Pointer to destination buffer (128x128x3 = 49152 bytes, RGB888 format)
 * @return None
 */
void crop_center_128x128_rgb888(uint8_t *src_frame, uint16_t src_width, uint16_t src_height,
                                uint8_t *dst_buffer)
{
    const uint16_t crop_size = 128;
    
    // Calculate center crop offsets
    // For 320x240: crop 128x128 centered
    // X offset: (320 - 128) / 2 = 96
    // Y offset: (240 - 128) / 2 = 56
    uint16_t x_offset = (src_width - crop_size) / 2;
    uint16_t y_offset = (src_height - crop_size) / 2;
    
    // Copy cropped region pixel by pixel
    for (uint16_t y = 0; y < crop_size; y++) {
        uint16_t src_y = y_offset + y;
        
        for (uint16_t x = 0; x < crop_size; x++) {
            uint16_t src_x = x_offset + x;
            
            // Source pixel index (BGR888 format from DCMI)
            uint32_t src_idx = ((uint32_t)src_y * src_width + src_x) * 3;
            
            // Destination pixel index (RGB888 format for model)
            uint32_t dst_idx = ((uint32_t)y * crop_size + x) * 3;
            
            // DCMI stores as BGR888, convert to RGB888 for TensorFlow model
            uint8_t b = src_frame[src_idx + 0];  // Blue from DCMI
            uint8_t g = src_frame[src_idx + 1];  // Green from DCMI
            uint8_t r = src_frame[src_idx + 2];  // Red from DCMI
            
            // Store as RGB888 for model input
            dst_buffer[dst_idx + 0] = r;  // Red
            dst_buffer[dst_idx + 1] = g;  // Green
            dst_buffer[dst_idx + 2] = b;  // Blue
        }
    }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
