/*
    \file   main.c
    \brief  ADC sample accumulator
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

#define ADC_SHIFT_DIV64    (6)

uint16_t volatile adcVal;

static void LED0_init(void);
static void LED0_toggle(void);
static void ADC0_init(void);
static uint16_t ADC0_read(void);

int main(void)
{
    LED0_init();
    ADC0_init();
    
    while (1) 
    {
        adcVal = ADC0_read();
        
        /* divide by 64 */
        adcVal = adcVal >> ADC_SHIFT_DIV64;
        LED0_toggle();
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
    
    /* Select ADC channel */
    ADC0.MUXPOS = ADC_MUXPOS_AIN18_gc;
    
    /* Set the accumulator mode to accumulate 64 samples */
    ADC0.CTRLB = ADC_SAMPNUM_ACC64_gc;
    
    ADC0.CTRLA = ADC_ENABLE_bm          /* ADC Enable: enabled */
               | ADC_RESSEL_10BIT_gc;   /* 10-bit mode */
}

static uint16_t ADC0_read(void)
{
    /* Start ADC conversion */
    ADC0.COMMAND = ADC_STCONV_bm;
    
    /* Wait until ADC conversion done */
    while ( !(ADC0.INTFLAGS & ADC_RESRDY_bm) )
    {
        ;
    }
    
    /* Clear the interrupt flag by writing 1: */
    ADC0.INTFLAGS = ADC_RESRDY_bm;
    
    return ADC0.RES;
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
