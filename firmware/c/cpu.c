/* cpu.c */

#include <stdint.h>
#include "cpu.h"
#include "display.h"
#include "loop.h"
#include "pressure.h"
#include "uart.h"
#include "utils.h"

// global data
uint32_t mainStack[400];

// Fault handlers
static void fault( void )
{
   while(1){}
}

static void NMI( void )           { fault(); }
static void FaultISR( void )      { fault(); }
static void MPUFaultISR( void )   { fault(); }
static void BusFaultISR( void )   { fault(); }
static void UsageFaultISR( void ) { fault(); }
static void BadISR( void )        { fault(); }

// Do some one time init of the CPU
void CPU_Init( void )
{
   RCC_Regs *rcc = (RCC_Regs *)RCC_BASE;

   // Enable the FPU
   SysCtrl_Reg *sysCtl = (SysCtrl_Reg *)SYSCTL_BASE;
   sysCtl->cpac = 0x00F00000;

   // Enable clocks to peripherials I use
   rcc->periphClkEna[0] = 0x00001101;  // Flash, DMA1, CRC
   rcc->periphClkEna[1] = 0x00002007;  // GPIO, ADC
   rcc->periphClkEna[4] = 0x15200001;  // Timer 2, I2C1, USB, CRS, Power
   rcc->periphClkEna[6] = 0x00035800;  // UART1, Timers 1, 15, 16, SPI1

   // Reset caches and set latency for 80MHz opperation
   FlashReg *flash = (FlashReg *)FLASH_BASE;
   flash->access = 0x00000004;
   flash->access = 0x00001804;
   flash->access = 0x00001804;
   flash->access = 0x00000604;

   // Turn on the HSI48 clock used for USB
   rcc->recovery = 1;

   // Fin = 4MHz
   // Fvco = Fin * (N/M)
   //
   // I'll use 160MHz for Fvco and divide by 2
   // for output clocks
   int N = 40;
   int M = 1;
   rcc->pllCfg = 0x01000001 | (N<<8) | ((M-1)<<4);

   // Turn on the PLL and HSI16
   rcc->clkCtrl |= 0x01000100;

   // Wait for PLL ready
   while( !(rcc->clkCtrl & 0x02000000) ){}

   // Set PLL as system clock
   rcc->clkCfg = 0x00000003;

   // Route the HSI16 clock to I2C1.
   // Mostly because I'm too lazy to figure out the complex timing
   // register settings for the i2c peripherial for a clock frequency
   // that's not given as an example in the reference manual.
   rcc->indClkCfg = 0x00002000;

   // Turn on power to the USB module
   PwrCtrl_Reg *pwr = (PwrCtrl_Reg *)POWER_BASE;
   pwr->ctrl[1] = 0x00000410;

   // Enable interrupts
   IntEnable();
}

// Enable an interrupt with a specified priority (0 to 15)
void EnableInterrupt( int id, int pri )
{
   IntCtrl_Regs *nvic = (IntCtrl_Regs *)NVIC_BASE;

   id -= 16;
   nvic->setEna[ id>>5 ] = 1<<(id&0x1F);

   // The STM32 processor implements bits 4-7 of the NVIM priority register.
   nvic->priority[id] = pri<<4;
}

void DisableInterrupt( int id )
{
   IntCtrl_Regs *nvic = (IntCtrl_Regs *)NVIC_BASE;
   id -= 16;
   nvic->clrEna[ id>>5 ] = 1<<(id&0x1F);
}

//void Reset( void )
//{
//   // Reset 
//   SysCtrl_Reg *sysCtl = (SysCtrl_Reg *)SYSCTL_BASE;
//   sysCtl->apInt = 0x05FA0004;
//}

// Interrupt vector table.
extern void ResetVect( void );
__attribute__ ((section(".isr_vector")))
void (* const vectors[])(void) =
{
   (void (*)(void))((uint32_t)mainStack + sizeof(mainStack)),
   (void(*)(void))((uint32_t)ResetVect+1), //   1 - 0x004 The reset handler
   NMI,                                    //   2 - 0x008 The NMI handler
   FaultISR,                               //   3 - 0x00C The hard fault handler
   MPUFaultISR,                            //   4 - 0x010 The MPU fault handler
   BusFaultISR,                            //   5 - 0x014 The bus fault handler
   UsageFaultISR,                          //   6 - 0x018 The usage fault handler
   BadISR,                                 //   7 - 0x01C Reserved
   BadISR,                                 //   8 - 0x020 Reserved
   BadISR,                                 //   9 - 0x024 Reserved
   BadISR,                                 //  10 - 0x028 Reserved
   BadISR,                                 //  11 - 0x02C SVCall handler
   BadISR,                                 //  12 - 0x030 Debug monitor handler
   BadISR,                                 //  13 - 0x034 Reserved
   BadISR,                                 //  14 - 0x038 The PendSV handler
   BadISR,                                 //  15 - 0x03C 
   BadISR,                                 //  16 - 0x040 
   BadISR,                                 //  17 - 0x044 
   BadISR,                                 //  18 - 0x048 
   BadISR,                                 //  19 - 0x04C 
   BadISR,                                 //  20 - 0x050 
   BadISR,                                 //  21 - 0x054 
   BadISR,                                 //  22 - 0x058 
   BadISR,                                 //  23 - 0x05C 
   BadISR,                                 //  24 - 0x060 
   BadISR,                                 //  25 - 0x064 
   BadISR,                                 //  26 - 0x068 
   BadISR,                                 //  27 - 0x06C 
   BadISR,                                 //  28 - 0x070 
   BadISR,                                 //  29 - 0x074 
   BadISR,                                 //  30 - 0x078 
   BadISR,                                 //  31 - 0x07C 
   DispISR,                                //  32 - 0x080 
   BadISR,                                 //  33 - 0x084 
   BadISR,                                 //  34 - 0x088 
   BadISR,                                 //  35 - 0x08C 
   BadISR,                                 //  36 - 0x090 
   BadISR,                                 //  37 - 0x094 
   BadISR,                                 //  38 - 0x098 
   BadISR,                                 //  39 - 0x09C 
   LoopISR,                                //  40 - 0x0A0 
   BadISR,                                 //  41 - 0x0A4 
   BadISR,                                 //  42 - 0x0A8 
   BadISR,                                 //  43 - 0x0AC 
   BadISR,                                 //  44 - 0x0B0 
   BadISR,                                 //  45 - 0x0B4 
   BadISR,                                 //  46 - 0x0B8 
   DispISR,                                //  47 - 0x0BC 
   BadISR,                                 //  48 - 0x0C0 
   BadISR,                                 //  49 - 0x0C4 
   BadISR,                                 //  50 - 0x0C8 
   SPI1_ISR,                               //  51 - 0x0CC - SPI1
   BadISR,                                 //  52 - 0x0D0 
   UART_ISR,                               //  53 - 0x0D4 
   BadISR,                                 //  54 - 0x0D8 
   BadISR,                                 //  55 - 0x0DC 
   BadISR,                                 //  56 - 0x0E0 
   BadISR,                                 //  57 - 0x0E4 
   BadISR,                                 //  58 - 0x0E8 
   BadISR,                                 //  59 - 0x0EC 
   BadISR,                                 //  60 - 0x0F0 
   BadISR,                                 //  61 - 0x0F4 
   BadISR,                                 //  62 - 0x0F8 
   BadISR,                                 //  63 - 0x0FC 
};


void LoopISR( void );
