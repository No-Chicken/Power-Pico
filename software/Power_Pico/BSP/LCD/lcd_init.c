#include "lcd_init.h"
#include "spi.h"
#include "tim.h"

uint8_t LCD_ROTATION = 0; // 0, 90, 180, 270
uint8_t OFFSET_X = 0;
uint8_t OFFSET_Y = 0;

/******************************************************************************
      函数说明：LCD端口初始化
      入口数据：无
      返回值：  无
******************************************************************************/
void LCD_GPIO_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure = {0};

	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	GPIO_InitStructure.Pin = RES_PIN;
 	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP; 		 //推挽输出
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;//速度50MHz
 	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);	  //初始化GPIOB
	HAL_GPIO_WritePin(GPIOB, RES_PIN, GPIO_PIN_SET);

	GPIO_InitStructure.Pin = CS_PIN|DC_PIN;
 	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP; 		 //推挽输出
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;//速度50MHz
 	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);	  //初始化GPIOB
	HAL_GPIO_WritePin(GPIOA, CS_PIN|DC_PIN, GPIO_PIN_SET);
}


/******************************************************************************
      函数说明：LCD串行数据写入函数(software SPI)
      入口数据：dat  要写入的串行数据
      返回值：  无
******************************************************************************/
void LCD_Writ_Bus(u8 dat)
{
	//hard SPI
	HAL_SPI_Transmit(&hspi2,&dat,1,1);

	//soft SPI
	/*
	u8 i;
	for(i=0;i<8;i++)
	{
		LCD_SCLK_Clr();
		if(dat&0x80)
		{
		   LCD_MOSI_Set();
		}
		else
		{
		   LCD_MOSI_Clr();
		}
		LCD_SCLK_Set();
		dat<<=1;
	}
	*/
}


/******************************************************************************
      函数说明：LCD写入数据
      入口数据：dat 写入的数据
      返回值：  无
******************************************************************************/
void LCD_WR_DATA8(u8 dat)
{
	LCD_Writ_Bus(dat);
}


/******************************************************************************
      函数说明：LCD写入数据
      入口数据：dat 写入的数据
      返回值：  无
******************************************************************************/
void LCD_WR_DATA(u16 dat)
{
//	LCD_Writ_Bus(dat>>8);
//	LCD_Writ_Bus(dat);
	uint8_t temp[2];
	temp[0]=(dat>>8)&0xff;
	temp[1]=dat&0xff;
	HAL_SPI_Transmit(&hspi2,temp,2,1);

}


/******************************************************************************
      函数说明：LCD写入命令
      入口数据：dat 写入的命令
      返回值：  无
******************************************************************************/
void LCD_WR_REG(u8 dat)
{
	LCD_DC_Clr();//写命令
	LCD_Writ_Bus(dat);
	LCD_DC_Set();//写数据
}


/******************************************************************************
      函数说明：设置起始和结束地址
      入口数据：x1,x2 设置列的起始和结束地址
                y1,y2 设置行的起始和结束地址
      返回值：  无
******************************************************************************/
void LCD_Address_Set(u16 x1,u16 y1,u16 x2,u16 y2)
{
    LCD_WR_REG(0x2a);//列地址设置
    LCD_WR_DATA(x1 + OFFSET_X);
    LCD_WR_DATA(x2 + OFFSET_X);
    LCD_WR_REG(0x2b);//行地址设置
    LCD_WR_DATA(y1 + OFFSET_Y);
    LCD_WR_DATA(y2 + OFFSET_Y);
    LCD_WR_REG(0x2c);//储存器写
}


/******************************************************************************
      函数说明：LCD调节背光
      入口数据：dc,占空比,5%~100%
      返回值：  无
******************************************************************************/
void LCD_Set_Light(uint8_t dc)
{
	if(dc>=5 && dc<=100)
		__HAL_TIM_SetCompare(&htim1,TIM_CHANNEL_3,dc*PWM_PERIOD/100);
}


/******************************************************************************
      函数说明：LCD关闭背光
      入口数据：无
      返回值：  无
******************************************************************************/
void LCD_Close_Light(void)
{
	__HAL_TIM_SetCompare(&htim1,TIM_CHANNEL_3,0);
	//HAL_TIM_PWM_Stop(&htim3,TIM_CHANNEL_3);
}


/******************************************************************************
      函数说明：LCD开启背光
      入口数据：无
      返回值：  无
******************************************************************************/
void LCD_Open_Light(void)
{
	HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_3);
}

/******************************************************************************
      函数说明：ST7789 SLEEP IN
      入口数据：无
      返回值：  无
******************************************************************************/
void LCD_ST7789_SleepIn(void)
{
	LCD_WR_REG(0x10);
	HAL_Delay(100);
}


/******************************************************************************
      函数说明：ST7789 SLEEP OUT
      入口数据：无
      返回值：  无
******************************************************************************/
void LCD_ST7789_SleepOut(void)
{
	LCD_WR_REG(0x11);
	HAL_Delay(100);
}

/******************************************************************************
	函数说明：设置屏幕旋转方向
	入口数据：rotation 0, 90, 180, 270
	返回值：  无
******************************************************************************/
void LCD_SetRotation(uint16_t rotation) {
	// 确保SPI总线空闲，避免在发送命令时发生冲突（因为DMA是16bit的）
	while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY) {
        __NOP(); // 只是为了防止编译器过度优化空循环
    }
	LCD_WR_REG(0x36); // Memory Data Access Control
	switch(rotation) {
		case 0:
			OFFSET_X = 0;
			OFFSET_Y = 0;
			LCD_WR_DATA8(0x00);
			break;
		case 180:
			OFFSET_X = 0;
			OFFSET_Y = 80;
			LCD_WR_DATA8(0xC0);
			break;
		case 90:
			OFFSET_X = 0;
			OFFSET_Y = 0;
			LCD_WR_DATA8(0x70);
			break;
		case 270:
			OFFSET_X = 80;
			OFFSET_Y = 0;
			LCD_WR_DATA8(0xA0);
			break;
		default:
			OFFSET_X = 0;
			OFFSET_Y = 0;
			LCD_WR_DATA8(0x00);
			break;
	}
}

/******************************************************************************
      函数说明：LCD初始化
      入口数据：无
      返回值：  无
******************************************************************************/
void LCD_Init(void)
{
	LCD_GPIO_Init();//初始化GPIO
	LCD_CS_Clr();		//chip select

	LCD_RES_Clr();	//复位
	HAL_Delay(100);
	LCD_RES_Set();
	HAL_Delay(100);

	// 1. 退出睡眠模式 (Exit Sleep Mode)
	LCD_WR_REG(0x11);
	HAL_Delay(120);

	// 2. 设置显示方向 (Memory Data Access Control)
	LCD_SetRotation(LCD_ROTATION);

	// 3. 设置像素格式 (Interface Pixel Format)
    // 0x55 表示 16位色 (RGB565)。如果设置为0x66 (18位色)，颜色会不正确。
	LCD_WR_REG(0x3A);
	LCD_WR_DATA8(0x55);

	// 4. Porch 设置 (时序控制)
    // 控制行扫描的前后肩时间，通常不需要修改。
	LCD_WR_REG(0xB2);
	LCD_WR_DATA8(0x0C);
	LCD_WR_DATA8(0x0C);
	LCD_WR_DATA8(0x00);
	LCD_WR_DATA8(0x33);
	LCD_WR_DATA8(0x33);

	// 5. Gate 控制 (Gate Control)
    // 【色彩关键】设置VGH和VGL电压，影响屏幕的对比度和上下视角。
    // 0x35是一个比较通用的值。如果屏幕有垂直方向的串扰或闪烁，可以尝试修改这个值。
	LCD_WR_REG(0xB7);
	LCD_WR_DATA8(0x35);

	// 6. VCOM 设置 (VCOM Setting)
    // 【色彩关键】这是非常重要的参数，直接影响显示质量，特别是对比度和闪烁。
    // 0x19 是一个参考值，可以在 0x10 到 0x30 之间尝试，找到最稳定的值。
	LCD_WR_REG(0xBB);
	LCD_WR_DATA8(0x19);

	// 7. LCM 控制 (LCM Control)
	LCD_WR_REG(0xC0);
	LCD_WR_DATA8(0x2C);

	// 8. VDV 和 VRH 命令使能
	LCD_WR_REG(0xC2);
	LCD_WR_DATA8(0x01);

	// 9. VRH 设置 (VRH Set)
    // 【色彩关键】影响驱动电压，可以微调屏幕的整体亮度和功耗。
	LCD_WR_REG(0xC3);
	LCD_WR_DATA8(0x12);

	// 10. VDV 设置 (VDV Set)
    // 通常保持默认值。
	LCD_WR_REG(0xC4);
	LCD_WR_DATA8(0x20);

	// 11. 帧率控制 (Frame Rate Control)
    // 0x0F 通常对应 60Hz 左右的刷新率。如果屏幕闪烁，可以尝试减小这个值（例如0x14），但会增加功耗。
	LCD_WR_REG(0xC6);
	LCD_WR_DATA8(0x0F);

	// 12. 电源控制 (Power Control)
	LCD_WR_REG(0xD0);
	LCD_WR_DATA8(0xA6);
	LCD_WR_DATA8(0xA1);

	// 13. 正 Gamma 校正 (Positive Voltage Gamma Control)
    // 【色彩关键】这是调整颜色和灰阶表现最核心的部分！
    // 这14个字节定义了从暗到亮的灰阶曲线。修改这些值可以显著改变色彩的饱和度、对比度和过渡平滑度。
	LCD_WR_REG(0xE0);
	LCD_WR_DATA8(0xD0);
	LCD_WR_DATA8(0x04);
	LCD_WR_DATA8(0x0D);
	LCD_WR_DATA8(0x11);
	LCD_WR_DATA8(0x13);
	LCD_WR_DATA8(0x2B);
	LCD_WR_DATA8(0x3F);
	LCD_WR_DATA8(0x54);
	LCD_WR_DATA8(0x4C);
	LCD_WR_DATA8(0x18);
	LCD_WR_DATA8(0x0D);
	LCD_WR_DATA8(0x0B);
	LCD_WR_DATA8(0x1F);
	LCD_WR_DATA8(0x23);

	// 14. 负 Gamma 校正 (Negative Voltage Gamma Control)
    // 【色彩关键】与正Gamma同样重要，两者需要配合调整以达到最佳效果。
	LCD_WR_REG(0xE1);
	LCD_WR_DATA8(0xD0);
	LCD_WR_DATA8(0x04);
	LCD_WR_DATA8(0x0C);
	LCD_WR_DATA8(0x11);
	LCD_WR_DATA8(0x13);
	LCD_WR_DATA8(0x2C);
	LCD_WR_DATA8(0x3F);
	LCD_WR_DATA8(0x44);
	LCD_WR_DATA8(0x51);
	LCD_WR_DATA8(0x2F);
	LCD_WR_DATA8(0x1F);
	LCD_WR_DATA8(0x1F);
	LCD_WR_DATA8(0x20);
	LCD_WR_DATA8(0x23);

	// 15. 开启颜色反转 (Display Inversion On)
	LCD_WR_REG(0x21);

	// 16. 开启显示 (Display On)
	LCD_WR_REG(0x29);
}



