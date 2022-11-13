is_rdy_for_wakeup:                   // Read RTC_CNTL_RDY_FOR_WAKEUP bit
       // READ_RTC_FIELD(RTC_CNTL_LOW_POWER_ST_REG, RTC_CNTL_RDY_FOR_WAKEUP)
       // AND r0, r0, 1
       // JUMP is_rdy_for_wakeup, eq    // Retry until the bit is set
       // WAKE                          // Trigger wake up
       // REG_WR 0x006, 24, 24, 0       // Stop ULP timer (clear RTC_CNTL_ULP_CP_SLP_TIMER_EN)
       // HALT                          // Stop the ULP program
