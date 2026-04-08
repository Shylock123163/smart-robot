/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "encoder.h"
#include "usart.h"
#include "motor.h"
#include "servo.h"
#include "tim.h"
#include "vl53l0x.h"
#include "robot.h"
#include "chassis.h"
#include "HWT101.h"
#include "ws2812b.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SEARCH_SPEED                40
#define APPROACH_SPEED              28
#define EXIT_SPEED                  28
#define BACKOUT_SPEED              (-25)
#define SEARCH_TIMEOUT_MS        300000U
#define FORWARD_TIMEOUT_MS        8000U
#define GRAB_TIMEOUT_MS           2500U
#define TURN_TIMEOUT_MS           5000U
#define EXIT_TIMEOUT_MS           8000U
#define TOP_EXIT_DELTA_MM          120U
#define TOP_EXIT_COUNT_TH            5U
#define VISION_STABLE_COUNT          3U
#define SERVO_MOVE_TIME_MS         700U
#define DEFAULT_TURN_DIR   TURN_DIR_RIGHT
#define VISION_RX_BUF_SIZE         64U
/* 这里先按 1 count = 1 mm 留接口，后面按实车再标定 */
#define ENCODER_MM_PER_COUNT      1.0f


  #define ROBOT_SM_TEST_COMM    0
  #define ROBOT_SM_TEST_SENSOR  0
  #define ROBOT_SM_TEST_SERVO   0
  #define ROBOT_SM_TEST_IMU     0
  #define ROBOT_SM_DEBUG_PRINT  1


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
volatile int g_enc1 = 0;
volatile int g_enc2 = 0;
volatile int g_enc3 = 0;
volatile int g_enc4 = 0;

volatile int16_t g_target_speed[4] = {0, 0, 0, 0};

volatile RobotState_t g_robot_state = STATE_BOOT_PREPARE;

volatile uint8_t g_vision_detected = 0;
volatile uint8_t g_vision_position = 0;
volatile uint16_t g_vision_distance = 0;
volatile uint16_t g_target_forward_mm = 0;
volatile uint16_t g_forward_progress_mm = 0;
volatile CloseReason_t g_close_reason = CLOSE_REASON_NONE;
volatile TurnDir_t g_turn_dir = TURN_DIR_NONE;

volatile uint8_t g_bumper_left = 0;
volatile uint8_t g_bumper_right = 0;
volatile uint8_t g_grab_switch_front = 0;
volatile uint8_t g_front_switch_triggered = 0;

volatile uint16_t g_dist_left = 9999;
volatile uint16_t g_dist_top = 9999;
volatile uint16_t g_dist_front = 9999;
volatile uint16_t g_dist_right = 9999;
volatile uint16_t g_top_inside_ref = 0;
volatile uint8_t g_exit_confirmed = 0;

volatile uint32_t g_state_timer = 0;
volatile ServoCmd_t g_servo_cmd = SERVO_CMD_NONE;
volatile uint8_t g_servo_busy = 0;

static uint32_t s_forward_encoder_accum = 0;
static uint8_t s_vision_rx_buf[VISION_RX_BUF_SIZE];
static volatile uint16_t s_vision_rx_size = 0;
static volatile uint8_t s_vision_rx_ready = 0;
/* USER CODE END Variables */
osThreadId MotorTaskHandle;
osThreadId EncoderTaskHandle;
osThreadId IMUTaskHandle;
osThreadId SensorTaskHandle;
osThreadId ServoTaskHandle;
osThreadId LedTaskHandle;
osThreadId CommTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static uint8_t VisionDetectedStable(void);
static uint8_t StateTimedOut(uint32_t timeout_ms);
static void EnterState(RobotState_t next_state);
static uint8_t RobotNeedLight(void);
static uint32_t Robot_AbsS32(int32_t value);
static void Robot_ClearFrontSwitchLatch(void);
static void Robot_ResetForwardProgress(void);
static void Robot_UpdateEncoderSnapshot(void);
static void Robot_UpdateForwardProgress(void);
static void Robot_VisionRxStart(void);
__weak void Robot_VisionProtocol_Poll(void);
 static void Robot_StateMachineTestStep(void);

 static const char *Robot_StateName(RobotState_t
   state);
  static void Robot_StateMachineTestStep(void);
/* USER CODE END FunctionPrototypes */

void StartMotorTask(void const * argument);
void StartEncoderTask(void const * argument);
void StartIMUTask(void const * argument);
void StartSensorTask(void const * argument);
void StartServoTask(void const * argument);
void StartLedTask(void const * argument);
void StartCommTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  osThreadDef(MotorTask, StartMotorTask, osPriorityHigh, 0, 512);
  MotorTaskHandle = osThreadCreate(osThread(MotorTask), NULL);

  osThreadDef(EncoderTask, StartEncoderTask, osPriorityAboveNormal, 0, 512);
  EncoderTaskHandle = osThreadCreate(osThread(EncoderTask), NULL);

  osThreadDef(IMUTask, StartIMUTask, osPriorityNormal, 0, 512);
  IMUTaskHandle = osThreadCreate(osThread(IMUTask), NULL);

  osThreadDef(SensorTask, StartSensorTask, osPriorityNormal, 0, 512);
  SensorTaskHandle = osThreadCreate(osThread(SensorTask), NULL);

  osThreadDef(ServoTask, StartServoTask, osPriorityBelowNormal, 0, 256);
  ServoTaskHandle = osThreadCreate(osThread(ServoTask), NULL);

  osThreadDef(LedTask, StartLedTask, osPriorityLow, 0, 256);
  LedTaskHandle = osThreadCreate(osThread(LedTask), NULL);

  osThreadDef(CommTask, StartCommTask, osPriorityLow, 0, 1024);
  CommTaskHandle = osThreadCreate(osThread(CommTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* USER CODE END RTOS_THREADS */
}

void StartMotorTask(void const * argument)
{
  /* USER CODE BEGIN StartMotorTask */
  RobotState_t last_state;
  uint8_t entered;

  (void)argument;
  Motor_Init();
  Chassis_Init();
  last_state = STATE_ERROR;
  EnterState(STATE_BOOT_PREPARE);

  for (;;)
  {
    entered = (g_robot_state != last_state) ? 1U : 0U;
    if (entered != 0U)
    {
      last_state = g_robot_state;
    }

    switch (g_robot_state)
    {
      case STATE_BOOT_PREPARE:
        if (entered != 0U)
        {
          Chassis_Stop();
          Robot_ClearFrontSwitchLatch();
          Robot_ResetForwardProgress();
          g_target_forward_mm = 0;
          g_close_reason = CLOSE_REASON_NONE;
          g_turn_dir = TURN_DIR_NONE;
          g_exit_confirmed = 0U;
          g_top_inside_ref = 0U;
          g_servo_cmd = SERVO_CMD_OPEN;
        }

        if ((g_servo_busy == 0U) && (g_servo_cmd == SERVO_CMD_NONE))
        {
          EnterState(STATE_SEARCH_STRAFE);
        }
        break;

      case STATE_SEARCH_STRAFE:
        Chassis_RunStrafeLeftPID(SEARCH_SPEED);
        if (VisionDetectedStable() != 0U)
        {
          Chassis_Stop();
          EnterState(STATE_LOCK_TARGET);
        }
        else if (StateTimedOut(SEARCH_TIMEOUT_MS) != 0U)
        {
          EnterState(STATE_ERROR);
        }
        break;

      case STATE_LOCK_TARGET:
        Chassis_Stop();
        if (entered != 0U)
        {
          if (g_vision_distance == 0U)
          {
            EnterState(STATE_ERROR);
            break;
          }

          g_target_forward_mm = g_vision_distance;
          g_top_inside_ref = g_dist_top;
          g_close_reason = CLOSE_REASON_NONE;
          g_turn_dir = TURN_DIR_NONE;
          g_exit_confirmed = 0U;
          Robot_ClearFrontSwitchLatch();
          Robot_ResetForwardProgress();
        }
        EnterState(STATE_FORWARD_TO_TARGET);
        break;

      case STATE_FORWARD_TO_TARGET:
        Chassis_RunStraightPID(APPROACH_SPEED);
        if (g_front_switch_triggered != 0U)
        {
          Chassis_Stop();
          g_close_reason = CLOSE_REASON_FRONT_SWITCH;
          EnterState(STATE_GRAB_CLOSE);
        }
        else if (g_forward_progress_mm >= g_target_forward_mm)
        {
          Chassis_Stop();
          g_close_reason = CLOSE_REASON_DISTANCE;
          EnterState(STATE_GRAB_CLOSE);
        }
        else if (StateTimedOut(FORWARD_TIMEOUT_MS) != 0U)
        {
          EnterState(STATE_ERROR);
        }
        break;

      case STATE_GRAB_CLOSE:
        Chassis_Stop();
        if (entered != 0U)
        {
          g_servo_cmd = SERVO_CMD_CLOSE;
        }

        if ((g_servo_busy == 0U) && (g_servo_cmd == SERVO_CMD_NONE))
        {
          if (g_close_reason == CLOSE_REASON_FRONT_SWITCH)
          {
            EnterState(STATE_EXIT_STRAIGHT);
          }
          else
          {
            EnterState(STATE_DECIDE_TURN);
          }
        }
        else if (StateTimedOut(GRAB_TIMEOUT_MS) != 0U)
        {
          EnterState(STATE_ERROR);
        }
        break;

      case STATE_DECIDE_TURN:
        Chassis_Stop();
        if (g_dist_left > g_dist_right)
        {
          g_turn_dir = TURN_DIR_LEFT;
          EnterState(STATE_TURN_LEFT_180);
        }
        else if (g_dist_right > g_dist_left)
        {
          g_turn_dir = TURN_DIR_RIGHT;
          EnterState(STATE_TURN_RIGHT_180);
        }
        else if (DEFAULT_TURN_DIR == TURN_DIR_LEFT)
        {
          g_turn_dir = TURN_DIR_LEFT;
          EnterState(STATE_TURN_LEFT_180);
        }
        else
        {
          g_turn_dir = TURN_DIR_RIGHT;
          EnterState(STATE_TURN_RIGHT_180);
        }
        break;

      case STATE_TURN_LEFT_180:
        if (entered != 0U)
        {
          Chassis_StartTurnLeft180();
        }
        if (Chassis_RunTurnLeft180() != 0U)
        {
          EnterState(STATE_EXIT_STRAIGHT);
        }
        else if (StateTimedOut(TURN_TIMEOUT_MS) != 0U)
        {
          EnterState(STATE_ERROR);
        }
        break;

      case STATE_TURN_RIGHT_180:
        if (entered != 0U)
        {
          Chassis_StartTurnRight180();
        }
        if (Chassis_RunTurnRight180() != 0U)
        {
          EnterState(STATE_EXIT_STRAIGHT);
        }
        else if (StateTimedOut(TURN_TIMEOUT_MS) != 0U)
        {
          EnterState(STATE_ERROR);
        }
        break;

      case STATE_EXIT_STRAIGHT:
        if (g_close_reason == CLOSE_REASON_FRONT_SWITCH)
        {
          Chassis_RunStraightPID(BACKOUT_SPEED);
        }
        else
        {
          Chassis_RunStraightPID(EXIT_SPEED);
        }

        if (g_exit_confirmed != 0U)
        {
          Chassis_Stop();
          EnterState(STATE_RELEASE);
        }
        else if (StateTimedOut(EXIT_TIMEOUT_MS) != 0U)
        {
          EnterState(STATE_ERROR);
        }
        break;

      case STATE_RELEASE:
        Chassis_Stop();
        if (entered != 0U)
        {
          g_servo_cmd = SERVO_CMD_OPEN;
        }

        if ((g_servo_busy == 0U) && (g_servo_cmd == SERVO_CMD_NONE))
        {
          EnterState(STATE_DONE);
        }
        break;

      case STATE_DONE:
        Chassis_Stop();
        break;

      case STATE_ERROR:
      default:
        Chassis_Stop();
        break;
    }

    osDelay(10);
  }
  /* USER CODE END StartMotorTask */
}

   void StartEncoderTask(void const * argument)
  {
    /* USER CODE BEGIN StartEncoderTask */
    uint32_t last_print_tick = 0U;

    (void)argument;
    HAL_TIM_Encoder_Start(&htim2,
  TIM_CHANNEL_ALL);
    HAL_TIM_Encoder_Start(&htim3,
  TIM_CHANNEL_ALL);
    HAL_TIM_Encoder_Start(&htim4,
  TIM_CHANNEL_ALL);
    HAL_TIM_Encoder_Start(&htim5,
  TIM_CHANNEL_ALL);

    for (;;)
    {
      Robot_UpdateEncoderSnapshot();
      Robot_UpdateForwardProgress();

      if ((g_robot_state == STATE_SEARCH_STRAFE)
  &&
          ((HAL_GetTick() - last_print_tick) >=
  100U))
      {
        last_print_tick = HAL_GetTick();
        printf("[STRAFE] cmd=%d | e1=%d e2=%d e3=%d e4=%d\r\n",SEARCH_SPEED,
               g_enc1, g_enc2, g_enc3, g_enc4);
      }

      osDelay(10);
    }
    /* USER CODE END StartEncoderTask */
  }

void StartIMUTask(void const * argument)
{
  /* USER CODE BEGIN StartIMUTask */
  (void)argument;
  
#if ROBOT_SM_TEST_IMU
    /* ===== 状态机逻辑测试桩 BEGIN ===== */
    for (;;)
    {
      osDelay(100);
    }
    /* ===== 状态机逻辑测试桩 END ===== */
#else
	HWT101_Init();
    osDelay(50);
  
	for (;;)
  {
    HWT101_GetValue();
    osDelay(10);
  }
#endif
  
  /* USER CODE END StartIMUTask */
}


void StartSensorTask(void const * argument)
{
  /* USER CODE BEGIN StartSensorTask */
  
  (void)argument;

#if  ROBOT_SM_TEST_SENSOR
    /* ===== 状态机逻辑测试桩 BEGIN ===== */
    for (;;)
    {
      osDelay(100);
    }
    /* ===== 状态机逻辑测试桩 END ===== */
#else
	uint8_t top_exit_count;
	uint8_t vl53_ready;
  
	top_exit_count = 0U;
    osDelay(100);
    vl53_ready = (VL53L0X_Init_All() == 0U) ? 1U : 0U;

  for (;;)
  {
    if (vl53_ready != 0U)
    {
      g_dist_left = VL53L0X_ReadDistance(0);
      g_dist_top = VL53L0X_ReadDistance(1);
      g_dist_right = VL53L0X_ReadDistance(2);
    }
    else
    {
      g_dist_left = 9999U;
      g_dist_top = 9999U;
      g_dist_right = 9999U;
    }

    g_dist_front = g_vision_distance;
    g_front_switch_triggered = ((g_bumper_left != 0U) || (g_bumper_right != 0U)) ? 1U : 0U;
    g_grab_switch_front = g_front_switch_triggered;

    if (g_robot_state == STATE_EXIT_STRAIGHT)
    {
      if ((g_top_inside_ref > 0U) && (g_dist_top > (uint16_t)(g_top_inside_ref + TOP_EXIT_DELTA_MM)))
      {
        if (top_exit_count < 255U)
        {
          top_exit_count++;
        }
      }
      else
      {
        top_exit_count = 0U;
      }

      g_exit_confirmed = (top_exit_count >= TOP_EXIT_COUNT_TH) ? 1U : 0U;
    }
    else
    {
      top_exit_count = 0U;
      g_exit_confirmed = 0U;
    }

    osDelay(20);
  }

#endif
  
  /* USER CODE END StartSensorTask */
}

  
void StartServoTask(void const * argument)
{
  /* USER CODE BEGIN StartServoTask */
  (void)argument;
 
#if  ROBOT_SM_TEST_SERVO
    /* ===== 状态机逻辑测试桩 BEGIN ===== */
    for (;;)
    {
      osDelay(100);
    }
    /* ===== 状态机逻辑测试桩 END ===== */
#else
 
  TIM8_SwitchToServo();
  HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_4);
  osDelay(100);

  for (;;)
  {
    if (g_servo_cmd != SERVO_CMD_NONE)
    {
	  printf("[SERVO] cmd=%d start\r\n",
  g_servo_cmd);
      g_servo_busy = 1U;

      TIM8_SwitchToServo();
      HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_4);
      osDelay(50);

      if (g_servo_cmd == SERVO_CMD_OPEN)
      {
        Servo_Open(1);
      }
      else if (g_servo_cmd == SERVO_CMD_CLOSE)
      {
        Servo_Close(1);
      }

      osDelay(SERVO_MOVE_TIME_MS);
 printf("[SERVO] cmd=%d done\r\n",
  g_servo_cmd);
      g_servo_cmd = SERVO_CMD_NONE;
      g_servo_busy = 0U;
    }

    osDelay(20);
  }
  
 #endif
  /* USER CODE END StartServoTask */
}

 
void StartLedTask(void const * argument)
{
  /* USER CODE BEGIN StartLedTask */
  uint8_t last_light_on;
  uint8_t light_on;

  (void)argument;
  last_light_on = 2U;

  TIM8_SwitchToWs2812();
  WS2812_Init();
  WS2812_Clear();
  osDelay(100);

  for (;;)
  {
    light_on = RobotNeedLight();

    if ((g_servo_busy == 0U) && (light_on != last_light_on))
    {
      TIM8_SwitchToWs2812();

      if (light_on != 0U)
      {
        WS2812_SetAll(80, 80, 80);
      }
      else
      {
        WS2812_Clear();
      }

      last_light_on = light_on;
      osDelay(50);
    }

    osDelay(50);
  }
  /* USER CODE END StartLedTask */
}


 void StartCommTask(void const * argument)
  {
    /* USER CODE BEGIN StartCommTask */
    (void)argument;
    Robot_VisionRxStart();

    for (;;)
    {
  #if  ROBOT_SM_TEST_COMM
      Robot_StateMachineTestStep();
  #else
      Robot_VisionProtocol_Poll();
  #endif
      osDelay(10);
    }
    /* USER CODE END StartCommTask */
  }

 

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
static uint8_t VisionDetectedStable(void)
{
  static uint8_t detect_count = 0;

  if (g_robot_state != STATE_SEARCH_STRAFE)
  {
    detect_count = 0;
    return 0U;
  }

  if ((g_vision_detected != 0U) && (g_vision_distance > 0U))
  {
    if (detect_count < 255U)
    {
      detect_count++;
    }
  }
  else
  {
    detect_count = 0;
  }

  return (detect_count >= VISION_STABLE_COUNT) ? 1U : 0U;
}

static uint8_t StateTimedOut(uint32_t timeout_ms)
{
  return ((HAL_GetTick() - g_state_timer) > timeout_ms) ? 1U : 0U;
}

static uint8_t RobotNeedLight(void)
{
  if ((g_robot_state == STATE_BOOT_PREPARE) ||
      (g_robot_state == STATE_SEARCH_STRAFE) ||
      (g_robot_state == STATE_LOCK_TARGET) ||
      (g_robot_state == STATE_FORWARD_TO_TARGET) ||
      (g_robot_state == STATE_GRAB_CLOSE) ||
      (g_robot_state == STATE_DECIDE_TURN) ||
      (g_robot_state == STATE_TURN_LEFT_180) ||
      (g_robot_state == STATE_TURN_RIGHT_180) ||
      (g_robot_state == STATE_EXIT_STRAIGHT) ||
      (g_robot_state == STATE_RELEASE))
  {
    return 1U;
  }

  return 0U;
}

static uint32_t Robot_AbsS32(int32_t value)
{
  return (value >= 0) ? (uint32_t)value : (uint32_t)(-value);
}

static void Robot_ClearFrontSwitchLatch(void)
{
  g_bumper_left = 0U;
  g_bumper_right = 0U;
  g_front_switch_triggered = 0U;
  g_grab_switch_front = 0U;
}

static void Robot_ResetForwardProgress(void)
{
  s_forward_encoder_accum = 0U;
  g_forward_progress_mm = 0U;
}

static void Robot_UpdateEncoderSnapshot(void)
{
  g_enc1 = -Read_Encoder_TIM2();
  g_enc2 = Read_Encoder_TIM3();
  g_enc3 = -Read_Encoder_TIM5();
  g_enc4 = -Read_Encoder_TIM4();
}

static void Robot_UpdateForwardProgress(void)
{
  uint32_t avg_abs_count;

  if (g_robot_state != STATE_FORWARD_TO_TARGET)
  {
    return;
  }

  avg_abs_count = (Robot_AbsS32(g_enc1) +
                   Robot_AbsS32(g_enc2) +
                   Robot_AbsS32(g_enc3) +
                   Robot_AbsS32(g_enc4)) / 4U;

  s_forward_encoder_accum += avg_abs_count;
  g_forward_progress_mm = (uint16_t)((float)s_forward_encoder_accum * ENCODER_MM_PER_COUNT);
}

static void Robot_VisionRxStart(void)
{
  s_vision_rx_ready = 0U;
  s_vision_rx_size = 0U;
  HAL_UARTEx_ReceiveToIdle_IT(&huart3, s_vision_rx_buf, VISION_RX_BUF_SIZE);
}

static void EnterState(RobotState_t next_state)
  {
    g_robot_state = next_state;
    g_state_timer = HAL_GetTick();

  #if ROBOT_SM_DEBUG_PRINT
    printf("[STATE] %s | vision=%u dist=%u forward=%u close=%d turn=%d exit=%u\r\n",
           Robot_StateName(next_state),
           g_vision_detected,
           g_vision_distance,
           g_forward_progress_mm,
           g_close_reason,
           g_turn_dir,
           g_exit_confirmed);
  #endif
  }
  
  void Robot_VisionProtocol_Poll(void)
  {
    char line[VISION_RX_BUF_SIZE];
    char *token;
    char *ctx = NULL;
    uint16_t copy_len;
    uint8_t clutter_flag = 0U;

    if (s_vision_rx_ready == 0U)
    {
      return;
    }

    s_vision_rx_ready = 0U;

    copy_len = s_vision_rx_size;
    if ((copy_len == 0U) || (copy_len >= VISION_RX_BUF_SIZE))
    {
      Robot_VisionRxStart();
      return;
    }

    memcpy(line, s_vision_rx_buf, copy_len);
    line[copy_len] = '\0';
    line[strcspn(line, "\r\n")] = '\0';

    if (line[0] == '\0')
    {
      Robot_VisionRxStart();
      return;
    }

    token = strtok_r(line, ",", &ctx);
    if ((token == NULL) || (strcmp(token, "$SWEEP") != 0))
    {
      Robot_VisionRxStart();
      return;
    }

    token = strtok_r(NULL, ",", &ctx);
    if (token == NULL)
    {
      Robot_VisionRxStart();
      return;
    }

    clutter_flag = (uint8_t)((atoi(token) != 0) ? 1U : 0U);

    if (clutter_flag != 0U)
    {
      g_vision_detected = 1U;
      g_vision_position = 1U;
      g_vision_distance = 1000U;
    }
    else
    {
      g_vision_detected = 0U;
      g_vision_distance = 0U;
      g_vision_position = 0U;
    }

    Robot_VisionRxStart();
  }
//__weak void Robot_VisionProtocol_Poll(void)
//{
//  uint16_t rx_size;
//  char line[VISION_RX_BUF_SIZE + 1U];
//  char prefix[8];
//  char state[16];
//  int clutter_flag;
//  int smooth_score;
//  int raw_score;
//  int decision_score;
//  int matched;

//  if (s_vision_rx_ready == 0U)
//  {
//    return;
//  }

//  rx_size = s_vision_rx_size;
//  if (rx_size > VISION_RX_BUF_SIZE)
//  {
//    rx_size = VISION_RX_BUF_SIZE;
//  }

//  memcpy(line, s_vision_rx_buf, rx_size);
//  line[rx_size] = '\0';

//  s_vision_rx_ready = 0U;
//  Robot_VisionRxStart();

//  matched = sscanf(line,
//                   "%7[^,],%d,%d,%d,%d,%15s",
//                   prefix,
//                   &clutter_flag,
//                   &smooth_score,
//                   &raw_score,
//                   &decision_score,
//                   state);
//  if (matched < 2)
//  {
//    return;
//  }

//  if (strcmp(prefix, "$SWEEP") != 0)
//  {
//    return;
//  }

//  g_vision_detected = (clutter_flag != 0) ? 1U : 0U;
//  g_vision_position = g_vision_detected;

//  if (g_vision_detected == 0U)
//  {
//    g_vision_distance = 0U;
//  }
//}
// 


#if ROBOT_SM_TEST_STUB
  /* ===== 状态机逻辑测试桩 BEGIN ===== */
   static void Robot_StateMachineTestStep(void)
  {
    static RobotState_t last_state = STATE_ERROR;
    static uint32_t state_tick = 0U;

    if (g_robot_state != last_state)
    {
      last_state = g_robot_state;
      state_tick = HAL_GetTick();
    }

    switch (g_robot_state)
    {
      case STATE_BOOT_PREPARE:
        g_vision_detected = 0U;
        g_vision_distance = 0U;
        g_forward_progress_mm = 0U;
        g_exit_confirmed = 0U;
        g_servo_busy = 0U;
        g_servo_cmd = SERVO_CMD_NONE;
        break;

      case STATE_SEARCH_STRAFE:
        /* 连续给有效视觉，满足VisionDetectedStable() */
        g_vision_detected = 1U;
        g_vision_distance = 450U;
        break;

      case STATE_LOCK_TARGET:
        g_vision_detected = 1U;
        g_vision_distance = 1000U;
        break;

      case STATE_FORWARD_TO_TARGET:
        /* 模拟前进累计距离 */
        if (g_forward_progress_mm < 400U)
        {
          g_forward_progress_mm += 20U;
        }
        break;

      case STATE_GRAB_CLOSE:
        /* 模拟夹爪动作开始后再完成 */
        if ((HAL_GetTick() - state_tick) < 50U)
        {
          g_servo_busy = 1U;
        }
        else if ((HAL_GetTick() - state_tick) >
  300U)
        {
          g_servo_busy = 0U;
          g_servo_cmd = SERVO_CMD_NONE;
        }
        break;

      case STATE_DECIDE_TURN:
        /* 强制左转分支 */
        g_dist_left = 500U;
        g_dist_right = 200U;
        break;

     case STATE_TURN_LEFT_180:
    if ((HAL_GetTick() - state_tick) > 300U)
    {
      EnterState(STATE_EXIT_STRAIGHT);
    }
    break;

  case STATE_TURN_RIGHT_180:
    if ((HAL_GetTick() - state_tick) > 300U)
    {
      EnterState(STATE_EXIT_STRAIGHT);
    }
    break;

      case STATE_EXIT_STRAIGHT:
        if ((HAL_GetTick() - state_tick) > 300U)
        {
          g_exit_confirmed = 1U;
        }
        break;

      case STATE_RELEASE:
        if ((HAL_GetTick() - state_tick) < 50U)
        {
          g_servo_busy = 1U;
        }
        else if ((HAL_GetTick() - state_tick) >
  300U)
        {
          g_servo_busy = 0U;
          g_servo_cmd = SERVO_CMD_NONE;
        }
        break;

      case STATE_DONE:
      case STATE_ERROR:
      default:
        break;
    }
  }
  
  /* ===== 状态机逻辑测试桩 END ===== */
  #endif
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if (huart->Instance == USART3)
  {
    if (Size > VISION_RX_BUF_SIZE)
    {
      Size = VISION_RX_BUF_SIZE;
    }
    s_vision_rx_size = Size;
    s_vision_rx_ready = 1U;
  }
}
static const char *Robot_StateName(RobotState_t state)
{
  switch (state)
  {
    case STATE_BOOT_PREPARE:      return "BOOT_PREPARE";
    case STATE_SEARCH_STRAFE:     return "SEARCH_STRAFE";
    case STATE_LOCK_TARGET:       return "LOCK_TARGET";
    case STATE_FORWARD_TO_TARGET: return "FORWARD_TO_TARGET";
    case STATE_GRAB_CLOSE:        return "GRAB_CLOSE";
    case STATE_DECIDE_TURN:       return "DECIDE_TURN";
    case STATE_TURN_LEFT_180:     return "TURN_LEFT_180";
    case STATE_TURN_RIGHT_180:    return "TURN_RIGHT_180";
    case STATE_EXIT_STRAIGHT:     return "EXIT_STRAIGHT";
    case STATE_RELEASE:           return "RELEASE";
    case STATE_DONE:              return "DONE";
    case STATE_ERROR:             return "ERROR";
    default:                      return "UNKNOWN";
  }
}

/* USER CODE END Application */
 
