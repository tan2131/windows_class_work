/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "adc.h"
#include "i2c.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include "motor.h"
#include "key.h"
#include "My_rtc.h"
#include "Light_senser.h"
#include "HC06.h"
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

extern uint8_t Alarm_Triggered; // 告诉编译器这个变量在别处定义�?
extern volatile int32_t  g_motor_abs_pos;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
 
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
  MX_I2C1_Init();
  MX_RTC_Init();
  MX_USART1_UART_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
	OLED_Init();
	OLED_Clear();
	
	My_RTC_Init();
	HC06_Init(); // 启动蓝牙串口中断
	
	
//	OLED_ShowString(0,0,(uint8_t*)"Init-ok",16);
	uint32_t Last_ADC_Time = 0;
	uint16_t Light_Data = 0;
	
	uint32_t last_adc_tick = 0;
	uint32_t last_motor_tick = 0;
	uint8_t  current_dir = 0; // 0:�?, 1:�?, 2:�?
	
	uint8_t run=0;
		// 1. 假设刚上电时是在 0 �?
g_motor_abs_pos = 0;
	
	HAL_TIM_Base_Start_IT(&htim1);
	
	
        char aa[]="\r\n-----蓝牙连接成功-----\r\n";
				HAL_UART_Transmit(&huart1,(uint8_t*)aa,sizeof(aa),100);
				
//	/*用于蓝牙的开�?*/
//	HAL_UART_Receive_IT(&huart1, &rx_data, 1); // 启动串口1接收中断
 

				
				
HAL_UART_Transmit(&huart1, (uint8_t*)"init\r\n", 6, 100);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {        

//任务4：蓝牙控制	
	HC06_Control_Logic(); // 处理蓝牙指令
		
// printf("init/n");
		//		OLED_ShowString(0,2,(uint8_t*)"while-begin",11);
				OLED_ShowString(0, 4, (uint8_t*)"Mode:", 16);
		OLED_ShowString(0, 0, (uint8_t*)"Pos:    (0-500)", 16);
		OLED_ShowString(0, 2, (uint8_t*)"Light:", 16);

    OLED_ShowNum(8*4, 0, g_motor_abs_pos/4, 3, 16); // 显示当前步数
		
    // 传感器触发示�?
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {
        Motor_L(500, 4); // 尝试�?回走 500 步，逻辑内部会自动判断当前位�?
    }
		
// 任务 3：手动控�?
		/*！问题！ �?  ！当电机转动的时候，�?有控制都失效了，暂停了，在等待电机完成！
		         是优先级问题吗？好像是TIM功能问题？好像是Delay问题�?
		          �?  �?                                =  =            */
		/*          ^      ！目前没有暂停功能！     ￥愁si我了�?           */
		Key_Process();
		
		
		
// 任务 2	:时间控制
		RTC_TimeTypeDef gTime;
    RTC_DateTypeDef gDate;
		
		// 必须先读�? Time，再读取 Date，否则时钟会锁死不更�?
    HAL_RTC_GetTime(&hrtc, &gTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &gDate, RTC_FORMAT_BIN);
		// OLED显示逻辑示例 
     char buf[20];
		sprintf(buf, "time:%02d:%02d:%02d", gTime.Hours, gTime.Minutes, gTime.Seconds);
     OLED_ShowString(0, 6,(uint8_t*) buf,16); 
    HAL_Delay(500); // 刷新频率	
		
		// �?查是否有闹钟触发
    if (Alarm_Triggered != 0) {
        if (Alarm_Triggered == 1) { // 7点动�?
//            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET); // 关灯
            Motor_L(500, 4); //关窗�?    close
					OLED_ShowString(0, 4, (uint8_t*)"Mode:close", 16);
            HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_RESET);
            HAL_Delay(100); // 这里�? Delay 不会卡死
            HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_SET);
Alarm_Triggered = 0;    // 清除标志
        } 
        else if (Alarm_Triggered == 2) { // 22点动�?
//            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET); // �?�?
					OLED_ShowString(0, 4, (uint8_t*)"Mode:open ", 16);       
					Motor_R(500, 4); //�?窗�??    open

            HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_RESET);
            HAL_Delay(100);
            HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_SET);
Alarm_Triggered = 0;    // 清除标志
        }
        
        Alarm_Triggered = 0; // 【极其重要�?�处理完后清空标志位
    }
		
		
		

// 任务 1: ADC 采样与显�? (10ms)	
		uint32_t now = HAL_GetTick();
if (now - last_adc_tick >= 15) {
    last_adc_tick = now;
    HAL_ADC_Start(&hadc1);
    
    if (HAL_ADC_PollForConversion(&hadc1, 1) == HAL_OK) {
        uint16_t val = HAL_ADC_GetValue(&hadc1);
			
    if (val > 4095) val = 4095; //亮度换算逻辑公式
    uint8_t brightness = (uint8_t)((4095 - val) * 100 / 4095);

    char str[16];
    sprintf(str, "Light:%3d%%", brightness); 
    OLED_ShowString(0, 2, (uint8_t*)str, 16);

        // --- 逻辑判断 1：强光关�? ---
					if (val < 1800 && run != 2){     // 只要当前不是“关窗状态�?�，就允许触�?
            run = 2; // 立即改变状�?�，防止重复触发
            OLED_ShowString(0, 4, (uint8_t*)"Mode:close(    )", 16);
            OLED_ShowCHinese(8*11, 4, 0); // �?
            OLED_ShowCHinese(8*13, 4, 2); // �?
            
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
            
            // 【关键�?�只启动电机，不要在后面�? Motor_Stop()！中断会自动停止�?
            Motor_L(500, 4); 

            // 提示音：非阻塞模式下，蜂鸣器可以保留
            HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_RESET);
            HAL_Delay(100); 
            HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_SET);
        }   

        // --- 逻辑判断 2：弱光开�? ---
//        else if (val > 2500 && (run == 0 || run == 2)) 
					else if (val > 1800 && run != 1){ // 只要当前不是“开窗状态�?�，就允许触�?
            run = 1; // 切换回状�? 1
            OLED_ShowString(0, 4, (uint8_t*)"Mode:open (    )", 16);
            OLED_ShowCHinese(8*11, 4, 1); // �?
            OLED_ShowCHinese(8*13, 4, 2); // �?
            
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
            
            // 【关键�?�只启动电机
            Motor_R(500, 4);

            HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_RESET);
            HAL_Delay(100); 
            HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_SET);
        }
    }
    HAL_ADC_Stop(&hadc1);
}

// 自动关闭蜂鸣器�?�辑�?
    // 如果发现蜂鸣器是响着的，就等待一小会儿关掉它
    if (HAL_GPIO_ReadPin(Buzzer_GPIO_Port, Buzzer_Pin) == GPIO_PIN_RESET) {
        HAL_Delay(50); // 这里�? 100ms 只会在到达终点时执行�?次，不影响平时运�?
        HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_SET);
    }


/*               （！光感到强光或弱光后电机一直转，不停！�?
		设置�?个标志位，可以解决遇到的问题
		标志位初始�?�为0，比如：遇到强光后标志位设置�?1，电机转动完成后标志位为2
		遇到弱光后，如果标志位为2，开始转动，转完后，标志位回�?1
标志位要判断初始值和转换值！�?   添加与或逻辑，可以随时转换一次不循环，没有限制条�?
*/

		
    
    
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_ADC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
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

#ifdef  USE_FULL_ASSERT
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
