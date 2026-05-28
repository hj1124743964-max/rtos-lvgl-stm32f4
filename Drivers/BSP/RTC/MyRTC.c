#include "./BSP/RTC/MyRTC.h"

#define MYRTC_BKP_VALUE        0xA5A6U
#define MYRTC_LSE_TIMEOUT_MS   1000U

RTC_HandleTypeDef hrtc;
uint16_t MyRTC_Time[6] = {2026, 5, 23, 10, 28, 0};

static uint8_t MyRTC_GetWeekDay(uint16_t year, uint8_t month, uint8_t day)
{
    static const uint8_t offset[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    uint8_t week;

    if (month < 3U)
    {
        year--;
    }

    week = (uint8_t)((year + year / 4U - year / 100U + year / 400U + offset[month - 1U] + day) % 7U);

    return (week == 0U) ? RTC_WEEKDAY_SUNDAY : week;
}

static HAL_StatusTypeDef MyRTC_ConfigClock(uint32_t rtc_clk_source)
{
    RCC_PeriphCLKInitTypeDef periph_clk = {0};

    periph_clk.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    periph_clk.RTCClockSelection = rtc_clk_source;

    return HAL_RCCEx_PeriphCLKConfig(&periph_clk);
}

static HAL_StatusTypeDef MyRTC_EnableLSE(void)
{
    RCC_OscInitTypeDef osc = {0};
    uint32_t tickstart;

    osc.OscillatorType = RCC_OSCILLATORTYPE_LSE;
    osc.LSEState = RCC_LSE_ON;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK)
    {
        return HAL_ERROR;
    }

    tickstart = HAL_GetTick();
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY) == RESET)
    {
        if ((HAL_GetTick() - tickstart) > MYRTC_LSE_TIMEOUT_MS)
        {
            return HAL_TIMEOUT;
        }
    }

    return MyRTC_ConfigClock(RCC_RTCCLKSOURCE_LSE);
}

static HAL_StatusTypeDef MyRTC_EnableLSI(void)
{
    RCC_OscInitTypeDef osc = {0};

    osc.OscillatorType = RCC_OSCILLATORTYPE_LSI;
    osc.LSIState = RCC_LSI_ON;
    osc.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return MyRTC_ConfigClock(RCC_RTCCLKSOURCE_LSI);
}

void MyRTC_Init(void)
{
    HAL_StatusTypeDef status;

    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();

    status = MyRTC_EnableLSE();
    if (status == HAL_OK)
    {
        hrtc.Init.AsynchPrediv = 127;
        hrtc.Init.SynchPrediv = 255;
    }
    else
    {
        status = MyRTC_EnableLSI();
        hrtc.Init.AsynchPrediv = 127;
        hrtc.Init.SynchPrediv = 249;
    }

    if (status != HAL_OK)
    {
        return;
    }

    __HAL_RCC_RTC_ENABLE();

    hrtc.Instance = RTC;
    hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

    if (HAL_RTC_Init(&hrtc) != HAL_OK)
    {
        return;
    }

    if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) != MYRTC_BKP_VALUE)
    {
        MyRTC_SetTime();
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, MYRTC_BKP_VALUE);
    }
    else
    {
        MyRTC_ReadTime();
    }
}

void MyRTC_SetTime(void)
{
    RTC_TimeTypeDef time = {0};
    RTC_DateTypeDef date = {0};

    time.Hours = (uint8_t)MyRTC_Time[3];
    time.Minutes = (uint8_t)MyRTC_Time[4];
    time.Seconds = (uint8_t)MyRTC_Time[5];
    time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    time.StoreOperation = RTC_STOREOPERATION_RESET;

    date.Year = (uint8_t)(MyRTC_Time[0] - 2000U);
    date.Month = (uint8_t)MyRTC_Time[1];
    date.Date = (uint8_t)MyRTC_Time[2];
    date.WeekDay = MyRTC_GetWeekDay(MyRTC_Time[0], date.Month, date.Date);

    HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, MYRTC_BKP_VALUE);
}

void MyRTC_ReadTime(void)
{
    RTC_TimeTypeDef time = {0};
    RTC_DateTypeDef date = {0};

    HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);

    MyRTC_Time[0] = (uint16_t)(date.Year + 2000U);
    MyRTC_Time[1] = date.Month;
    MyRTC_Time[2] = date.Date;
    MyRTC_Time[3] = time.Hours;
    MyRTC_Time[4] = time.Minutes;
    MyRTC_Time[5] = time.Seconds;
}
