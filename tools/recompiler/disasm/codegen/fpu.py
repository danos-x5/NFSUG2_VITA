"""x87 instructions. The register stack lives in cpu.fpu, or in locals where its depth is known (ctx.st)."""

from capstone import x86 as X

from .ctx import T, S


def _index(ctx, op):
    name = ctx.ins.reg_name(op.reg)
    return int(name[-2] if name.endswith(')') else name[-1])


FLOAT_ALIGN = 'off'         # vldr/vstr for 4-aligned float operands: 'off', 'stack' (esp, absolute), 'all' (any base)
FLOAT_ALIGN_CHECK = False   # those helpers check the address and log misaligned sites (development)


def _esp_stays_aligned(fn):
    """Whether the function moves esp only by multiples of 4."""
    cached = getattr(fn, '_esp_aligned', None)
    if cached is None:
        cached = True
        for ins in fn.instructions:
            ops = list(getattr(ins, 'operands', []))
            if not ops or ops[0].type != X.X86_OP_REG or ins.reg_name(ops[0].reg) != 'esp':
                if ins.mnemonic in ('push', 'pop') and ops and ops[0].size == 2:
                    cached = False
                continue
            if ins.mnemonic in ('add', 'sub') and ops[1].type == X.X86_OP_IMM and ops[1].imm % 4:
                cached = False
            if ins.mnemonic == 'lea' and (ops[1].mem.disp % 4 or ops[1].mem.index and ops[1].mem.scale < 4):
                cached = False
        fn._esp_aligned = cached
    return cached


def _aligned(ctx, op):
    """Whether FLOAT_ALIGN treats the operand's address as 4-aligned."""
    m = op.mem
    if FLOAT_ALIGN == 'off' or m.segment or m.disp % 4 or (m.index and m.scale < 4):
        return False
    if not m.base:
        return True
    if ctx.ins.reg_name(m.base) == 'esp':
        return _esp_stays_aligned(ctx.fn)
    return FLOAT_ALIGN == 'all'


def _float_access(ctx, op, kind):
    """Call prefix of the float load or store helper."""
    if not _aligned(ctx, op):
        return 'rc::%s(%s' % (kind, ctx.address(op))
    if FLOAT_ALIGN_CHECK:
        return 'rc::%sc(0x%x, %s' % (kind, ctx.ins.address, ctx.address(op))
    return 'rc::%sa(%s' % (kind, ctx.address(op))


def fval(ctx, op):
    """Operand as an x86::Float rvalue."""
    if op.type == X.X86_OP_REG:
        return ctx.st(_index(ctx, op))
    if op.type == X.X86_OP_MEM:
        a = ctx.address(op)
        if op.size == 4:
            return 'x86::Float(%s))' % _float_access(ctx, op, 'ldf32')
        if op.size == 8:
            return 'x86::Float(%s))' % _float_access(ctx, op, 'ldf64')
        if op.size == 10:
            return 'x86::Float(app->getMemory<x86::IEEEf80>(%s))' % a
    raise AssertionError(ctx.ins.op_str)


def fstore(ctx, op, value):
    if op.type == X.X86_OP_REG:
        return ['%s = x86::Float(%s);' % (ctx.st(_index(ctx, op)), value)]
    a = ctx.address(op)
    if op.size == 4:
        return ['%s, float(%s));' % (_float_access(ctx, op, 'stf32'), value)]
    if op.size == 8:
        return ['%s, double(%s));' % (_float_access(ctx, op, 'stf64'), value)]
    if op.size == 10:
        return ['app->getMemory<x86::IEEEf80>(%s) = x86::Float(%s);' % (a, value)]
    raise AssertionError(ctx.ins.op_str)


def cg_fninit(ctx):
    return ['cpu.fpu.init();'] + ctx.set_fc([])


def cg_fld(ctx, value):
    return ctx.fpush(fval(ctx, value))


def cg_fild(ctx, value):
    bits = value.size * 8
    if bits == 64:
        return ctx.fpush('x86::Float(x86::int64ToDouble(%s(%s)))' % (S(bits), ctx.val(value)))
    return ctx.fpush('x86::Float(%s(%s))' % (S(bits), ctx.val(value)))


def _const(value):
    return lambda ctx: ctx.fpush(value)


cg_fldz = _const('0.0')
cg_fld1 = _const('1.0')
cg_fldpi = _const('3.1415926535897932')
cg_fldl2e = _const('1.4426950408889634')
cg_fldln2 = _const('0.6931471805599453')


def cg_fst(ctx, dst):
    return fstore(ctx, dst, ctx.st(0))


def cg_fstp(ctx, dst):
    return cg_fst(ctx, dst) + ctx.fpop()


def cg_fist(ctx, dst):
    bits = dst.size * 8
    return ctx.put(dst, '%s(x86::fistInteger<%s>(%s))' % (T(bits), S(bits), ctx.rndint()))


def cg_fistp(ctx, dst):
    return cg_fist(ctx, dst) + ctx.fpop()


def cg_frndint(ctx):
    return ['%s = %s;' % (ctx.st(0), ctx.rndint())]


def _arith(op, reverse=False):
    def one_or_two(ctx, *args):
        if len(args) == 1:
            dst, src = ctx.st(0), fval(ctx, args[0])
        else:
            dst, src = fval(ctx, args[0]), fval(ctx, args[1])
        if reverse:
            return ['%s = x86::Float(%s) %s x86::Float(%s);' % (dst, src, op, dst)]
        return ['%s %s= x86::Float(%s);' % (dst, op, src)]
    return one_or_two


def _arith_pop(op, reverse=False):
    def pop(ctx, *args):
        dst = fval(ctx, args[0]) if args else ctx.st(1)
        if reverse:
            return ['%s = %s %s x86::Float(%s);' % (dst, ctx.st(0), op, dst)] + ctx.fpop()
        return ['%s %s= %s;' % (dst, op, ctx.st(0))] + ctx.fpop()
    return pop


def _arith_int(op, reverse=False):
    """fiadd, fisub, ...: st0 op a 16- or 32-bit integer in memory."""
    def integer(ctx, src):
        value = 'x86::Float(%s(%s))' % (S(src.size * 8), ctx.val(src))
        if reverse:
            return ['%s = %s %s x86::Float(%s);' % (ctx.st(0), value, op, ctx.st(0))]
        return ['%s %s= %s;' % (ctx.st(0), op, value)]
    return integer


cg_fadd = _arith('+')
cg_fsub = _arith('-')
cg_fsubr = _arith('-', reverse=True)
cg_fmul = _arith('*')
cg_fdiv = _arith('/')
cg_fdivr = _arith('/', reverse=True)
cg_faddp = _arith_pop('+')
cg_fsubp = _arith_pop('-')
cg_fsubrp = _arith_pop('-', reverse=True)
cg_fmulp = _arith_pop('*')
cg_fdivp = _arith_pop('/')
cg_fdivrp = _arith_pop('/', reverse=True)
cg_fiadd = _arith_int('+')
cg_fisub = _arith_int('-')
cg_fisubr = _arith_int('-', reverse=True)
cg_fimul = _arith_int('*')
cg_fidiv = _arith_int('/')
cg_fidivr = _arith_int('/', reverse=True)


def cg_fxch(ctx, *args):
    other = fval(ctx, args[-1]) if args else ctx.st(1)
    top = ctx.st(0)
    return ['{', '    const x86::Float t = %s;' % top, '    %s = %s;' % (top, other), '    %s = t;' % other, '}']


def _unary(expr):
    return lambda ctx: ['%s = %s;' % (ctx.st(0), expr.format(st0=ctx.st(0), st1=ctx.st(1)))]


cg_fchs = _unary('-{st0}')
cg_fabs = _unary('cpu.fpu.abs({st0})')
cg_fsqrt = _unary('cpu.fpu.sqrt({st0})')
cg_fsin = _unary('cpu.fpu.sin({st0})')
cg_fcos = _unary('cpu.fpu.cos({st0})')
cg_f2xm1 = _unary('cpu.fpu.f2xm1({st0})')
cg_fscale = _unary('cpu.fpu.scale({st0}, {st1})')
def _remainder(function):
    """fprem/fprem1: st0 = remainder of st0 / st1; the runtime sets C2 = 0 (complete) and C0, C3, C1 to the low
    quotient bits."""
    return lambda ctx: ['%s = cpu.fpu.%s(%s, %s);' % (ctx.st(0), function, ctx.st(0), ctx.st(1))] + ctx.set_fc([])


cg_fprem = _remainder('rem')
cg_fprem1 = _remainder('rem1')


def cg_fxam(ctx):
    """Classifies st0 into C3, C2, C0 and sets C1 to its sign (the stack is never reported empty)."""
    return ctx.set_fc(['cpu.fpu.examine(%s);' % ctx.st(0)])


def _environment(store):
    def cg(ctx, op):
        address = ctx.address(op)
        ctx.use_fc()
        if store:
            return ctx.audit_fc() + ['cpu.fpu.storeEnvironment(rc::host(%s));' % address] + ctx.set_fc([])
        return ['cpu.fpu.loadEnvironment(rc::host(%s));' % address] + ctx.set_fc([])
    return cg


cg_fnstenv = _environment(True)
cg_fstenv = cg_fnstenv
cg_fldenv = _environment(False)


def cg_fnsave(ctx, op):
    ctx.use_fc()
    return ctx.audit_fc() + ['cpu.fpu.save(rc::host(%s));' % ctx.address(op)] + ctx.set_fc([])


cg_fsave = cg_fnsave


def cg_frstor(ctx, op):
    return ['cpu.fpu.restore(rc::host(%s));' % ctx.address(op)] + ctx.set_fc([])


def cg_fsincos(ctx):
    """st0 = sin(st0), then push cos of the original value; C2 = 0 (in range)."""
    lines = ['const x86::Float t = %s;' % ctx.st(0), '%s = cpu.fpu.sin(t);' % ctx.st(0)]
    return ['{'] + ['    ' + l for l in lines + ctx.fpush('cpu.fpu.cos(t)')] + ['}'] + ctx.set_fc([])


def cg_ffree(ctx, op):
    """Marks a register empty: the stack model keeps no tags, so nothing changes."""
    return [';']


def cg_fptan(ctx):
    return ['%s = cpu.fpu.tan(%s);' % (ctx.st(0), ctx.st(0))] + ctx.fpush('1.0')


def cg_fpatan(ctx):
    return ['%s = cpu.fpu.atan(%s, %s);' % (ctx.st(1), ctx.st(0), ctx.st(1))] + ctx.fpop()


def cg_fyl2x(ctx):
    return ['%s = cpu.fpu.log2(%s) * %s;' % (ctx.st(1), ctx.st(0), ctx.st(1))] + ctx.fpop()


def _compare(ctx, other, pops):
    lines = ['const double fa = %s;' % ctx.st(0), 'const double fb = %s;' % other]
    sahf = ctx.fn.fused.get(ctx.index)
    if sahf is not None and (ctx.record or ctx.fn.fcmp_saved(sahf)):
        address = ctx.fn.instructions[sahf].address
        lines += ['lzfa_%x = fa;' % address, 'lzfb_%x = fb;' % address]
    lines += ctx.set_fc(['cpu.fpu.compare(fa, fb);'])
    for _ in range(pops):
        lines += ctx.fpop()
    return ['{'] + ['    ' + l for l in lines] + ['}']


def cg_fcom(ctx, *args):
    return _compare(ctx, fval(ctx, args[0]) if args else ctx.st(1), 0)


def cg_fcomp(ctx, *args):
    return _compare(ctx, fval(ctx, args[0]) if args else ctx.st(1), 1)


def cg_fcompp(ctx):
    return _compare(ctx, ctx.st(1), 2)


# Unordered compares differ only in raising #IA for quiet NaNs, which the runtime does not model.
cg_fucom = cg_fcom
cg_fucomp = cg_fcomp
cg_fucompp = cg_fcompp


def cg_ftst(ctx):
    return _compare(ctx, '0.0', 0)


def cg_fnstsw(ctx, dst):
    if not ctx.record and ctx.fn.is_ax_fnstsw(ctx.index) and not ctx.fn.writes_ax(ctx.index):
        return []
    ctx.use_fc()
    check = [] if ctx.record or ctx.fn.masks_condition_codes(ctx.index) else ctx.audit_fc()
    return check + ctx.put(dst, 'cpu.fpu.status.word')


cg_fstsw = cg_fnstsw


def cg_fldcw(ctx, src):
    return ['cpu.fpu.control.word = %s;' % ctx.val(src)]


def cg_fnstcw(ctx, dst):
    return ctx.put(dst, 'cpu.fpu.control.word')


cg_fstcw = cg_fnstcw
