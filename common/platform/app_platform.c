/*
 * Copyright (c) 2009-2020 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include "platform.h"
#include "app_cfg.h"

#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wunknown-warning-option"
#   pragma clang diagnostic ignored "-Wreserved-identifier"
#   pragma clang diagnostic ignored "-Wdeclaration-after-statement"
#   pragma clang diagnostic ignored "-Wformat-nonliteral"
#   pragma clang diagnostic ignored "-Wsign-compare"
#   pragma clang diagnostic ignored "-Wmissing-prototypes"
#   pragma clang diagnostic ignored "-Wcast-qual"
#   pragma clang diagnostic ignored "-Wsign-conversion"
#   pragma clang diagnostic ignored "-Wgnu-statement-expression"
#   pragma clang diagnostic ignored "-Wundef"
#elif defined(__IS_COMPILER_GCC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wpedantic"
#endif

#include "RTE_Components.h"

#include CMSIS_device_header
#if defined(RTE_Compiler_EventRecorder) || defined(RTE_CMSIS_View_EventRecorder)
#   include <EventRecorder.h>
#endif


extern void SysTick_Handler(void);
extern void _ttywrch(int ch);

__WEAK 
void platform_1ms_event_handler(void) 
{}

__WEAK 
void bsp_1ms_event_handler(void)
{}


#ifdef RTE_CMSIS_RTOS2_RTX5
void ARM_WRAP(osRtxTick_Handler)(void)
{
    platform_1ms_event_handler();
    bsp_1ms_event_handler();

#if __IS_COMPILER_GCC__ || __IS_COMPILER_LLVM__
    user_code_insert_to_systick_handler();
#endif

    extern void ARM_REAL(osRtxTick_Handler)(void);
    ARM_REAL(osRtxTick_Handler)();
}

#else
__attribute__((used))
void SysTick_Handler(void)
{
    platform_1ms_event_handler();
    
    bsp_1ms_event_handler();
    
#if __IS_COMPILER_GCC__ || __IS_COMPILER_LLVM__
    user_code_insert_to_systick_handler();
#endif
}
#endif

/*----------------------------------------------------------------------------*
 * A thread safe printf                                                       *
 *----------------------------------------------------------------------------*/
#if defined(RTE_CMSIS_RTOS2)

#if defined(__IS_COMPILER_GCC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wpedantic"
#endif

#if defined(__IS_COMPILER_GCC)
_ATTRIBUTE ((__format__ (__printf__, 1, 2)))
#elif defined(__IS_ARM_COMPILER_5__) || defined(__IS_ARM_COMPILER_6__)
#pragma __printf_args
__attribute__((__nonnull__(1)))
#endif
int	ARM_WRAP(printf) (const char *__restrict format, ...)
{
    va_list va;
    va_start(va, format);
    int ret = 0;
    arm_thread_safe { 
        ret = vprintf(format, va);
    }
    va_end(va);
    
    return ret;
}

#endif


__WEAK 
bool device_specific_init(void)
{
    //SysTick_Config(SystemCoreClock / 1000ul);
    return false;
}

__attribute__((used, constructor(101)))
void app_platform_init(void)
{
#if defined(RTE_Compiler_EventRecorder) || defined(RTE_CMSIS_View_EventRecorder)
    EventRecorderInitialize(0, 1);
#endif

    init_cycle_counter(device_specific_init());
}


int lcd_printf(int16_t x, int16_t y, const char *format, ...)
{
    int real_size;
    static char s_chBuffer[64];
    __va_list ap;
    va_start(ap, format);
        real_size = vsnprintf(s_chBuffer, sizeof(s_chBuffer)-1, format, ap);
    va_end(ap);
    real_size = MIN(sizeof(s_chBuffer)-1, real_size);
    s_chBuffer[real_size] = '\0';
    GLCD_DrawString(x, y, s_chBuffer);
    return real_size;
}



#if __IS_COMPILER_ARM_COMPILER_6__
__asm(".global __use_no_semihosting\n\t");
#   ifndef __MICROLIB
__asm(".global __ARM_use_no_argv\n\t");
#   endif

__NO_RETURN
void _sys_exit(int ret)
{
    UNUSED_PARAM(ret);
    while(1) {}
}

#endif

#if defined(__MICROLIB)
_ARMABI_NORETURN 
ARM_NONNULL(1,2)
void __aeabi_assert(const char *chCond, const char *chLine, int wErrCode) 
{
    ARM_2D_UNUSED(chCond);
    ARM_2D_UNUSED(chLine);
    ARM_2D_UNUSED(wErrCode);
    while(1) {
        __NOP();
    }
}
#endif

#if !defined(__IS_COMPILER_GCC__)
void _ttywrch(int ch)
{
    UNUSED_PARAM(ch);
}
#endif

#if defined(__clang__)
#   pragma clang diagnostic pop
#elif defined(__IS_COMPILER_GCC__)
#   pragma GCC diagnostic pop
#endif
