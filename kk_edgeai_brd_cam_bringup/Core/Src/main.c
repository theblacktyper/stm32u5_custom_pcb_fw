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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
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

//uint32_t CameraBuf[480*272*3/4];
__ALIGNED(32) uint8_t CameraBuf[320*240*2+4];  // *3 = RGB888  *2 = RGB565
//volatile uint8_t frameFlag;
uint8_t led_ready = 0;
uint32_t cam_tick = 0;

volatile uint8_t proc_frame = 0;

// Display buffer for scaled frame (240x135 RGB565)
__ALIGNED(32) uint16_t DisplayBuf[240*135];

uint32_t frameNum = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
//static uint32_t calculate_checksum(uint8_t *data, uint32_t len);
//static int32_t send_frame_uart(UART_HandleTypeDef *huart, uint8_t *frame_data, uint16_t width, uint16_t height);
static void scale_and_display_frame(uint8_t *src_frame, uint16_t src_width, uint16_t src_height, 
                                     uint16_t *dst_buffer, uint16_t dst_width, uint16_t dst_height);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t *get_camera_buf(void)
{
	return &CameraBuf[4];
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
  /* USER CODE BEGIN 2 */
  MX_DCMIQueue_Config();
  HAL_DMAEx_List_LinkQ(&handle_GPDMA1_Channel12, &DCMIQueue);
  __HAL_LINKDMA(&hdcmi, DMA_Handle, handle_GPDMA1_Channel12);

  // Print
//  HAL_UART_Transmit(&huart1, (uint8_t *)"\r\nKenneth's Edge AI Board Bring-up Test\r\n", strlen("\r\nKenneth's Edge AI Board Bring-up Test\r\n"), 1000);

  // Light show
  set_green_led_state(ON);
  for (int i=0; i<10; i++) {
	  HAL_Delay(250);
	  HAL_GPIO_TogglePin(GRN_LED1_GPIO_Port, GRN_LED1_Pin);
	  HAL_GPIO_TogglePin(RED_LED2_GPIO_Port, RED_LED2_Pin);
  }
  set_green_led_state(OFF);
  set_red_led_state(OFF);

  // Fill Disp + Text
  ST7789_Init();
  ST7789_Fill_Color(BLUE);
  uint16_t x, y;
  y = x = 8;
  ST7789_WriteString(x, y, "HELLO!", Font_16x26, YELLOW, BLUE);
  ST7789_WriteString(x, y+32, "Welcome to KK's Edge AI Board", Font_7x10, WHITE, BLUE);
  ST7789_WriteString(x, y+64, "Press Button 2", Font_11x18, MAGENTA, BLUE);

  // Initialize camera
  if (BSP_CAMERA_Init(0, CAMERA_R320x240, CAMERA_PF_RGB565) != BSP_ERROR_NONE)
  {
    set_red_led_state(ON);
  }
  else
  {
//    HAL_Delay(1000); // give the camera time to return good images
	uint32_t lightMode, colorEffect, mirrorFlip;
	int32_t brightness, sat, contr, hue;
	BSP_CAMERA_GetLightMode(0, &lightMode);
	BSP_CAMERA_GetColorEffect(0, &colorEffect);
	BSP_CAMERA_GetBrightness(0, &brightness);
	BSP_CAMERA_GetSaturation(0, &sat);
	BSP_CAMERA_GetContrast(0, &contr);
	BSP_CAMERA_GetHueDegree(0, &hue);
	BSP_CAMERA_GetMirrorFlip(0, &mirrorFlip);
	char strBuf[150] = {0};
	sprintf(strBuf, "\r\nLight Mode: %lu\r\nColor Effect: %lu\r\nBrightness: %ld\r\nSaturation: %ld\r\nContrast: %ld\r\nHueDeg: %ld\r\nMirror Flip: %lu\r\n", lightMode, colorEffect, brightness, sat, contr, hue, mirrorFlip);
	HAL_UART_Transmit(&huart1, (uint8_t *)strBuf, strlen(strBuf), 1000);

	// Set Camera params
//	BSP_CAMERA_EnableNightMode(0);
//	BSP_CAMERA_SetBrightness(0, 4);  // 2nd arg = brightness level [-4 , 4]
//	HAL_Delay(20);
//	BSP_CAMERA_GetBrightness(0, &brightness);
//	sprintf(strBuf, "\r\nBrightness set %ld\r\n", brightness);
//	HAL_UART_Transmit(&huart1, (uint8_t *)strBuf, strlen(strBuf), 1000);
	BSP_CAMERA_SetLightMode(0, CAMERA_LIGHT_CLOUDY);
	HAL_Delay(20);
	BSP_CAMERA_SetMirrorFlip(0, CAMERA_MIRRORFLIP_MIRROR);
	HAL_Delay(20);

    // Take snapshot
//    frameFlag = 0;
    while (GPIO_PIN_RESET == HAL_GPIO_ReadPin(BUTTON2_GPIO_Port, BUTTON2_Pin));
    // fill in SYNC WORD
    *((uint32_t *)CameraBuf) = 0xDEADBEEF;
    BSP_CAMERA_Start(0, get_camera_buf(), CAMERA_MODE_CONTINUOUS);
    cam_tick = get_ticks();
//    led_ready = 1;
    set_green_led_state(ON);
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    if (msec_since(cam_tick) > 1000) {
      BSP_CAMERA_Resume(0);
      cam_tick = get_ticks();
      set_green_led_state(ON);
    }
    else if (proc_frame) {
	  // Scale and display frame on ST7789 display
	  scale_and_display_frame(get_camera_buf(), 320, 240, DisplayBuf, 240, 135);
	  // Also send frame to UART
//	  send_frame_uart(&huart1, get_camera_buf(), 320, 240);
	  set_green_led_state(OFF);
      proc_frame = 0;
    }
    /* USER CODE END WHILE */

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
//    HAL_UART_Transmit(huart, CameraBuf, 4, 1000);
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
    uint16_t pixel;
    uint16_t *src_pixel_ptr;
    
    // Scale using nearest neighbor interpolation
    for (y = 0; y < dst_height; y++) {
        // Calculate source Y position with scaling
        src_y = (y * src_height) / dst_height;
        
        for (x = 0; x < dst_width; x++) {
            // Calculate source X position with scaling
            src_x = (x * src_width) / dst_width;
            
            // Get source pixel (RGB565 is 2 bytes per pixel)
            // Source frame is stored as uint8_t array, so we need to read 2 bytes
            src_pixel_ptr = (uint16_t *)(src_frame + (src_y * src_width + src_x) * 2);
            pixel = *src_pixel_ptr;
            
            // Swap bytes - DCMI stores RGB565 in big-endian format, but we need little-endian
            // This fixes color issues (red/blue swapped, etc.)
            pixel = __REV16(pixel);
            
            // Store in destination buffer
            dst_buffer[y * dst_width + x] = pixel;
        }
    }
    
    // Display the scaled frame on ST7789 (full screen)
    ST7789_DrawImage(0, 0, dst_width, dst_height, dst_buffer);

    static char osd_buf[16];
    sprintf(osd_buf, "FRAME #%lu", ++frameNum);
    ST7789_WriteString(4, 4, osd_buf, Font_11x18, WHITE, BLACK);
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
