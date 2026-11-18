"""Stack frames: esp heights (solving the esp changes of calls), frame address escapes, and the stack slots kept
in C++ locals (generate.py --promote-stack-slots, --separate-stack)."""

from fractions import Fraction

from capstone import x86 as X

from .codegen import ctx as C
from .codegen.integer import stack_slot

ENTRY = 'E'
SAVED = ('ebx', 'esi', 'edi', 'ebp')
ARG_REGS = ('eax', 'ecx', 'edx')
TOP_LOCAL = ('top', 'L')        # a frame address of unknown offset below the return address
TOP_ANY = ('top', '?')          # a stack address of unknown offset
MAX_ROUNDS = 6

# Instructions whose memory operands go through Ctx.val/put/bind or fpu.fval/fstore.
SLOT_MNEMONICS = set('''mov movzx movsx add sub adc sbb and or xor cmp test inc dec neg not shl sal shr sar rol ror
rcr shld shrd bsf bsr xchg imul mul div idiv xadd fld fst fstp fild fist fistp fadd fsub fsubr fmul fdiv fdivr fiadd
fisub fisubr fimul fidiv fidivr fcom fcomp fucom fucomp fldcw fnstcw fstcw fnstsw fstsw push pop leave'''.split())
SLOT_MNEMONICS |= set('set' + cc for cc in C.CC_ALIASES) | set('cmov' + cc for cc in C.CC_ALIASES)
FPU_FLOAT = set('fld fst fstp fadd fsub fsubr fmul fdiv fdivr fcom fcomp fucom fucomp'.split())
STRING = ('movs', 'stos', 'lods', 'scas', 'cmps')


def form(base, off, vars_=frozenset()):
    return ('F', base, off, vars_)


def is_form(v):
    return isinstance(v, tuple) and v[0] == 'F'


def is_top(v):
    return isinstance(v, tuple) and v[0] == 'top'


def is_frame(v):
    return is_form(v) or is_top(v)


def top_of(*values):
    """Unknown frame address; local unless an input reaches the arguments."""
    for v in values:
        if is_form(v) and (v[3] or (v[1] == ENTRY and v[2] >= 0)):
            return TOP_ANY
        if v == TOP_ANY:
            return TOP_ANY
    return TOP_LOCAL


def add(v, k):
    if is_form(v):
        return form(v[1], v[2] + k, v[3])
    if isinstance(v, tuple) and v[0] == 'const':
        return ('const', (v[1] + k) & 0xffffffff)
    if is_top(v):
        return v
    return None


def mem_key(op):
    m = op.mem
    return ('m', m.base, m.index, m.scale, m.disp, op.size)


class Access:
    __slots__ = ('index', 'key', 'base', 'k', 'size', 'read', 'write', 'kind', 'supported', 'save')

    def __init__(self, index, key, base, k, size, read, write, kind, supported, save=None):
        self.index = index
        self.key = key
        self.base = base
        self.k = k
        self.size = size
        self.read = read
        self.write = write
        self.kind = kind            # 'int', 'f32', 'f64'
        self.supported = supported  # the generator can serve it from a local
        self.save = save            # push of a callee-saved register's entry value: its name


class Frame:
    """Analysis result for one function."""
    def __init__(self):
        self.reason = None          # why nothing is promoted
        self.heights = None         # index -> (base, offset) or None
        self.accesses = []
        self.calls = []             # (index, (base, offset) at the call, extent or None, pops or None)
        self.escaped = False        # a frame address may be visible outside (non-save slots are not promoted)
        self.escape_sites = []
        self.floor = {}             # base -> height where the prologue ends
        self.extent = 4             # bytes above the entry esp this function and its callees may access
        self.unbounded = False      # its extent is unknown (callers apply the argument assumption)
        self.slots = {}             # (base, k) -> Slot
        self.sites = {}             # (index, key) -> Slot
        self.stack_sites = set()    # (index, key) of accesses that alias no other guest access (separate_stack)
        self.escaped_at = set()     # (base, offset) of frame addresses that escaped
        self.escaped_unknown = False    # a frame address of unknown offset escaped
        self.stack_limit = 0        # separate_stack bytes end at the entry esp plus this
        self.top_accesses = 0
        self.continued = []         # heights of jumps into other functions that continue this frame
        self.prologue_end = None    # index of the first instruction after the prologue
        self.entry_ebp = False      # dereferences the caller's ebp (a frame walk, or code continuing a frame)
        self.escaped_args = False   # an address at or above the return address may be visible outside
        self.ebp_calls = []         # (callee, ebp holds a frame address, ebp holds the entry value)
        self._dom = None

    def dominators(self, fn):
        """Per instruction: bit set of its dominators."""
        if self._dom is None:
            n = fn.n
            full = (1 << n) - 1
            dom = [full] * n
            dom[fn.entry] = 1 << fn.entry
            order = sorted(fn.reachable)
            changed = True
            while changed:
                changed = False
                for i in order:
                    if i == fn.entry:
                        continue
                    d = full
                    for p in fn.pred[i]:
                        if p in fn.reachable:
                            d &= dom[p]
                    d |= 1 << i
                    if d != dom[i]:
                        dom[i] = d
                        changed = True
            self._dom = dom
        return self._dom


class Slot:
    def __init__(self, base, k, size, ctype, name, save):
        self.base = base
        self.k = k
        self.size = size
        self.ctype = ctype          # C++ type of the local
        self.name = name
        self.save = save
        self.entry_load = False     # read before written on some path: loaded from guest memory at entry
        self.arg = False            # an incoming argument: writes also go to guest memory
        self.load_site = None       # (index, key) of the read that loads it from guest memory


class Solver:
    """Linear equations over the bytes calls pop, which are at least 0."""
    def __init__(self, known):
        self.known = known          # unknown -> int (program-wide and solved)
        self.shared = {}            # call site -> the unknown it stands for (an import slot, a callee, itself)
        self.rows = []
        self.ineqs = []             # (a, b): form a <= form b

    def lookup(self, site):
        return self.known.get(self.shared.get(site, site))

    def value(self, f):
        """The form with known unknowns substituted."""
        if not is_form(f) or not f[3]:
            return f
        off = f[2]
        rest = []
        for v in f[3]:
            k = self.lookup(v)
            if k is not None:
                off += k
            else:
                rest.append(v)
        return form(f[1], off, frozenset(rest))

    def linear(self, a, b):
        """a - b as (coefficients, constant)."""
        a, b = self.value(a), self.value(b)
        coeffs = {}
        for v in a[3]:
            v = self.shared.get(v, v)
            coeffs[v] = coeffs.get(v, 0) + 1
        for v in b[3]:
            v = self.shared.get(v, v)
            coeffs[v] = coeffs.get(v, 0) - 1
        return dict((v, coeffs[v]) for v in sorted(coeffs, key=repr) if coeffs[v]), a[2] - b[2]

    def at_most(self, a, b):
        """Records a <= b: a callee pops at most what was pushed."""
        if is_form(a) and is_form(b) and a[1] == b[1]:
            self.ineqs.append((a, b))

    def violated(self, values=None):
        """Whether a fully known inequality fails."""
        for a, b in self.ineqs:
            coeffs, const = self.linear(a, b)
            if values is not None:
                const += sum(c * values[v] for v, c in coeffs.items() if v in values)
                coeffs = dict((v, c) for v, c in coeffs.items() if v not in values)
            if not coeffs and const > 0:
                return True
        return False

    def bounds(self):
        """Unknowns an inequality bounds to 0; False on a contradiction."""
        changed = False
        for a, b in self.ineqs:
            coeffs, const = self.linear(a, b)
            if len(coeffs) == 1:
                (v, c), = coeffs.items()
                if c > 0 and const >= 0:
                    if const > 0:
                        return None
                    self.known[v] = 0
                    changed = True
        return changed

    def equal(self, a, b):
        a, b = self.value(a), self.value(b)
        coeffs = {}
        for v in a[3]:
            v = self.shared.get(v, v)
            coeffs[v] = coeffs.get(v, 0) + 1
        for v in b[3]:
            v = self.shared.get(v, v)
            coeffs[v] = coeffs.get(v, 0) - 1
        coeffs = dict((v, coeffs[v]) for v in sorted(coeffs, key=repr) if coeffs[v])
        rhs = b[2] - a[2]
        if not coeffs:
            return rhs == 0
        self.rows.append((coeffs, rhs))
        return True

    def solve(self):
        """Eliminates; False when the equations contradict."""
        rows = [(dict((v, Fraction(c)) for v, c in r.items()), Fraction(rhs)) for r, rhs in self.rows]
        progress = True
        while progress:
            progress = False
            b = self.bounds()
            if b is None:
                return False
            progress = b
            rows = [self.substitute(r, rhs) for r, rhs in rows]
            reduced = []
            for r, rhs in rows:
                for pivot, (pr, prhs) in reduced:
                    if pivot in r:
                        f = r[pivot] / pr[pivot]
                        for v, c in pr.items():
                            r[v] = r.get(v, 0) - f * c
                        rhs -= f * prhs
                        r = dict((v, r[v]) for v in sorted(r, key=repr) if r[v])
                if not r:
                    if rhs != 0:
                        return False
                    continue
                reduced.append((min(r, key=repr), (r, rhs)))
            rows = [x[1] for x in reduced]
            for r, rhs in rows:
                if len(r) == 1:
                    (v, c), = r.items()
                    value = rhs / c
                    if value.denominator != 1 or value < 0 or int(value) % 4:
                        return False
                    self.known[v] = int(value)
                    progress = True
                elif all(c > 0 for c in r.values()) or all(c < 0 for c in r.values()):
                    s = rhs if all(c > 0 for c in r.values()) else -rhs
                    if s < 0:
                        return False
                    if s == 0:
                        for v in r:
                            self.known[v] = 0
                        progress = True
        self.rows = [(dict((v, int(c)) for v, c in r.items()), int(rhs)) for r, rhs in rows
                     if all(c.denominator == 1 for c in r.values()) and rhs.denominator == 1]
        return not self.violated()

    def substitute(self, r, rhs):
        out = {}
        for v, c in r.items():
            if v in self.known:
                rhs -= c * self.known[v]
            else:
                out[v] = c
        return out, rhs


class Walker:
    """One forward pass over a function with the current solution."""

    def __init__(self, program, fn, known, extents, final):
        self.program = program
        self.fn = fn
        self.solver = Solver(known)
        self.extents = extents
        self.final = final
        self.frame = Frame()
        self.vars = set()
        self.pushed = {}            # unknown of an indirect call -> bytes pushed right before it
        self.pending = None
        self.prologue = prologue_end(fn)
        self.exits = []             # (kind, esp form, ret immediate or tail target)
        self.current = None

    def fail(self, why):
        if self.frame.reason is None:
            self.frame.reason = why
            if self.current is not None and ' 0x' not in why:
                self.frame.reason += ' 0x%x' % self.fn.instructions[self.current].address

    # ---- values ---------------------------------------------------------------------------------------------
    def reg_value(self, regs, ins, reg):
        name = ins.reg_name(reg)
        if name in C.REGS:
            return regs[name]
        if name in C.SUBREGS:
            full = C.SUBREGS[name][0]
            return TOP_LOCAL if is_frame(regs[full]) else None
        return None

    def address(self, regs, ins, op, esp_override=None):
        """Form, TOP, 'segment' or None for a memory operand."""
        m = op.mem
        if m.segment:
            return 'segment'
        base = None
        if self.final and ('orig', 'ebp') in [regs.get(ins.reg_name(r)) for r in (m.base, m.index) if r]:
            self.frame.entry_ebp = True
        if m.base:
            name = ins.reg_name(m.base)
            base = esp_override if name == 'esp' and esp_override is not None else self.reg_value(regs, ins, m.base)
        index = self.reg_value(regs, ins, m.index) if m.index else None
        if is_form(base) and not m.index:
            return add(base, m.disp)
        if is_frame(base) or is_frame(index):
            return top_of(*(v for v in (base, index) if is_frame(v)))
        return None

    def value_of(self, regs, ins, op):
        if op.type == X.X86_OP_REG:
            return self.reg_value(regs, ins, op.reg)
        if op.type == X.X86_OP_IMM:
            return ('const', op.imm & 0xffffffff)
        return None

    # ---- recording ------------------------------------------------------------------------------------------
    def record(self, i, key, addr, size, read, write, kind, supported, save=None):
        if not self.final:
            return
        if is_top(addr):
            self.frame.top_accesses += 1
            if addr == TOP_ANY:
                self.frame.unbounded = True
            self.escape(i, 'indexed frame access', addr)
            return
        addr = self.solver.value(addr)
        if addr[3]:
            self.fail('access at an unknown height 0x%x' % self.fn.instructions[i].address)
            return
        self.frame.accesses.append(Access(i, key, addr[1], addr[2], size, read, write, kind, supported, save))

    def escape(self, i, why, value=None):
        if self.final:
            v = self.solver.value(value) if is_form(value) else None
            if v is not None and not v[3]:
                self.frame.escaped_at.add((v[1], v[2]))
                if v[1] == ENTRY and v[2] >= 0:
                    self.frame.escaped_args = True
            else:
                self.frame.escaped_unknown = True
                if value != TOP_LOCAL:
                    self.frame.escaped_args = True
            self.frame.escaped = True
            if len(self.frame.escape_sites) < 4:
                self.frame.escape_sites.append((self.fn.instructions[i].address, why))

    def check_regs_escape(self, i, regs, names, why):
        for r in names:
            v = regs[r]
            if is_frame(v):
                w = self.solver.value(v) if is_form(v) else v
                if w == TOP_ANY or (is_form(w) and (w[3] or (w[1] == ENTRY and w[2] >= 0))):
                    self.frame.unbounded = True
                self.escape(i, '%s in %s' % (why, r), v)

    def esp_known(self, regs, i):
        esp = regs['esp']
        if not is_form(esp):
            self.fail('esp unknown at 0x%x' % self.fn.instructions[i].address)
            return None
        return esp

    # ---- one instruction ------------------------------------------------------------------------------------
    def step(self, i, state):
        fn = self.fn
        self.current = i
        ins = fn.instructions[i]
        regs = dict(state)
        kind = fn.branch[i][0]
        mnemonic = ins.mnemonic.split()[-1]
        prefixed = len(ins.mnemonic.split()) > 1
        operands = list(getattr(ins, 'operands', []))
        if kind == 'skip':
            self.fail('skipped instruction')
            return regs
        esp = regs['esp']
        supported = mnemonic in SLOT_MNEMONICS and not prefixed

        x87_store = mnemonic in ('fst', 'fstp', 'fist', 'fistp', 'fisttp', 'fnstsw', 'fstsw', 'fnstcw', 'fstcw',
                                 'fnstenv', 'fstenv', 'fnsave', 'fsave', 'fbstp')
        for op in operands:
            if op.type != X.X86_OP_MEM or mnemonic == 'lea':
                continue
            override = add(esp, stack_slot(ins)) if mnemonic == 'pop' and is_form(esp) else None
            a = self.address(regs, ins, op, override)
            if a == 'segment':
                if ins.reg_name(op.mem.segment) == 'fs':
                    self.fail('fs access')
                continue
            if a is None:
                continue
            write = bool(op.access & 2) or x87_store
            read = bool(op.access & 1) and not x87_store
            if mnemonic in ('fnstenv', 'fstenv', 'fnsave', 'fsave', 'frstor', 'fldenv'):
                read = write = True
            k = 'int'
            if mnemonic in FPU_FLOAT:
                k = 'f32' if op.size == 4 else 'f64' if op.size == 8 else 'other'
            ok = supported and op.size in (1, 2, 4, 8) and k != 'other'
            if mnemonic in ('bt', 'bts', 'btr', 'btc') and operands[1].type != X.X86_OP_IMM:
                ok = False
            if op.size == 8 and k == 'int' and mnemonic not in ('fild', 'fistp'):
                ok = False
            if mnemonic == 'fldcw' or mnemonic in ('fnstcw', 'fstcw', 'fnstsw', 'fstsw'):
                ok = ok and op.size == 2
            self.record(i, mem_key(op), a, op.size, read, write, k, ok)

        writes_memory = any(op.type == X.X86_OP_MEM and (op.access & 2) for op in operands) or mnemonic == 'push'
        if writes_memory or mnemonic == 'xchg':
            for op in operands:
                value = self.value_of(regs, ins, op) if op.type == X.X86_OP_REG else None
                if is_frame(value) and mnemonic != 'pop':
                    self.escape(i, '%s of a frame address' % mnemonic, value)
        if mnemonic in STRING or mnemonic in ('pushal', 'pushfd', 'pushf', 'enter', 'xlatb'):
            for r in ('esi', 'edi', 'ebx'):
                if is_frame(regs[r]):
                    self.escape(i, 'string op on the frame', regs[r])
            if mnemonic in STRING and is_frame(regs['eax']) and mnemonic == 'stos':
                self.escape(i, 'stos of a frame address')
            if mnemonic in ('pushal', 'enter'):
                self.fail(mnemonic)

        if kind in ('call', 'dyncall'):
            return self.call(i, regs, kind)
        if mnemonic == 'push':
            e = self.esp_known(regs, i)
            if e is None:
                return regs
            size = stack_slot(ins)
            op = operands[0]
            value = self.value_of(regs, ins, op)
            save = None
            if op.type == X.X86_OP_REG and op.size == 4:
                name = ins.reg_name(op.reg)
                if name in SAVED and value == ('orig', name):
                    save = name
            self.record(i, ('push',), add(e, -size), size, False, True, 'int', not prefixed and size in (2, 4), save)
            regs['esp'] = add(e, -size)
            return regs
        if mnemonic == 'pop':
            e = self.esp_known(regs, i)
            if e is None:
                return regs
            size = stack_slot(ins)
            self.record(i, ('pop',), e, size, True, False, 'int', not prefixed and size in (2, 4))
            regs['esp'] = add(e, size)
            op = operands[0]
            if op.type == X.X86_OP_REG:
                name = ins.reg_name(op.reg)
                full = C.SUBREGS.get(name, (name,))[0]
                if full == 'esp':
                    regs['esp'] = None
                    self.fail('pop esp')
                elif full in C.REGS:
                    regs[full] = None
            return regs
        if mnemonic == 'leave':
            ebp = regs['ebp']
            if not is_form(ebp):
                self.fail('leave without a frame pointer')
                regs['esp'] = None
                return regs
            self.record(i, ('leave',), ebp, 4, True, False, 'int', True)
            regs['esp'] = add(ebp, 4)
            regs['ebp'] = None
            return regs
        if mnemonic in ('ret', 'retn'):
            self.exit(i, regs)
            self.exits.append(('ret', esp, operands[0].imm if operands else 0))
            if is_form(esp):
                if not self.solver.equal(esp, form(ENTRY, 0)):
                    self.fail('ret at a known height other than 0')
            else:
                self.fail('ret with esp unknown')
            return regs
        for x in fn.exits[i]:
            if x.kind in ('tail', 'dyntail'):
                self.exit(i, regs)
                if x.kind == 'tail':
                    self.callee_regs_escape(i, regs, x.target)
                self.exits.append((x.kind, esp, x.target))
                if not is_form(esp):
                    self.fail('tail jump with esp unknown')
                elif self.final:
                    e = self.solver.value(esp)
                    if e[3]:
                        self.fail('tail jump at an unknown height')
                    elif (e[1], e[2]) != (ENTRY, 0):
                        self.frame.continued.append((e[1], e[2]))
                    elif x.kind == 'tail':
                        self.frame.ebp_calls.append((x.target, is_frame(regs['ebp']), regs['ebp'] == ('orig', 'ebp')))

        dst = operands[0] if operands else None
        dst_reg = None
        if dst is not None and dst.type == X.X86_OP_REG:
            dst_reg = ins.reg_name(dst.reg)
        full = C.SUBREGS.get(dst_reg, (None, 0))[0] if dst_reg else None
        bits = C.SUBREGS[dst_reg][1] if dst_reg in C.SUBREGS else 0

        if mnemonic == 'mov' and full and bits == 32:
            src = operands[1]
            regs[full] = self.value_of(regs, ins, src) if src.type != X.X86_OP_MEM else None
            if src.type == X.X86_OP_REG and ins.reg_name(src.reg) not in C.REGS:
                regs[full] = None
            if full == 'esp' and not is_form(regs['esp']):
                self.fail('mov esp from an unknown value')
            return regs
        if mnemonic == 'lea' and full and bits == 32:
            a = self.address(regs, ins, operands[1])
            regs[full] = a if a != 'segment' else None
            if full == 'esp' and not is_form(regs['esp']):
                self.fail('lea esp unknown')
            return regs
        if mnemonic in ('add', 'sub') and full and bits == 32:
            src = operands[1]
            cur = regs[full]
            if src.type == X.X86_OP_IMM:
                imm = src.imm if mnemonic == 'add' else -src.imm
                regs[full] = add(cur, imm) if (is_frame(cur) or (isinstance(cur, tuple) and cur[0] == 'const')) \
                    else None
            else:
                other = self.value_of(regs, ins, src) if src.type == X.X86_OP_REG else None
                if mnemonic == 'sub' and is_frame(cur) and is_frame(other):
                    regs[full] = None
                elif is_frame(cur) or is_frame(other):
                    regs[full] = top_of(*(v for v in (cur, other) if is_frame(v)))
                else:
                    regs[full] = None
            if full == 'esp' and not is_form(regs['esp']):
                self.fail('esp changed by an unknown amount at 0x%x' % ins.address)
            return regs
        if mnemonic in ('inc', 'dec') and full and bits == 32:
            regs[full] = add(regs[full], 1 if mnemonic == 'inc' else -1) if is_frame(regs[full]) else None
            return regs
        if mnemonic == 'and' and full == 'esp' and operands[1].type == X.X86_OP_IMM and is_form(esp):
            regs['esp'] = form(('A', i), 0)
            if self.final:
                self.frame.floor.setdefault(('A', i), 0)
            return regs
        if mnemonic == 'xchg' and full and operands[1].type == X.X86_OP_REG and bits == 32:
            other = ins.reg_name(operands[1].reg)
            if other in C.REGS:
                regs[full], regs[other] = regs[other], regs[full]
                return regs
        e = fn.effects[i]
        sources = []
        for op in operands:
            if op.type == X.X86_OP_REG and op.access & 1:
                sources.append(self.reg_value(regs, ins, op.reg))
        for r in getattr(ins, 'regs_read', ()):
            name = ins.reg_name(r)
            if name in C.SUBREGS and name not in ('esp', 'sp'):
                sources.append(regs[C.SUBREGS[name][0]])
        frame_src = [v for v in sources if is_frame(v)]
        written = e.reg_def | e.reg_partial
        for r in C.REGS:
            if written & C.REG_BIT[r]:
                regs[r] = top_of(*frame_src) if frame_src else None
        if written & C.REG_BIT['esp']:
            self.fail('esp written by %s at 0x%x' % (mnemonic, ins.address))
        return regs

    def callee_regs_escape(self, i, regs, target):
        live = (self.program.live_in.get(target, -1) >> 8) & C.ALL_REGS
        names = [r for r in C.REGS if r not in ('esp', 'ebp') and live & C.REG_BIT[r]]
        self.check_regs_escape(i, regs, names, 'call argument')

    def exit(self, i, regs):
        self.check_regs_escape(i, regs, [r for r in C.REGS if r != 'esp'], 'exit')

    def call(self, i, regs, kind):
        fn = self.fn
        ins = fn.instructions[i]
        esp = self.esp_known(regs, i)
        if esp is None:
            return regs
        target = fn.branch[i][1] if kind == 'call' else None
        info = self.program.stack.get(target) if target is not None else None
        if kind == 'call':
            self.callee_regs_escape(i, regs, target)
            if info and info.skips:
                self.fail('callee returns from its caller too')
        else:
            self.check_regs_escape(i, regs, ARG_REGS, 'indirect call argument')
        out = dict(regs)
        pops = None
        var = None
        if target == CHKSTK:
            eax = regs['eax']
            if isinstance(eax, tuple) and eax[0] == 'const' and eax[1] < 0x1000000:
                out['esp'] = add(esp, -eax[1])
            else:
                self.fail('_chkstk with an unknown size')
                out['esp'] = None
        elif info is not None and info.delta is not None:
            pops = info.delta - 4
            out['esp'] = add(esp, pops)
        else:
            var = ('call', fn.entry_address, i)
            key = var
            if kind == 'call':
                key = ('fn', target)
            else:
                op = ins.operands[0]
                if op.type == X.X86_OP_MEM and not op.mem.base and not op.mem.index and \
                        (op.mem.disp & 0xffffffff) in self.program.import_slots:
                    key = ('imp', op.mem.disp & 0xffffffff)
                else:
                    self.pushed[var] = self.pushed_before(i) + (method_call(fn, i),)
            self.solver.shared[var] = key
            self.vars.add(key)
            out['esp'] = form(esp[1], esp[2], esp[3] | {var})
        if is_form(out['esp']) and is_form(regs.get('base')) and target != CHKSTK:
            self.solver.at_most(out['esp'], regs['base'])
        if self.final:
            if target is not None:
                self.frame.ebp_calls.append((target, is_frame(regs['ebp']), regs['ebp'] == ('orig', 'ebp')))
            e = self.solver.value(esp)
            height = (e[1], e[2]) if not e[3] else None
            if var is not None and self.solver.lookup(var) is not None:
                pops = self.solver.lookup(var)
            cleanup = self.cleanup(i) if pops == 0 else None
            self.frame.calls.append((i, height, target, pops, cleanup))
        if kind == 'call' and target != CHKSTK:
            changed = (fn.callee_may_def(i) >> 8) & C.ALL_REGS
            if info:
                changed &= ~info.preserved
            keep = SAVED
        else:
            changed = C.ALL_REGS
            keep = SAVED if kind == 'dyncall' else ()
            if target == CHKSTK:
                keep = tuple(r for r in C.REGS if r != 'eax')
        for r in C.REGS:
            if r != 'esp' and changed & C.REG_BIT[r] and r not in keep:
                out[r] = None
        return out

    def pushed_before(self, i):
        """(bytes pushed since the prologue or last esp change, agreeing)."""
        if self.pending is None:
            self.pending = pending_pushes(self.fn, self.prologue)
        p, clean = pushed_at(self.fn, self.pending, i)
        return p, clean and i != self.prologue

    def cleanup(self, i):
        """Bytes an add esp, imm right after call i removes."""
        fn = self.fn
        if i + 1 >= fn.n or fn.is_label(i + 1):
            return None
        nxt = fn.instructions[i + 1]
        if nxt.mnemonic == 'add' and nxt.operands[0].type == X.X86_OP_REG and \
                nxt.reg_name(nxt.operands[0].reg) == 'esp' and nxt.operands[1].type == X.X86_OP_IMM:
            return nxt.operands[1].imm
        return None

    # ---- joins ----------------------------------------------------------------------------------------------
    def merge(self, old, new):
        if old is None:
            return dict(new)
        out = {'base': old['base'] if old['base'] == new['base'] else None}
        for r in C.REGS:
            a, b = old[r], new[r]
            if a == b:
                out[r] = a
            elif is_form(a) and is_form(b) and a[1] == b[1]:
                va, vb = self.solver.value(a), self.solver.value(b)
                if va == vb:
                    out[r] = a
                elif r == 'esp' and va[3] != vb[3]:
                    if not self.solver.equal(a, b):
                        self.fail('stack heights disagree at a join')
                    out[r] = a
                elif r == 'esp':
                    self.fail('stack heights disagree at a join')
                    out[r] = None
                else:
                    out[r] = top_of(a, b)
            elif is_frame(a) or is_frame(b):
                out[r] = top_of(*(v for v in (a, b) if is_frame(v))) if r != 'esp' else None
                if r == 'esp':
                    self.fail('esp unknown at a join')
            else:
                out[r] = None
        return out

    def run(self):
        fn = self.fn
        n = fn.n
        state_in = [None] * n
        entry = dict((r, ('orig', r)) for r in C.REGS)
        entry['esp'] = form(ENTRY, 0)
        entry['base'] = None
        state_in[fn.entry] = entry
        work = [fn.entry]
        seen = set()
        while work:
            i = work.pop()
            seen.add(i)
            state = state_in[i]
            if i == self.prologue:
                state = dict(state, base=state['esp'])
            out = self.step(i, state)
            if fn.effects[i].reg_def & C.REG_BIT['esp'] and out['esp'] != state['esp'] and \
                    fn.instructions[i].mnemonic.split()[-1] in ('mov', 'lea', 'and', 'leave'):
                out['base'] = out['esp']
            elif is_form(out.get('base')) and is_form(out['esp']) and out['base'][1] == out['esp'][1]:
                coeffs, above = self.solver.linear(out['esp'], out['base'])
                if not coeffs and above > 0:
                    out['base'] = out['esp']
            for s in fn.succ[i]:
                merged = self.merge(state_in[s], out)
                if merged != state_in[s]:
                    state_in[s] = merged
                    work.append(s)
        if self.final:
            heights = {}
            for i in range(n):
                st = state_in[i]
                e = self.solver.value(st['esp']) if st and is_form(st['esp']) else None
                heights[i] = (e[1], e[2]) if e and not e[3] else None
            self.frame.heights = heights
            self.frame.floor.update(self.prologue_floor(state_in))
        return state_in

    def prologue_floor(self, state_in):
        """Per base: the height where the prologue ends."""
        fn = self.fn
        i = fn.entry
        floors = {}
        regs = state_in[i]
        while regs is not None and i < self.prologue:
            ins = fn.instructions[i]
            m = ins.mnemonic.split()[-1]
            esp = self.solver.value(regs['esp']) if is_form(regs['esp']) else None
            if esp is None or esp[3]:
                break
            ops = ins.operands
            if m == 'push':
                op = ops[0]
                if op.type == X.X86_OP_REG and op.size == 4 and regs.get(ins.reg_name(op.reg)) == \
                        ('orig', ins.reg_name(op.reg)):
                    floors[esp[1]] = esp[2] - 4
                else:
                    break
            elif m == 'sub' and ops[0].type == X.X86_OP_REG and ins.reg_name(ops[0].reg) == 'esp' and \
                    ops[1].type == X.X86_OP_IMM:
                floors[esp[1]] = esp[2] - ops[1].imm
            elif m == 'and' and ops[0].type == X.X86_OP_REG and ins.reg_name(ops[0].reg) == 'esp':
                floors[('A', i)] = 0
            elif fn.branch[i][0] == 'call' and fn.branch[i][1] == CHKSTK:
                nxt = state_in[i + 1] if i + 1 < fn.n else None
                e2 = self.solver.value(nxt['esp']) if nxt and is_form(nxt['esp']) else None
                if e2 is None or e2[3]:
                    break
                floors[e2[1]] = e2[2]
            elif fn.branch[i][0] != 'next' or len(fn.succ[i]) != 1:
                break
            i += 1
            regs = state_in[i] if i < fn.n else None
        self.frame.prologue_end = i
        floors.setdefault(ENTRY, 0)
        return floors


CHKSTK = 0x75c4d0
# Reasons that mean a callee moves esp or ebp, or reads its caller's frame: callers keep their frames in memory.
FRAME_HELPER = ('esp changed by an unknown amount', 'leave without a frame pointer', 'ret at a known height other than',
                'mov esp from an unknown value', 'lea esp unknown', 'callee returns from its caller too', 'pop esp',
                'ret with esp unknown', 'tail jump with esp unknown')


def walk(program, fn, known, extents, final):
    w = Walker(program, fn, known, extents, final)
    try:
        w.run()
    except RecursionError:
        w.fail('recursion')
    return w


def method_call(fn, i):
    """call [[s] + n] with s in ecx or pushed last."""
    op = fn.instructions[i].operands[0]
    if op.type != X.X86_OP_MEM or not op.mem.base or op.mem.index:
        return False
    ins = fn.instructions[i]
    r = ins.reg_name(op.mem.base)
    if r not in C.REGS or r == 'esp':
        return False
    last_push = None
    j = i - 1
    while j >= 0 and fn.succ[j] == [j + 1] and fn.branch[j][0] == 'next':
        p = fn.instructions[j]
        m = p.mnemonic.split()[-1]
        written = fn.effects[j].reg_def | fn.effects[j].reg_partial
        if m == 'push' and last_push is None:
            o = p.operands[0]
            last_push = p.reg_name(o.reg) if o.type == X.X86_OP_REG else ''
        if written & C.REG_BIT[r]:
            src = p.operands[1] if m == 'mov' and len(p.operands) == 2 else None
            if src is None or src.type != X.X86_OP_MEM or not src.mem.base or src.mem.index or src.mem.disp:
                return False
            obj = p.reg_name(src.mem.base)
            for k in range(j + 1, i):
                if (fn.effects[k].reg_def | fn.effects[k].reg_partial) & C.REG_BIT.get(obj, 0) and obj != 'esp':
                    return False
            return obj == 'ecx' or obj == last_push
        if fn.is_label(j):
            return False
        j -= 1
    return False


def prologue_op(fn, i, written, strict):
    """Part of the prologue: entry-value pushes, frame set-up, then plain code."""
    ins = fn.instructions[i]
    m = ins.mnemonic.split()[-1]
    ops = ins.operands
    if fn.branch[i][0] == 'call':
        return fn.branch[i][1] == CHKSTK
    if fn.branch[i][0] != 'next' or len(fn.succ[i]) != 1:
        return False
    if m == 'push':
        name = ins.reg_name(ops[0].reg) if ops[0].type == X.X86_OP_REG else None
        return name in (C.REGS if strict else SAVED) and not written & C.REG_BIT[name]
    return strict or not (fn.effects[i].reg_def | fn.effects[i].reg_partial) & C.REG_BIT['esp']


def prologue_end(fn):
    """Index of the first push or call after the prologue."""
    i = fn.entry
    written = 0
    strict = True
    while i < fn.n and (i == fn.entry or not fn.is_label(i)) and prologue_op(fn, i, written, strict):
        ins = fn.instructions[i]
        if ins.mnemonic != 'push' and (any(op.type == X.X86_OP_MEM for op in ins.operands) and ins.mnemonic != 'lea'
                                       or ins.mnemonic not in ('mov', 'sub', 'and', 'xor', 'lea')):
            strict = False
        written |= fn.effects[i].reg_def | fn.effects[i].reg_partial
        i += 1
    return i


def pushed_at(fn, pending, i):
    """(bytes pushed before call i, whether every path agrees)."""
    p = pending[i]
    if p is not None and p != '?':
        return p, True
    total = 0
    j = i - 1
    while j >= 0 and fn.succ[j] == [j + 1] and fn.branch[j][0] == 'next':
        ins = fn.instructions[j]
        if ins.mnemonic.split()[-1] == 'push':
            total += stack_slot(ins)
        elif fn.effects[j].reg_def & C.REG_BIT['esp'] or fn.effects[j].reg_partial & C.REG_BIT['esp']:
            break
        if fn.is_label(j):
            break
        j -= 1
    return total, False


def pending_pushes(fn, reset=None):
    """Per instruction: bytes pushed since the last esp change (or reset)."""
    state = [None] * fn.n
    state[fn.entry] = 0
    work = [fn.entry]
    while work:
        i = work.pop()
        if i == reset:
            state[i] = 0
        v = state[i]
        ins = fn.instructions[i]
        m = ins.mnemonic.split()[-1]
        if fn.branch[i][0] in ('call', 'dyncall'):
            v = 0
        elif m == 'push':
            v = v + stack_slot(ins) if v != '?' else v
        elif m == 'sub' and ins.operands[0].type == X.X86_OP_REG and ins.reg_name(ins.operands[0].reg) == 'esp' \
                and ins.operands[1].type == X.X86_OP_IMM:
            v = v + ins.operands[1].imm if v != '?' else v
        elif fn.effects[i].reg_def & C.REG_BIT['esp'] or fn.effects[i].reg_partial & C.REG_BIT['esp']:
            v = 0
        for s in fn.succ[i]:
            old = state[s]
            new = v if old is None or s == reset else (old if old == v else '?')
            if new != old:
                state[s] = new
                work.append(s)
    return state


def solve_function(program, fn, known):
    """(solution, unknowns, rows or None, chosen unknowns, last walker)."""
    local = dict(known)
    chosen = set()
    picked = {}
    w = None
    for _ in range(MAX_ROUNDS):
        w = walk(program, fn, local, {}, False)
        before = dict(local)
        if not w.solver.solve():
            if not picked:
                return local, w.vars, None, chosen, w
            chosen -= set(picked)
            for v in picked:
                before.pop(v, None)
            local = before
            w = walk(program, fn, local, {}, False)
            w.solver.solve()
            break
        local = w.solver.known
        picked = {}
        if local == before:
            picked = dict((v, x) for v, x in choose(w.solver.rows, w.pushed, w.solver.violated).items()
                          if v not in local)
            if not picked:
                break
            local.update(picked)
            chosen |= set(picked)
    return local, w.vars, w.solver.rows, chosen, w


def own_pops(w, known, stack):
    """Bytes the function pops besides its return address."""
    values = set()
    for kind, esp, extra in w.exits:
        e = w.solver.value(esp) if is_form(esp) else None
        if e is None or e[3] or e[1] != ENTRY:
            return None
        if kind == 'ret' and e[2] == 0:
            values.add(extra)
        elif kind == 'tail':
            info = stack.get(extra)
            t = info.delta - 4 if info and info.delta is not None else known.get(('fn', extra))
            if t is None:
                return None
            values.add(e[2] + t)
        else:
            return None
    fn = w.fn
    for i in fn.reachable:
        for x in fn.exits[i]:
            if x.edge != 'call' and x.kind == 'ret' and fn.instructions[i].mnemonic.split()[-1] != 'ret':
                return None
    return values.pop() if len(values) == 1 else None


def choose(rows, pushed, violated=lambda values: False, budget=20000):
    """Settles open indirect calls: each pops 0 or its pushes."""
    out = {}
    in_rows = set(v for r, _ in rows for v in r)
    for v, (p, clean, method) in pushed.items():
        if v not in in_rows and clean and (p == 0 or method) and not violated({v: p}):
            out[v] = p
    open_ = sorted(in_rows, key=repr)
    if not open_ or any(v not in pushed for v in open_):
        return out
    options = dict((v, sorted(set((0, pushed[v][0])))) for v in open_)

    def fits(values):
        return all(sum(c * values[v] for v, c in r.items()) == rhs for r, rhs in rows)

    found = []
    nodes = [0]
    values = {}

    def feasible():
        for r, rhs in rows:
            lo = hi = 0
            for v, c in r.items():
                if v in values:
                    lo += c * values[v]
                    hi += c * values[v]
                else:
                    a, b = c * options[v][0], c * options[v][-1]
                    lo += min(a, b)
                    hi += max(a, b)
            if not lo <= rhs <= hi:
                return False
        return True

    def search(j):
        nodes[0] += 1
        if len(found) > 1 or nodes[0] > budget:
            return
        if not feasible():
            return
        if j == len(open_):
            if not violated(values):
                found.append(dict(values))
            return
        for o in options[open_[j]]:
            values[open_[j]] = o
            search(j + 1)
        del values[open_[j]]

    search(0)
    if nodes[0] <= budget:
        if len(found) == 1:
            out.update(found[0])
        return out
    callee_pops = dict((v, pushed[v][0]) for v in open_)
    if fits(callee_pops) and not violated(callee_pops) and not fits(dict((v, 0) for v in open_)):
        out.update(callee_pops)
    return out


class Analysis:
    """Program-wide unknowns, extents and per-function frames."""

    def __init__(self, program, log=print):
        self.program = program
        self.log = log
        self.known = {}
        self.frames = {}
        self.conflicts = []
        self.chosen = {}

    def functions(self):
        return [fn for a, fn in sorted(self.program.functions.items())]

    def solve_shared(self):
        """Solves shared unknowns; returns per-function solutions."""
        fns = [fn for fn in self.functions() if fn.local_registers]
        known = {}
        solutions = {}
        pending = fns
        for fn in self.functions():
            info = self.program.stack.get(fn.entry_address)
            pops = ret_pops(fn)
            if pops is not None and info is not None and not info.skips:
                known[('fn', fn.entry_address)] = pops
        for round_ in range(5):
            votes = {}
            again = []
            for fn in pending:
                local, vars_, rows, chosen, w = solve_function(self.program, fn, known)
                self.chosen[fn.entry_address] = chosen
                solutions[fn.entry_address] = local if rows is not None else None
                if rows is None:
                    continue
                own = ('fn', fn.entry_address)
                info = self.program.stack.get(fn.entry_address)
                if own not in known and info is not None and info.delta is None and not info.skips:
                    pops = own_pops(w, known, self.program.stack)
                    if pops is not None:
                        votes.setdefault(own, set()).add(pops)
                for v, value in local.items():
                    if v[0] in ('imp', 'fn') and v not in known:
                        votes.setdefault(v, set()).add(value)
                if any(v[0] in ('imp', 'fn') and v not in local for v in vars_):
                    again.append(fn)
            changed = False
            for v, values in votes.items():
                if len(values) == 1:
                    known[v] = values.pop()
                    changed = True
                elif v not in dict(self.conflicts):
                    self.conflicts.append((v, sorted(values)))
            if not changed or not again:
                break
            pending = again
        for v, _ in self.conflicts:
            known.pop(v, None)
        self.known = known
        return solutions

    def run(self, promote=True, separate=False):
        solutions = self.solve_shared()
        frames = {}
        for fn in self.functions():
            a = fn.entry_address
            if solutions.get(a) is None:
                f = Frame()
                f.reason = 'stack heights contradict' if a in solutions else 'registers in cpu'
                f.unbounded = True
                frames[a] = f
                continue
            local = dict(self.known)
            local.update(solutions[a])
            frames[a] = walk(self.program, fn, local, {}, True).frame
        extents = {}
        for _ in range(30):
            new = dict((a, frame_extent(self.program.functions[a], f, extents)) for a, f in frames.items())
            if new == extents:
                break
            extents = new
        self.extents = extents
        walkers = set(a for a, f in frames.items() if f.entry_ebp)
        changed = True
        while changed:
            changed = False
            for a, f in frames.items():
                if a not in walkers and any(orig and t in walkers for t, _, orig in f.ebp_calls):
                    walkers.add(a)
                    changed = True
        self.ebp_readers = walkers
        for a, f in frames.items():
            if f.reason is None and any(frame and t in walkers for t, frame, _ in f.ebp_calls):
                f.reason = 'callee reads the frame through ebp'
        helpers = set(a for a, f in frames.items() if f.reason and (f.reason.startswith(FRAME_HELPER) or (
            f.reason.startswith('fs access') and frame_helper(self.program.functions[a])))) - {CHKSTK}
        for a, f in frames.items():
            fn = self.program.functions[a]
            if f.reason is None and any(fn.branch[i][0] == 'call' and fn.branch[i][1] in helpers for i in fn.reachable):
                f.reason = 'calls a function that changes its frame'
        for a, f in frames.items():
            f.extent = extents.get(a)
            if f.reason is None and not heights_plausible(self.program.functions[a], f):
                f.reason = 'pops do not match the pushes'
            if f.reason is None:
                if promote:
                    choose_slots(self.program, self.program.functions[a], f, extents)
                if separate:
                    separate_stack(f)
        self.frames = frames
        return frames


def moves_esp(fn, i):
    """esp set from something other than itself and constants."""
    ins = fn.instructions[i]
    if not fn.effects[i].reg_def & C.REG_BIT['esp'] or ins.mnemonic.split()[-1] in ('push', 'pop', 'call', 'ret'):
        return False
    ops = ins.operands
    if ins.mnemonic in ('add', 'sub', 'and') and len(ops) == 2 and ops[1].type == X.X86_OP_IMM:
        return False
    return not (ins.mnemonic == 'lea' and ops[1].mem.base and ins.reg_name(ops[1].mem.base) == 'esp'
                and not ops[1].mem.index)


def frame_helper(fn):
    """Sets esp from a register, or leaves through the caller's ebp."""
    for i in fn.reachable:
        ins = fn.instructions[i]
        if moves_esp(fn, i) and ins.mnemonic not in ('leave', 'mov'):
            return True
        if ins.mnemonic in ('leave', 'mov') and moves_esp(fn, i) and not any(
                fn.instructions[j].mnemonic == 'mov' and fn.instructions[j].op_str == 'ebp, esp' for j in fn.reachable):
            return True
    return False


def ret_pops(fn):
    """The ret immediate when every exit is one ret N."""
    values = set()
    for i in fn.reachable:
        ins = fn.instructions[i]
        if moves_esp(fn, i) or any(op.type == X.X86_OP_MEM and op.mem.segment for op in ins.operands):
            return None
        for x in fn.exits[i]:
            if x.edge == 'call':
                continue
            if x.kind != 'ret' or fn.instructions[i].mnemonic.split()[-1] != 'ret':
                return None
            ops = fn.instructions[i].operands
            values.add(ops[0].imm if ops else 0)
    return values.pop() if len(values) == 1 else None


def frame_extent(fn, frame, extents):
    """Bytes above the entry esp the function may access."""
    if frame.reason is not None or frame.unbounded:
        return None
    e = 4
    for a in frame.accesses:
        if a.base == ENTRY and a.k + a.size > e:
            e = a.k + a.size
    for i, height, target, pops, cleanup in frame.calls:
        extent = extents.get(target) if target is not None else None
        if height is None or extent is None or height[0] != ENTRY:
            continue
        e = max(e, height[1] - 4 + extent)
    for i in fn.reachable:
        for x in fn.exits[i]:
            if x.kind == 'tail':
                t = extents.get(x.target)
                h = frame.heights.get(i)
                if t is None:
                    return None
                if h and h[0] == ENTRY:
                    e = max(e, h[1] + t)
    return e


def heights_plausible(fn, frame):
    """Pops restore saved registers from their own push slots."""
    pushed = {}
    for a in frame.accesses:
        if a.key == ('push',) and a.save:
            pushed.setdefault((a.base, a.k), set()).add(a.save)
    for a in frame.accesses:
        if a.key != ('pop',):
            continue
        if a.base == ENTRY and a.k >= 0:
            return False
        op = fn.instructions[a.index].operands[0]
        name = fn.instructions[a.index].reg_name(op.reg) if op.type == X.X86_OP_REG else None
        regs = pushed.get((a.base, a.k))
        if name in SAVED and regs and name not in regs:
            return False
    return True


def choose_slots(program, fn, frame, extents):
    by_slot = {}
    for a in frame.accesses:
        by_slot.setdefault((a.base, a.k), []).append(a)
    shapes = {}
    for (base, k), accs in by_slot.items():
        sizes = set(a.size for a in accs)
        shapes[(base, k)] = max(sizes)
    bad = set()
    items = sorted(shapes.items(), key=slot_order)
    for (base, k), size in items:
        accs = by_slot[(base, k)]
        if len(set(a.size for a in accs)) != 1 or not all(a.supported for a in accs):
            bad.add((base, k))
        for (b2, k2), s2 in items:
            if b2 == base and (b2, k2) != (base, k) and k2 < k + size and k < k2 + s2:
                bad.add((base, k))
                bad.add((b2, k2))
    floors = frame.floor
    aligned = [b for b in floors if b != ENTRY]
    if aligned:
        lowest = min((h[1] for i, h in frame.heights.items() if h and h[0] == ENTRY
                      and fn.instructions[i].mnemonic.startswith('and')), default=0)
        for (base, k), size in shapes.items():
            if (base == ENTRY and k < lowest) or (base != ENTRY and k + size > 0):
                frame.reason = 'aligned frame may alias the entry frame'
                return
    pending = pending_pushes(fn, prologue_end(fn))
    for (b, k), accs in by_slot.items():
        if not save_slot(accs) and all(a.key in (('push',), ('pop',)) for a in accs):
            bad.add((b, k))
    for i, height, target, pops, cleanup in frame.calls:
        extent = extents.get(target) if target is not None else None
        if height is None:
            frame.reason = 'call at an unknown height'
            return
        base, c = height
        reach = []
        if extent is not None:
            reach.append(extent - 4)
        if pops:
            reach.append(pops)
        if cleanup:
            reach.append(cleanup)
        if any(reach):
            top = c + max(reach)
            for (b, k), size in shapes.items():
                if b == base and k < top and k + size > c:
                    bad.add((b, k))
        top = c + pushed_at(fn, pending, i)[0]
        for (b, k), size in shapes.items():
            if b == base and k < top and k + size > c and not save_slot(by_slot[(b, k)]):
                bad.add((b, k))
    for base, c in frame.continued:
        for (b, k), size in shapes.items():
            if b != base or k + size > c:
                bad.add((b, k))
    live = slot_liveness(fn, frame, by_slot)
    names = {}
    for (base, k), accs in sorted(by_slot.items(), key=slot_order):
        if (base, k) in bad:
            continue
        size = accs[0].size
        if base == ENTRY and k >= 4:
            arg_slot(fn, frame, base, k, accs)
            continue
        floor = floors.get(base)
        if floor is None or k < floor or k + size > 0:
            continue
        save = save_slot(accs)
        if frame.escaped and not save:
            continue
        if not (SAVES if save else LOCALS):
            continue
        if not any(a.read for a in accs):
            continue
        info = live.get((base, k))
        if info is None or info['across_call'] or (info['entry'] and base != ENTRY):
            continue
        slot = new_slot(base, k, accs, save)
        slot.entry_load = info['entry']
        frame.slots[(base, k)] = slot
        for a in accs:
            frame.sites[(a.index, a.key)] = slot


def separate_stack(frame):
    """Marks frame accesses no other guest access can alias."""
    if frame.escaped:
        if frame.escaped_unknown or frame.top_accesses or any(b != ENTRY for b in frame.floor):
            return
        if any(k < frame.floor.get(base, 0) for base, k in frame.escaped_at):
            return
        frame.stack_limit = frame.floor.get(ENTRY, 0)
    groups = []
    for a in sorted(frame.accesses, key=lambda a: (repr(a.base), a.k)):
        if (a.index, a.key) in frame.sites:
            continue
        if a.base == ENTRY and a.k + a.size > 0:
            continue
        if groups and groups[-1][0] == a.base and a.k < groups[-1][1]:
            groups[-1][1] = max(groups[-1][1], a.k + a.size)
            groups[-1][2].append(a)
        else:
            groups.append([a.base, a.k + a.size, [a]])
    chosen = [accs for base, end, accs in groups if not (frame.escaped and end > frame.floor.get(base, 0))]
    if not all(a.supported for accs in chosen for a in accs):
        return
    for accs in chosen:
        for a in accs:
            frame.stack_sites.add((a.index, a.key))


def save_slot(accs):
    """The register when every write is a push of its entry value."""
    writes = [a for a in accs if a.write]
    if writes and all(a.save for a in writes) and len(set(a.save for a in writes)) == 1:
        return writes[0].save
    return None


def new_slot(base, k, accs, save):
    size = accs[0].size
    kinds = set(a.kind for a in accs)
    if kinds == {'f32'} and size == 4:
        ctype = 'float'
    elif kinds == {'f64'} and size == 8:
        ctype = 'double'
    else:
        ctype = 'x86::reg%d' % (size * 8)
    tag = 'm%x' % -k if k < 0 else 'p%x' % k
    if base != ENTRY:
        tag = 'a%x_%s' % (base[1], tag)
    return Slot(base, k, size, ctype, 'sl_' + tag, save)


ARGUMENTS = True             # what kinds of slots choose_slots promotes (for comparisons)
SAVES = True
LOCALS = True


def arg_slot(fn, frame, base, k, accs):
    """Caches an argument read more than once in a local."""
    if not ARGUMENTS or frame.unbounded or frame.escaped_args:
        return
    reads = [a for a in accs if a.read]
    if len(reads) < 2:
        return
    dom = frame.dominators(fn)
    sites = set(a.index for a in accs)
    load = None
    for r in reads:
        if all(dom[x] >> r.index & 1 for x in sites):
            load = r
            break
    slot = new_slot(base, k, accs, None)
    slot.arg = True
    if load is None:
        slot.entry_load = True
    else:
        slot.load_site = (load.index, load.key)
    frame.slots[(base, k)] = slot
    for a in accs:
        frame.sites[(a.index, a.key)] = slot


def slot_order(item):
    (base, k) = item[0]
    return (repr(base), k)


def slot_liveness(fn, frame, by_slot):
    """Per slot: live at entry, live across a clobbering call."""
    n = fn.n
    gen = {}
    kill = {}
    for (base, k), accs in by_slot.items():
        for a in accs:
            if a.read:
                gen.setdefault(a.index, set()).add((base, k))
            elif a.write:
                kill.setdefault(a.index, set()).add((base, k))
    for i in gen:
        if i in kill:
            kill[i] -= gen[i]
    live_in = [set() for _ in range(n)]
    changed = True
    order = sorted(fn.reachable, reverse=True)
    while changed:
        changed = False
        for i in order:
            out = set()
            for s in fn.succ[i]:
                out |= live_in[s]
            v = (out - kill.get(i, set())) | gen.get(i, set())
            if v != live_in[i]:
                live_in[i] = v
                changed = True
    result = dict((s, {'entry': s in live_in[fn.entry], 'across_call': False}) for s in by_slot)
    for i, h, _, _, _ in frame.calls:
        out = set()
        for s in fn.succ[i]:
            out |= live_in[s]
        for (base, k) in out:
            if h is None:
                across = True
            elif h[0] == base:
                across = k < h[1]
            else:
                across = base != ENTRY
            if across:
                result[(base, k)]['across_call'] = True
    return result
