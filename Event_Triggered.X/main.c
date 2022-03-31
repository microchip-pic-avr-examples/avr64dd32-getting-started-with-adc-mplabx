/*
    \file   main.c
    \brief  ADC triggered by RTC Overflow
    (c) 2022 Microchip Technology Inc. and its subsidiaries.
    Subject to your compliance with these terms, you may use Microchip software and any
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party
    license terms applicable to your use of third party software (including open source software) that
    may accompany Microchip software.
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS
    FOR A PARTICULAR PURPOSE.
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS
    SOFTWARE.
*/

#include <avr/io.h>
#include <avr/interrupt.h>

/* Default fuses configuration:
- BOD disabled
- Oscillator in High-Frequency Mode
- UPDI pin active(WARNING: DO NOT CHANGE!)
- RESET pin used as GPIO
- CRC disabled
- MVIO enabled for dual supply
- Watchdog Timer disabled
*/
FUSES =
{
.BODCFG = ACTIVE_DISABLE_gc | LVL_BODLEVEL0_gc | SAMPFREQ_128Hz_gc | SLEEP_DISABLE_gc,
.BOOTSIZE = 0x0,
.CODESIZE = 0x0,
.OSCCFG = CLKSEL_OSCHF_gc,
.SYSCFG0 = CRCSEL_CRC16_gc | CRCSRC_NOCRC_gc | RSTPINCFG_GPIO_gc | UPDIPINCFG_UPDI_gc,
.SYSCFG1 = MVSYSCFG_DUAL_gc | SUT_0MS_gc,
.WDTCFG = PERIOD_OFF_gc | WINDOW_OFF_gc,
};

/* RTC Period */
#define RTC_PERIOD        511 /* Period of 500ms: 32768 Hz / 32 / (511+1) = 2Hz */

uint16_t volatile adcVal;

static void ADC0_init(void);
static void LED0_init(void);
static void LED0_toggle(void);
static void RTC_init(void);
static void EVSYS_init(void);

ISR(ADC0_RESRDY_vect)
{
    /* Clear flag by writing '1': */
    ADC0.INTFLAGS = ADC_RESRDY_bm;
    adcVal = ADC0.RES;    
    LED0_toggle();
}

int main(void)
{
    ADC0_init();
    LED0_init();
    RTC_init();
    EVSYS_init();
    
    /* Enable Global Interrupts */
    sei();
    
    while (1) 
    {
        ;
    }
}

static void ADC0_init(void)
{
    /* Select ADC voltage reference */
    VREF.ADC0REF = VREF_REFSEL_1V024_gc;
    
    /* Disable digital input buffer */
    PORTF.PIN2CTRL &= ~PORT_ISC_gm;
    PORTF.PIN2CTRL |= PORT_ISC_INPUT_DISABLE_gc;
    
    /* Disable pull-up resistor */
    PORTF.PIN2CTRL &= ~PORT_PULLUPEN_bm;

    ADC0.CTRLC = ADC_PRESC_DIV4_gc;      /* CLK_PER divided by 4 */
    
    ADC0.CTRLA = ADC_ENABLE_bm          /* ADC Enable: enabled */
               | ADC_RESSEL_10BIT_gc;   /* 10-bit mode */
    
    /* Select ADC channel */
    ADC0.MUXPOS  = ADC_MUXPOS_AIN18_gc;
    
    /* Enable interrupts */
    ADC0.INTCTRL |= ADC_RESRDY_bm;
    
    /* Enable event triggered conversion */
    ADC0.EVCTRL |= ADC_STARTEI_bm;
}

static void LED0_init(void)
{
    /* Make High (LED OFF) */
    PORTF.OUTSET = PIN5_bm;
    /* Make output */
    PORTF.DIRSET = PIN5_bm;
}

static void LED0_toggle(void)
{
    PORTF.OUTTGL = PIN5_bm;
}

static void RTC_init(void)
{
    uint8_t temp;
    
    /* Initialize 32.768kHz Oscillator: */
    /* Disable oscillator: */
    temp = CLKCTRL.XOSC32KCTRLA;
    temp &= ~CLKCTRL_ENABLE_bm;
    /* Enable writing to protected register */
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.XOSC32KCTRLA = temp;
    
    while(CLKCTRL.MCLKSTATUS & CLKCTRL_XOSC32KS_bm)
    {
        ; /* Wait until XOSC32KS becomes 0 */
    }
    
    /* SEL = 0 (Use External Crystal): */
    temp = CLKCTRL.XOSC32KCTRLA;
    temp &= ~CLKCTRL_SEL_bm;
    /* Enable writing to protected register */
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.XOSC32KCTRLA = temp;
    
    /* Enable oscillator: */
    temp = CLKCTRL.XOSC32KCTRLA;
    temp |= CLKCTRL_ENABLE_bm;
    /* Enable writing to protected register */
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.XOSC32KCTRLA = temp;
    
    /* Initialize RTC: */
    while (RTC.STATUS > 0)
    {
        ; /* Wait for all register to be synchronized */
    }

    RTC.CTRLA = RTC_PRESCALER_DIV32_gc  /* 32 */
              | RTC_RTCEN_bm            /* Enable: enabled */
              | RTC_RUNSTDBY_bm;        /* Run In Standby: enabled */

    /* Set period */
    RTC.PER = RTC_PERIOD;

    /* 32.768kHz External Crystal Oscillator (XOSC32K) */
    RTC.CLKSEL = RTC_CLKSEL_XTAL32K_gc;

    /* Run in debug: enabled */
    RTC.DBGCTRL |= RTC_DBGRUN_bm;
}

static void EVSYS_init(void)
{
    /* Real Time Counter overflow */
    EVSYS.CHANNEL0 = EVSYS_CHANNEL0_RTC_OVF_gc;
    /* Connect user to event channel 0 */
    EVSYS.USERADC0START = EVSYS_USER_CHANNEL0_gc;
}

