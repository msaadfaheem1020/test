.globl compute_step

# void compute_step(Body *bodies, int n, double dt, double g)
# a0: bodies pointer
# a1: n (number of bodies)
# fa0: dt
# fa1: g (gravitational constant)

compute_step:
    ble     a1, zero, done

    # stack + saved regs (s0 - s9 (10) so 10 * 8 = 80 bytes + 16 byte alignment = 96 bytes)
    addi    sp, sp, -96
    sd      s0, 0(sp)
    sd      s1, 8(sp)
    sd      s2, 16(sp)
    sd      s3, 24(sp)
    sd      s4, 32(sp)
    sd      s5, 40(sp)
    sd      s6, 48(sp)
    sd      s7, 56(sp)
    sd      s8, 64(sp)
    sd      s9, 72(sp)

    li      s0, 96              # struct stride
    la      t0, softening_const
    fld     ft0, 0(t0)          # softening^2

    # reset accel 
    mv      t0, a1
    addi    t1, a0, 48          # accel base
    fmv.d.x ft1, zero

reset_loop:
    vsetvli t2, t0, e64, m1, ta, ma
    vfmv.v.f v0, ft1
    vsse64.v v0, (t1), s0
    addi    t3, t1, 8
    vsse64.v v0, (t3), s0
    addi    t3, t1, 16
    vsse64.v v0, (t3), s0

    mul     t4, t2, s0
    add     t1, t1, t4
    sub     t0, t0, t2
    bnez    t0, reset_loop

    # accel (O(N^2))
    li      s1, 0               # i

outer_loop:
    bge     s1, a1, end_accel

    # load body[i]
    mul     t0, s1, s0
    add     t1, a0, t0
    fld     ft1, 0(t1)          # px
    fld     ft2, 8(t1)          # py
    fld     ft3, 16(t1)         # pz

    fmv.d.x ft4, zero           # ax sum
    fmv.d.x ft5, zero           # ay sum
    fmv.d.x ft6, zero           # az sum

    mv      t0, a1              # j count
    mv      t1, a0              # j ptr
    li      s2, 0               # j index base

inner_j_loop:
    vsetvli t2, t0, e64, m1, ta, ma

    vid.v   v24
    vadd.vx v24, v24, s2
    vmsne.vx v0, v24, s1        # mask j!=i

    # load j
    vlse64.v v1, (t1), s0, v0.t
    addi    t3, t1, 8
    vlse64.v v2, (t3), s0, v0.t
    addi    t3, t1, 16
    vlse64.v v3, (t3), s0, v0.t
    addi    t3, t1, 72
    vlse64.v v4, (t3), s0, v0.t # mass

    # dr
    vfsub.vf v1, v1, ft1, v0.t
    vfsub.vf v2, v2, ft2, v0.t
    vfsub.vf v3, v3, ft3, v0.t

    # dist^2
    vfmul.vv v5, v1, v1, v0.t
    vfmacc.vv v5, v2, v2, v0.t
    vfmacc.vv v5, v3, v3, v0.t
    vfadd.vf v5, v5, ft0, v0.t

    # inv r^3
    vfsqrt.v v6, v5, v0.t
    vfmul.vv v6, v5, v6, v0.t
    vfrdiv.vf v6, v6, fa1, v0.t

    vfmul.vv v6, v6, v4, v0.t   # scale mass

    # accel contrib
    vfmul.vv v7, v1, v6, v0.t
    vfmul.vv v8, v2, v6, v0.t
    vfmul.vv v9, v3, v6, v0.t

    # reduce x
    fmv.d.x ft7, zero
    vfmv.s.f v10, ft7
    vfredusum.vs v11, v7, v10, v0.t
    vfmv.f.s ft8, v11
    fadd.d  ft4, ft4, ft8

    # reduce y
    vfmv.s.f v10, ft7
    vfredusum.vs v11, v8, v10, v0.t
    vfmv.f.s ft8, v11
    fadd.d  ft5, ft5, ft8

    # reduce z
    vfmv.s.f v10, ft7
    vfredusum.vs v11, v9, v10, v0.t
    vfmv.f.s ft8, v11
    fadd.d  ft6, ft6, ft8

    mul     t4, t2, s0
    add     t1, t1, t4
    add     s2, s2, t2
    sub     t0, t0, t2
    bnez    t0, inner_j_loop

    # store accel[i]
    mul     t0, s1, s0
    add     t1, a0, t0
    fsd     ft4, 48(t1)
    fsd     ft5, 56(t1)
    fsd     ft6, 64(t1)

    addi    s1, s1, 1
    j       outer_loop

end_accel:

    # integrate
    mv      t0, a1
    mv      t1, a0

integration_loop:
    vsetvli t2, t0, e64, m1, ta, ma

    # load vel
    addi    t3, t1, 24
    vlse64.v v1, (t3), s0
    addi    t3, t1, 32
    vlse64.v v2, (t3), s0
    addi    t3, t1, 40
    vlse64.v v3, (t3), s0

    # load accel
    addi    t3, t1, 48
    vlse64.v v4, (t3), s0
    addi    t3, t1, 56
    vlse64.v v5, (t3), s0
    addi    t3, t1, 64
    vlse64.v v6, (t3), s0

    # v += a*dt
    vfmacc.vf v1, fa0, v4
    vfmacc.vf v2, fa0, v5
    vfmacc.vf v3, fa0, v6

    # store vel
    addi    t3, t1, 24
    vsse64.v v1, (t3), s0
    addi    t3, t1, 32
    vsse64.v v2, (t3), s0
    addi    t3, t1, 40
    vsse64.v v3, (t3), s0

    # load pos
    addi    t3, t1, 0
    vlse64.v v4, (t3), s0
    addi    t3, t1, 8
    vlse64.v v5, (t3), s0
    addi    t3, t1, 16
    vlse64.v v6, (t3), s0

    # p += v*dt
    vfmacc.vf v4, fa0, v1
    vfmacc.vf v5, fa0, v2
    vfmacc.vf v6, fa0, v3

    # store pos
    addi    t3, t1, 0
    vsse64.v v4, (t3), s0
    addi    t3, t1, 8
    vsse64.v v5, (t3), s0
    addi    t3, t1, 16
    vsse64.v v6, (t3), s0

    mul     t4, t2, s0
    add     t1, t1, t4
    sub     t0, t0, t2
    bnez    t0, integration_loop

done:
    ld      s0, 0(sp)
    ld      s1, 8(sp)
    ld      s2, 16(sp)
    ld      s3, 24(sp)
    ld      s4, 32(sp)
    ld      s5, 40(sp)
    ld      s6, 48(sp)
    ld      s7, 56(sp)
    ld      s8, 64(sp)
    ld      s9, 72(sp)
    addi    sp, sp, 96
    ret

.section .rodata
.align 3 # align to 8 bytes for double
softening_const:
    .double 1e18