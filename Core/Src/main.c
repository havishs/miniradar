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
#include <stdio.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */

void USART1_Init(void) // bluetooth
{
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;


    GPIOA->MODER &= ~(GPIO_MODER_MODE9 | GPIO_MODER_MODE10);
    GPIOA->MODER |= (GPIO_MODER_MODE9_1 | GPIO_MODER_MODE10_1);
    GPIOA->AFR[1] &= ~(GPIO_AFRH_AFSEL9 | GPIO_AFRH_AFSEL10);
    GPIOA->AFR[1] |= (7U << GPIO_AFRH_AFSEL9_Pos) | (7U << GPIO_AFRH_AFSEL10_Pos);

    USART1->CR1 &= ~USART_CR1_UE;
    USART1->BRR = 8333;
    USART1->CR1 = USART_CR1_TE;
    USART1->CR1 |= USART_CR1_UE;
}

void Init()
{
    // Enable GPIO Clock
    RCC->AHB2ENR |= (RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOCEN);

    // Initialize Green LED
    GPIOA->MODER &= ~GPIO_MODER_MODE5; // RESETS BOTH TO 0 (output)
    GPIOA->MODER |= GPIO_MODER_MODE5_0; // SETS BIT TO 1 (output)
    GPIOA->OTYPER &= ~GPIO_OTYPER_OT5; // SETS BIT TO 0 (push pull)
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD5; // SETS TO NO PUPD (00)

    // Initialize User Button
    GPIOC->MODER &= ~GPIO_MODER_MODE13; // Sets as input (00)
    GPIOC->PUPDR &= ~GPIO_PUPDR_PUPD13; // Sets no pull (00)
}

void USART2_Init(void)
{
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;

    USART2->CR1 &= ~USART_CR1_UE;

    // baud = 80,000,000 / 115,200 = 694
    USART2->BRR = 694;

    USART2->CR1 = USART_CR1_TE;

    USART2->CR1 |= USART_CR1_UE;
}

void USART2_SendChar(char c)
{
    while (!(USART2->ISR & USART_ISR_TXE));  // wait until TDR is empty
    USART2->TDR = c;
}

void USART2_SendString(const char *s)
{
    while (*s)
        USART2_SendChar(*s++);
}

void TIM3_PWM_Init(void)
{
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM3EN;

    // pa6 alternate function 2
    GPIOA->MODER &= ~GPIO_MODER_MODE6;
    GPIOA->MODER |= GPIO_MODER_MODE6_1;
    GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL6;
    GPIOA->AFR[0] |= (2U << GPIO_AFRL_AFSEL6_Pos);

    // 80Mhz clock so with prescaler it becomes 1 Mhz. We have a period of 20,000 us.
    TIM3->PSC = 79;
    TIM3->ARR = 19999;

    TIM3->CCMR1 &= ~TIM_CCMR1_OC1M;
    TIM3->CCMR1 |= (TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2);
    TIM3->CCMR1 |= TIM_CCMR1_OC1PE;
    TIM3->CCER  |= TIM_CCER_CC1E;

    TIM3->CCR1 = 1500;
    TIM3->EGR  |= TIM_EGR_UG;
    TIM3->CR1  |= TIM_CR1_CEN;
}

void Radar_Init(void)
{
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOBEN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM4EN;

    GPIOB->MODER &= ~GPIO_MODER_MODE5;
    GPIOB->MODER |= GPIO_MODER_MODE5_0;
    GPIOB->BSRR   = GPIO_BSRR_BR5;

    GPIOB->MODER &= ~GPIO_MODER_MODE6;
    GPIOB->MODER |= GPIO_MODER_MODE6_1;
    GPIOB->AFR[0] &= ~GPIO_AFRL_AFSEL6;
    GPIOB->AFR[0] |= (2U << GPIO_AFRL_AFSEL6_Pos);

    TIM4->PSC = 79;
    TIM4->ARR = 0xFFFF;

    TIM4->CCMR1 = TIM_CCMR1_CC1S_0
                | TIM_CCMR1_CC2S_1;
    TIM4->CCER  = TIM_CCER_CC2P
                | TIM_CCER_CC1E
                | TIM_CCER_CC2E;

    TIM4->EGR |= TIM_EGR_UG;
    TIM4->CR1 |= TIM_CR1_CEN;
}

static void delay_us(uint16_t us)
{
    uint16_t t0 = TIM4->CNT;
    while ((uint16_t)(TIM4->CNT - t0) < us);
}

uint32_t Measure_Echo_us(void)
{
    //clear flag
    TIM4->SR = 0;

    GPIOB->BSRR = GPIO_BSRR_BS5;
    delay_us(10);
    GPIOB->BSRR = GPIO_BSRR_BR5;

    uint16_t t0 = TIM4->CNT;
    while (!(TIM4->SR & TIM_SR_CC2IF))
        if ((uint16_t)(TIM4->CNT - t0) > 40000) return 0;  // nothing is in detectable distance

    return (uint16_t)(TIM4->CCR2 - TIM4->CCR1);
}
uint8_t crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc = crc << 1;
        }
    }
    return crc;
}

static void send_bytes(USART_TypeDef *port, const uint8_t *p, int n)
{
    for (int i = 0; i < n; i++)
    {
        while (!(port->ISR & USART_ISR_TXE));
        port->TDR = p[i];
    }
}

static uint8_t seq = 0;

void send_packet(uint8_t angle, uint16_t dist_mm)
{
    uint8_t p[8];

    p[0] = 0xAA;
    p[1] = 0x55;
    p[2] = seq++;
    p[3] = angle;
    p[4] = dist_mm & 0xFF;
    p[5] = dist_mm >> 8;
    p[6] = crc8(&p[2], 4);
    p[7] = 0;

    send_bytes(USART2, p, 8);
    send_bytes(USART1, p, 8);
}
static uint32_t median3(uint32_t a, uint32_t b, uint32_t c)
{
    uint32_t t;
    if (a > b) { t = a; a = b; b = t; }
    if (b > c) { t = b; b = c; c = t; }
    if (a > b) { t = a; a = b; b = t; }
    return b;  // we want the middle value
}

#define SERVO_MIN   700
#define SERVO_MAX   2300
#define SERVO_STEP  100

static void scan_at(int ccr)
{
    TIM3->CCR1 = ccr;
    HAL_Delay(100);

    uint32_t p1 = Measure_Echo_us();
    HAL_Delay(50);
    uint32_t p2 = Measure_Echo_us();
    HAL_Delay(50);
    uint32_t p3 = Measure_Echo_us();

    uint32_t us = median3(p1, p2, p3);
    uint32_t mm = us * 343 / 2000;
    int angle = (ccr - SERVO_MIN) * 180 / (SERVO_MAX - SERVO_MIN);

    send_packet((uint8_t)angle, (uint16_t)mm);

    /*
    snprintf(buf, sizeof(buf), "angle=%d  mm=%lu\r\n", angle, (unsigned long)mm);
    USART2_SendString(buf);
    */
}

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
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Initialize leds */
  BSP_LED_Init(LED_GREEN);

  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  Init();
  USART2_Init();
  USART1_Init();
  TIM3_PWM_Init();
  Radar_Init();

  TIM3->CCR1 = SERVO_MIN;
  HAL_Delay(500);




  while (1)
  {
      for (int ccr = SERVO_MIN; ccr <= SERVO_MAX; ccr += SERVO_STEP)
          scan_at(ccr);
      for (int ccr = SERVO_MAX; ccr >= SERVO_MIN; ccr -= SERVO_STEP)
          scan_at(ccr);
  }




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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pins : USART_TX_Pin USART_RX_Pin */
  GPIO_InitStruct.Pin = USART_TX_Pin|USART_RX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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
