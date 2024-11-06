#include <nds/asminc.h>

    .syntax unified
    .arm

    // r0 -> EP_DATA, can over-read buffer
BEGIN_ASM_FUNC nrio_d14_tx_bytes itcm
    mov r2, #0x8400000

    cmp r1, #16
    blt .Lnrio_d14_tx_bytes4

    push {r4-r6}
.Lnrio_d14_tx_bytes16:
    subs r1, r1, #16
    ldmiage r0!, {r3-r6}
    stmiage r2!, {r3-r6}
    bgt .Lnrio_d14_tx_bytes16
    pop {r4-r6}
    bxeq lr

    add r1, r1, #16

.Lnrio_d14_tx_bytes4:
    subs r1, r1, #4
    ldr r3, [r0], #4
    str r3, [r2], #4
    bgt .Lnrio_d14_tx_bytes4
    bx lr

    // EP_DATA -> r0, cannot over-write buffer
BEGIN_ASM_FUNC nrio_d14_rx_bytes itcm
    mov r2, #0x8400000

    cmp r1, #16
    blt .Lnrio_d14_rx_bytes4

    push {r4-r6}
.Lnrio_d14_rx_bytes16:
    subs r1, r1, #16
    ldmiage r2!, {r3-r6}
    stmiage r0!, {r3-r6}
    bgt .Lnrio_d14_rx_bytes16
    pop {r4-r6}
    bxeq lr

    add r1, r1, #16

.Lnrio_d14_rx_bytes4:
    subs r1, r1, #4
    ldrge r3, [r2], #4
    strge r3, [r0], #4
    bgt .Lnrio_d14_rx_bytes4

    // Trick borrowed from JoaoBapt
    movs r1, r1, lsl #31
    ldrhcs r3, [r2], #2
    strhcs r3, [r0], #2
    ldrhmi r3, [r2]
    strbmi r3, [r0]

    bx lr
