#include "motor.h"
extern TIM_HandleTypeDef htim1;
	
//	#define M1_DIR  1   // pwm1 ��ǰ
//    #define M2_DIR  1   // pwm2 ��ǰ
//    #define M3_DIR  -1   // pwm3 ���
//    #define M4_DIR  -1   // pwm4 �Һ�
/* PWM���ֵ����ӦTIM1��ARR=99 */
#define MOTOR_PWM_MAX   99
#define MOTOR_FORWARD_LEFT_TRIM   0
#define MOTOR_FORWARD_RIGHT_TRIM  0.3

/**
 * @brief �޷���������ֹPWM������Χ
 */
static int16_t Motor_Clamp(int16_t val, int16_t min, int16_t max)
{
    if(val > max) return max;
    if(val < min) return min;
    return val;
}

/**
 * @brief ���Ƶ������������ٶ�
 */
static void Motor_Channel(GPIO_TypeDef *IN1_Port, uint16_t IN1_Pin,
                          GPIO_TypeDef *IN2_Port, uint16_t IN2_Pin,
                          uint32_t channel,
                          int16_t pwm)
{
    pwm = Motor_Clamp(pwm, -MOTOR_PWM_MAX, MOTOR_PWM_MAX);
    if(pwm == 0)
    {
        HAL_GPIO_WritePin(IN1_Port, IN1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(IN2_Port, IN2_Pin, GPIO_PIN_RESET);
    }
    else if(pwm > 0)
    {
        HAL_GPIO_WritePin(IN1_Port, IN1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(IN2_Port, IN2_Pin, GPIO_PIN_RESET);
    }
    else
    {
        HAL_GPIO_WritePin(IN1_Port, IN1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(IN2_Port, IN2_Pin, GPIO_PIN_SET);
        pwm = -pwm;
    }
    __HAL_TIM_SET_COMPARE(&htim1, channel, (uint32_t)pwm);
}

/**
 * @brief ��ʼ�����������TIM1��·PWM
 */
void Motor_Init(void)
{
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
    Motor_Stop();
}

/**
 * @brief �����ĸ����PWM
 * @param pwm1~pwm4: ��Χ-99~+99��������ת��������ת��0ֹͣ
 * �����Ӧ��ϵ��
 * pwm1 = ��ǰ�֣�TIM_CHANNEL_1��
 * pwm2 = ��ǰ�֣�TIM_CHANNEL_2��
 * pwm3 = ����֣�TIM_CHANNEL_3��
 * pwm4 = �Һ��֣�TIM_CHANNEL_4��
 */
 void Motor_SetPWM(int16_t pwm1, int16_t pwm2,
                    int16_t pwm3, int16_t pwm4)
  {
      Motor_Channel(AIN1_GPIO_Port, AIN1_Pin,
                    AIN2_GPIO_Port, AIN2_Pin,
                    TIM_CHANNEL_1, pwm1);

      Motor_Channel(BIN1_GPIO_Port, BIN1_Pin,
                    BIN2_GPIO_Port, BIN2_Pin,
                    TIM_CHANNEL_2, pwm2);

      Motor_Channel(CIN1_GPIO_Port, CIN1_Pin,
                    CIN2_GPIO_Port, CIN2_Pin,
                    TIM_CHANNEL_3, pwm3);

      Motor_Channel(DIN1_GPIO_Port, DIN1_Pin,
                    DIN2_GPIO_Port, DIN2_Pin,
                    TIM_CHANNEL_4, pwm4);
  }

/**
 * @brief ֹͣ���е��
 */
void Motor_Stop(void)
{
    Motor_SetPWM(0, 0, 0, 0);
}

/**
 * @brief ǰ��
 */
void Motor_Forward(int16_t speed)
{
    Motor_SetPWM(speed + MOTOR_FORWARD_LEFT_TRIM,
                 speed + MOTOR_FORWARD_RIGHT_TRIM,
                 speed + MOTOR_FORWARD_LEFT_TRIM,
                 speed + MOTOR_FORWARD_RIGHT_TRIM);
}

/**
 * @brief ����
 */
void Motor_Backward(int16_t speed)
{
    Motor_SetPWM(-speed, -speed, -speed, -speed);
}

/**
 * @brief ԭ����ת
 * �������ת���Ҳ�����ת
 */
void Motor_TurnLeft(int16_t speed)
{
    Motor_SetPWM(-speed, speed, -speed, speed);
}

/**
 * @brief ԭ����ת
 * �������ת���Ҳ�����ת
 */
void Motor_TurnRight(int16_t speed)
{
    Motor_SetPWM(speed, -speed, speed, -speed);
}

/**
 * @brief ����ƽ�ƣ������ķ�֣�
 * ��ǰ+ ��ǰ- ���- �Һ�+
 *  ʵ����������ˣ������������ŶԵ�
 */
void Motor_MoveRight(int16_t speed)
{
    Motor_SetPWM(speed, -speed, -speed, speed);
}

/**
 * @brief ����ƽ�ƣ������ķ�֣�
 * ��ǰ- ��ǰ+ ���+ �Һ�-
 *  ʵ����������ˣ������������ŶԵ�
 */
void Motor_MoveLeft(int16_t speed)
{
    Motor_SetPWM(-speed, speed, speed, -speed);
}