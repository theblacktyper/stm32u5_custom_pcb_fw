
/**
  ******************************************************************************
  * @file    app_x-cube-ai.c
  * @author  X-CUBE-AI C code generator
  * @brief   AI program body
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

 /*
  * Description
  *   v1.0 - Minimum template to show how to use the Embedded Client API
  *          model. Only one input and one output is supported. All
  *          memory resources are allocated statically (AI_NETWORK_XX, defines
  *          are used).
  *          Re-target of the printf function is out-of-scope.
  *   v2.0 - add multiple IO and/or multiple heap support
  *
  *   For more information, see the embeded documentation:
  *
  *       [1] %X_CUBE_AI_DIR%/Documentation/index.html
  *
  *   X_CUBE_AI_DIR indicates the location where the X-CUBE-AI pack is installed
  *   typical : C:\Users\[user_name]\STM32Cube\Repository\STMicroelectronics\X-CUBE-AI\7.1.0
  */

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#if defined ( __ICCARM__ )
#elif defined ( __CC_ARM ) || ( __GNUC__ )
#endif

/* System headers */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "app_x-cube-ai.h"
#include "main.h"
#include "ai_datatypes_defines.h"
#include "p_det_model.h"
#include "p_det_model_data.h"

/* USER CODE BEGIN includes */
extern float inference_res;

// Forward declaration for camera buffer access
extern uint8_t *get_camera_frame_for_ai(void);
extern void crop_center_128x128_rgb888(uint8_t *src_frame, uint16_t src_width, uint16_t src_height,
                                       uint8_t *dst_buffer);
// Forward declaration for LED control
extern void set_green_led_state(uint8_t en);
extern void set_red_led_state(uint8_t en);
/* USER CODE END includes */

/* IO buffers ----------------------------------------------------------------*/

#if !defined(AI_P_DET_MODEL_INPUTS_IN_ACTIVATIONS)
AI_ALIGNED(4) ai_i8 data_in_1[AI_P_DET_MODEL_IN_1_SIZE_BYTES];
ai_i8* data_ins[AI_P_DET_MODEL_IN_NUM] = {
data_in_1
};
#else
ai_i8* data_ins[AI_P_DET_MODEL_IN_NUM] = {
NULL
};
#endif

#if !defined(AI_P_DET_MODEL_OUTPUTS_IN_ACTIVATIONS)
AI_ALIGNED(4) ai_i8 data_out_1[AI_P_DET_MODEL_OUT_1_SIZE_BYTES];
ai_i8* data_outs[AI_P_DET_MODEL_OUT_NUM] = {
data_out_1
};
#else
ai_i8* data_outs[AI_P_DET_MODEL_OUT_NUM] = {
NULL
};
#endif

/* Activations buffers -------------------------------------------------------*/

AI_ALIGNED(32)
static uint8_t pool0[AI_P_DET_MODEL_DATA_ACTIVATION_1_SIZE];

ai_handle data_activations0[] = {pool0};

/* AI objects ----------------------------------------------------------------*/

static ai_handle p_det_model = AI_HANDLE_NULL;

static ai_buffer* ai_input;
static ai_buffer* ai_output;

static void ai_log_err(const ai_error err, const char *fct)
{
  /* USER CODE BEGIN log */
  if (fct)
    printf("TEMPLATE - Error (%s) - type=0x%02x code=0x%02x\r\n", fct,
        err.type, err.code);
  else
    printf("TEMPLATE - Error - type=0x%02x code=0x%02x\r\n", err.type, err.code);

  do {} while (1);
  /* USER CODE END log */
}

static int ai_boostrap(ai_handle *act_addr)
{
  ai_error err;

  /* Create and initialize an instance of the model */
  err = ai_p_det_model_create_and_init(&p_det_model, act_addr, NULL);
  if (err.type != AI_ERROR_NONE) {
    ai_log_err(err, "ai_p_det_model_create_and_init");
    return -1;
  }

  ai_input = ai_p_det_model_inputs_get(p_det_model, NULL);
  ai_output = ai_p_det_model_outputs_get(p_det_model, NULL);

#if defined(AI_P_DET_MODEL_INPUTS_IN_ACTIVATIONS)
  /*  In the case where "--allocate-inputs" option is used, memory buffer can be
   *  used from the activations buffer. This is not mandatory.
   */
  for (int idx=0; idx < AI_P_DET_MODEL_IN_NUM; idx++) {
	data_ins[idx] = ai_input[idx].data;
  }
#else
  for (int idx=0; idx < AI_P_DET_MODEL_IN_NUM; idx++) {
	  ai_input[idx].data = data_ins[idx];
  }
#endif

#if defined(AI_P_DET_MODEL_OUTPUTS_IN_ACTIVATIONS)
  /*  In the case where "--allocate-outputs" option is used, memory buffer can be
   *  used from the activations buffer. This is no mandatory.
   */
  for (int idx=0; idx < AI_P_DET_MODEL_OUT_NUM; idx++) {
	data_outs[idx] = ai_output[idx].data;
  }
#else
  for (int idx=0; idx < AI_P_DET_MODEL_OUT_NUM; idx++) {
	ai_output[idx].data = data_outs[idx];
  }
#endif

  return 0;
}

static int ai_run(void)
{
  ai_i32 batch;

  batch = ai_p_det_model_run(p_det_model, ai_input, ai_output);
  if (batch != 1) {
    ai_log_err(ai_p_det_model_get_error(p_det_model),
        "ai_p_det_model_run");
    return -1;
  }

  return 0;
}

/* USER CODE BEGIN 2 */
int acquire_and_process_data(ai_i8* data[])
{
  /* fill the inputs of the c-model
   * Model expects: 128x128x3 RGB888 format (49152 bytes)
   * Camera provides: 320x240x3 RGB888 format (but stored as BGR888 by DCMI)
   */
  
  // Get pointer to camera frame buffer
  uint8_t *camera_frame = get_camera_frame_for_ai();
  
  if (camera_frame == NULL) {
    return -1;  // No frame available
  }
  
  // Crop center 128x128 from 320x240 frame and convert BGR888 to RGB888
  // data[0] points to data_in_1 buffer which is AI_P_DET_MODEL_IN_1_SIZE_BYTES (49152 bytes)
  // This crops the center region: X offset = (320-128)/2 = 96, Y offset = (240-128)/2 = 56
  // NOTE: this function is now repurposed to just feed camera buf as-is to model since we are using HW cropping.
  crop_center_128x128_rgb888(camera_frame, 128, 128, (uint8_t*)data[0]);
  
  return 0;
}

int post_process(ai_i8* data[])
{
  /* process the predictions
   * Model output: single float value (4 bytes) representing person detection probability
   * Format: AI_BUFFER_FORMAT_FLOAT, size: 1, bytes: 4
   * Interpretation: 
   *   - Value > 0.5 (or > 0.0 for logit) = person detected
   *   - Value <= 0.5 (or <= 0.0 for logit) = no person
   */
  
  if (data == NULL || data[0] == NULL) {
    return -1;  // Invalid output data
  }
  
  // Get the output value as float
  // The output is a single float (4 bytes) at data[0]
  float *output_value = (float *)data[0];
  float person_score = output_value[0];
  inference_res = person_score;
  
  // Interpret the output
  // For binary classification with sigmoid activation, threshold is typically 0.5
  // However, if the model outputs logits, threshold would be 0.0
  // Based on the model report showing LOGISTIC activation, output should be 0-1 probability
  // Using 0.5 as threshold for person detection
  const float PERSON_THRESHOLD = 0.66f;//0.5f;
  
  if (person_score > PERSON_THRESHOLD) {
    // Person detected - set GREEN LED on, RED LED off
    set_green_led_state(1);  // ON
    set_red_led_state(0);    // OFF
  } else {
    // No person detected - set RED LED on, GREEN LED off
    set_green_led_state(0);  // OFF
    set_red_led_state(1);    // ON
  }
  
  return 0;
}
/* USER CODE END 2 */

/* Entry points --------------------------------------------------------------*/

void MX_X_CUBE_AI_Init(void)
{
    /* USER CODE BEGIN 5 */
  printf("\r\nTEMPLATE - initialization\r\n");

  ai_boostrap(data_activations0);
    /* USER CODE END 5 */
}

void MX_X_CUBE_AI_Process(void)
{
    /* USER CODE BEGIN 6 */
  int res = -1;

  if (p_det_model) {
    /* 1 - acquire and pre-process input data */
    res = acquire_and_process_data(data_ins);
    /* 2 - process the data - call inference engine */
    if (res == 0)
      res = ai_run();
    /* 3- post-process the predictions */
    if (res == 0)
      res = post_process(data_outs);
  }

  if (res && res != -1) {
    ai_error err = {AI_ERROR_INVALID_STATE, AI_ERROR_CODE_NETWORK};
    ai_log_err(err, "Process has FAILED");
  }
    /* USER CODE END 6 */
}
#ifdef __cplusplus
}
#endif
