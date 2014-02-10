#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "global.h"

#define STEPS_BUF_SZ	8

typedef struct
{
	uint32_t steps[4];
	uint32_t pscValue[4], arrValue[4], f[4];
	uint8_t dir[4];
} LINE_DATA;

volatile LINE_DATA steps_buf[STEPS_BUF_SZ];
int32_t steps_buf_sz, steps_buf_p1, steps_buf_p2;

#if (USE_STEP_DEBUG == 1)
	LINE_DATA cur_steps_buf; // for debug only
#endif

#if (STEPS_MOTORS > 0)
volatile struct
{
	int32_t globalSteps;
	uint32_t steps;
	uint8_t clk, dir, isInProc;
} step_motors[STEPS_MOTORS];
#endif

void stepm_powerOff(uint8_t id)
{
	steps_buf_sz = steps_buf_p1 = steps_buf_p2 = 0;
	switch (id)
	{
#ifdef M0_EN_PORT
	case 0: GPIO_ResetBits(M0_EN_PORT, M0_EN_PIN); break;
#endif
#ifdef M1_EN_PORT
	case 1: GPIO_ResetBits(M1_EN_PORT, M1_EN_PIN); break;
#endif
#ifdef M2_EN_PORT
	case 2: GPIO_ResetBits(M2_EN_PORT, M2_EN_PIN); break;
#endif
#ifdef M3_EN_PORT
	case 3: GPIO_ResetBits(M3_EN_PORT, M3_EN_PIN); break;
#endif
	default: break;
	}
}

void stepm_init(void)
{
	steps_buf_sz = steps_buf_p1 = steps_buf_p2 = 0;

#if (STEPS_MOTORS > 0)
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBase;
	int i;

	PIN_SPEED_LOW();
	PIN_OUTPUT_PP();

	for (i = 0; i < STEPS_MOTORS; i++)
	{
		step_motors[i].steps = 0;
		step_motors[i].clk = true;
		step_motors[i].isInProc = false;
		step_motors[i].globalSteps = 0;
		stepm_powerOff(i);
		switch (i)
		{
	#ifdef M0_EN_PORT
		case 0:
			PIN_SET_MODE(M0_EN_PORT,   M0_EN_PIN);
			PIN_SET_MODE(M0_DIR_PORT,  M0_DIR_PIN);
			PIN_SET_MODE(M0_STEP_PORT, M0_STEP_PIN);
			break;
	#endif
	#ifdef M1_EN_PORT
		case 1:
			PIN_SET_MODE(M1_EN_PORT,   M1_EN_PIN);
			PIN_SET_MODE(M1_DIR_PORT,  M1_DIR_PIN);
			PIN_SET_MODE(M1_STEP_PORT, M1_STEP_PIN);
			break;
	#endif
	#ifdef M2_EN_PORT
		case 2:
			PIN_SET_MODE(M2_EN_PORT,   M2_EN_PIN);
			PIN_SET_MODE(M2_DIR_PORT,  M2_DIR_PIN);
			PIN_SET_MODE(M2_STEP_PORT, M2_STEP_PIN);
			break;
	#endif
	#ifdef M3_EN_PORT
		case 3:
			PIN_SET_MODE(M3_EN_PORT,   M3_EN_PIN);
			PIN_SET_MODE(M3_DIR_PORT,  M3_DIR_PIN);
			PIN_SET_MODE(M3_STEP_PORT, M3_STEP_PIN);
			break;
	#endif
		}
	}

	TIM_TimeBase.TIM_Prescaler = 1799;	// any
	TIM_TimeBase.TIM_Period = 100;		// any
	TIM_TimeBase.TIM_ClockDivision = 0;
	TIM_TimeBase.TIM_CounterMode = TIM_CounterMode_Up;

	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);

	#ifdef M0_EN_PORT
		NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
		NVIC_Init(&NVIC_InitStructure);

		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
		TIM_TimeBaseInit(M0_TIM, &TIM_TimeBase);
		M0_TIM->EGR = TIM_PSCReloadMode_Update;
		TIM_ARRPreloadConfig(M0_TIM, ENABLE);
		TIM_ITConfig(M0_TIM, TIM_IT_Update, ENABLE);
		TIM_Cmd(M0_TIM, ENABLE);
	#endif

	#ifdef M1_EN_PORT
	M1_TIM_INIT();
	#endif

	#ifdef M2_EN_PORT
	M2_TIM_INIT();
	#endif

	#ifdef M3_EN_PORT
	M3_TIM_INIT();
	#endif

#endif
}

void stepm_proc(uint8_t id)
{
	if (limits_chk())
	{
		stepm_EmergeStop();
		return;
	}
#if (STEPS_MOTORS > 0)
	if (step_motors[id].isInProc)
	{
		switch (id)
		{
#ifdef M0_EN_PORT
		case 0:
			if (step_motors[id].clk)
				GPIO_SetBits(M0_STEP_PORT, M0_STEP_PIN);
			else
				GPIO_ResetBits(M0_STEP_PORT, M0_STEP_PIN);
			break;
#endif
#ifdef M1_EN_PORT
		case 1:
			if (step_motors[id].clk)
				GPIO_SetBits(M1_STEP_PORT, M1_STEP_PIN);
			else
				GPIO_ResetBits(M1_STEP_PORT, M1_STEP_PIN);
			break;
#endif
#ifdef M2_EN_PORT
		case 2:
			if (step_motors[id].clk)
				GPIO_SetBits(M2_STEP_PORT, M2_STEP_PIN);
			else
				GPIO_ResetBits(M2_STEP_PORT, M2_STEP_PIN);
			break;
#endif
#ifdef M3_EN_PORT
		case 3:
			if (step_motors[id].clk)
				GPIO_SetBits(M3_STEP_PORT, M3_STEP_PIN);
			else
				GPIO_ResetBits(M3_STEP_PORT, M3_STEP_PIN);
			break;
#endif
		default:
			break;
		}
		if (!step_motors[id].clk)
		{
			if (step_motors[id].steps != 0)
				step_motors[id].steps--;
			if (step_motors[id].dir)
				step_motors[id].globalSteps++;
			else
				step_motors[id].globalSteps--;
		}
		else
		{
			if (step_motors[id].steps == 0)
				step_motors[id].isInProc = false;
		}
		step_motors[id].clk = !step_motors[id].clk;
	}
	else
	{
		if (steps_buf_sz > 0
#if (STEPS_MOTORS >= 1)
			&& !step_motors[0].isInProc
#endif
#if (STEPS_MOTORS >= 2)
			&& !step_motors[1].isInProc
#endif
#if (STEPS_MOTORS >= 3)
			&& !step_motors[2].isInProc
#endif
#if (STEPS_MOTORS >= 4)
			&& !step_motors[3].isInProc
#endif
			)
		{
			int i;

			__disable_irq();
			LINE_DATA *p = (LINE_DATA *)(&steps_buf[steps_buf_p1]);
#if (USE_STEP_DEBUG == 1)
			memcpy(&cur_steps_buf, p, sizeof(cur_steps_buf)); // for debug
#endif
			for (i = 0; i < STEPS_MOTORS; i++)
			{
				step_motors[i].steps = p->steps[i];
				step_motors[i].dir = p->dir[i];
			}

#ifdef M0_EN_PORT
			if (step_motors[0].steps)
			{
				step_motors[0].isInProc = true;
				GPIO_WriteBit(M0_DIR_PORT, M0_DIR_PIN, p->dir[0] ? Bit_SET : Bit_RESET);
				GPIO_SetBits(M0_EN_PORT, M0_EN_PIN);
				TIM2->PSC = p->pscValue[0];
				TIM_SetAutoreload(TIM2, p->arrValue[0]);
			}
#endif
#ifdef M1_EN_PORT
			if (step_motors[1].steps)
			{
				step_motors[1].isInProc = true;
				GPIO_WriteBit(M1_DIR_PORT, M1_DIR_PIN, p->dir[1] ? Bit_SET : Bit_RESET);
				GPIO_SetBits(M1_EN_PORT, M1_EN_PIN);
				TIM3->PSC = p->pscValue[1];
				TIM_SetAutoreload(TIM3, p->arrValue[1]);
			}
#endif
#ifdef M2_EN_PORT
			if (step_motors[2].steps)
			{
#if (USE_ENCODER == 1)
				if (isEncoderCorrection && step_motors[2].steps > ENCODER_CORRECTION_MIN_STEPS)
				{
					int32_t enVal = encoderZvalue();
					int32_t globalSteps = stepm_getCurGlobalStepsNum(2);
					int32_t dv = enVal*MM_PER_360 * 1000L / ENCODER_Z_CNT_PER_360 - globalSteps * MM_PER_360 * 1000L / SM_Z_STEPS_PER_360; // delta in 1/1000 mm
					encoderCorrectionDelta = dv; // TODO for debug.. will be removed
					if (labs(encoderCorrectionMaxDelta) < labs(encoderCorrectionDelta))
						encoderCorrectionMaxDelta = encoderCorrectionDelta;

					if (dv > ENCODER_Z_MIN_MEASURE)
					{
						encoderCorrectionCntDown++; // TODO for debug.. will be removed

						step_motors[2].globalSteps++;
						if (step_motors[2].dir) step_motors[2].steps--;
						else step_motors[2].steps++;
					}
					if (dv < -ENCODER_Z_MIN_MEASURE)
					{
						encoderCorrectionCntUp++; // TODO for debug.. will be removed

						step_motors[2].globalSteps--;
						if (step_motors[2].dir) step_motors[2].steps++;
						else step_motors[2].steps--;
					}
				}
#endif
				step_motors[2].isInProc = true;
				GPIO_WriteBit(M2_DIR_PORT, M2_DIR_PIN, p->dir[2] ? Bit_RESET : Bit_SET);
				GPIO_SetBits(M2_EN_PORT, M2_EN_PIN);
				M2_TIM->PSC = p->pscValue[2];
				TIM_SetAutoreload(M2_TIM, p->arrValue[2]);
			}
#endif
#ifdef M3_EN_PORT
			if (step_motors[3].steps)
			{
				step_motors[3].isInProc = true;
				GPIO_WriteBit(M3_DIR_PORT, M3_DIR_PIN, p->dir[3] ? Bit_RESET : Bit_SET);
				GPIO_SetBits(M3_EN_PORT, M3_EN_PIN);
				M3_TIM->PSC = p->pscValue[3];
				TIM_SetAutoreload(M3_TIM, p->arrValue[3]);
			}
#endif
			steps_buf_p1++;

			if (steps_buf_p1 >= STEPS_BUF_SZ)
				steps_buf_p1 = 0;
			steps_buf_sz--;
			__enable_irq();
		}
	}
#endif
}

void stepm_EmergeStop(void)
{
#if (STEPS_MOTORS > 0)
	int i;
	for (i = 0; i < STEPS_MOTORS; i++)
	{
		stepm_powerOff(i);
		step_motors[i].isInProc = false;
		step_motors[i].steps = 0;
	}
#endif
	steps_buf_sz = steps_buf_p1 = steps_buf_p2 = 0;
}

void stepm_addMove(uint32_t steps[4], uint32_t frq[4], uint8_t dir[4])
{
	uint32_t pscValue;


	if (steps[0] == 0 && steps[1] == 0 && steps[2] == 0)
		return;

	while (steps_buf_sz >= STEPS_BUF_SZ)
		__NOP();

	LINE_DATA *p = (LINE_DATA *)(&steps_buf[steps_buf_p2]);

	for (int i = 0; i < 3; i++)
	{
		uint32_t f = frq[i];
		if (f >(15000 * K_FRQ))
			f = 15000 * K_FRQ;	// 15kHz
		if (f < K_FRQ)
			f = K_FRQ;			// 1Hz

		// 72Mhz / (psc * arr) = frq
		pscValue = 1;

		uint32_t arrValue = (72000000UL / 2 * K_FRQ) / f; // (1 falling age on 2 IRQ)
		while ((arrValue & 0xffff0000) != 0)
		{
			pscValue = pscValue << 1;
			arrValue = arrValue >> 1;
		}
		pscValue--;
		p->f[i] = frq[i]; // for debug
		p->arrValue[i] = arrValue;
		p->pscValue[i] = pscValue;
		p->dir[i] = dir[i]; p->steps[i] = steps[i];
	}

	steps_buf_p2++;
	if (steps_buf_p2 >= STEPS_BUF_SZ)
		steps_buf_p2 = 0;
	__disable_irq();
	steps_buf_sz++;
	__enable_irq();
}

int32_t stepm_getRemainLines(void)
{
	return steps_buf_sz;
}

int32_t stepm_inProc(void)
{
	if (steps_buf_sz > 0)
		return true;
#if (STEPS_MOTORS > 0)
	int i;
	for (i = 0; i < STEPS_MOTORS; i++)
		if (step_motors[i].isInProc)
			return true;
#endif
	return false;
}

uint32_t stepm_LinesBufferIsFull(void)
{
	return steps_buf_sz >= STEPS_BUF_SZ;
}

int32_t stepm_getCurGlobalStepsNum(uint8_t id)
{
#if (STEPS_MOTORS > 0)
	return step_motors[id].globalSteps;
#else
	return 0;
#endif
}

void stepm_ZeroGlobalCrd(void)
{
#if (STEPS_MOTORS > 0)
	int i;
	for (i = 0; i < 4; i++)
		step_motors[i].globalSteps = 0;
#endif
}

#if (USE_STEP_DEBUG == 1)
void step_dump()
{
	#if (USE_LCD != 0)
	LINE_DATA *p = (LINE_DATA *)(&cur_steps_buf);
	for (int i = 0; i < 4; i++)
	{
		scr_gotoxy(1, 7 + i);
		scr_printf("%d,%d,%d,%d [%d]   ", p->steps[i], p->dir[i], p->pscValue[i], p->arrValue[i], p->f[i]); // TODO
	}
	#endif
}
#endif