"""Writes include/winapi/d3d9_fxtables.h from SPEED2.EXE's 23 compiled effects; fails on what the port does not model.

    python3 -I tools/fx/fxtable.py --exe <game folder>/SPEED2.EXE --out include/winapi/d3d9_fxtables.h
"""
import argparse, hashlib, re, struct, sys

# ---- PE ------------------------------------------------------------------------------------------------------------

class PE:
    def __init__(self, data):
        self.d = data
        e = struct.unpack_from('<I', data, 0x3c)[0]
        nsec = struct.unpack_from('<H', data, e + 6)[0]
        opt = struct.unpack_from('<H', data, e + 20)[0]
        self.base = struct.unpack_from('<I', data, e + 24 + 28)[0]
        self.resource_rva = struct.unpack_from('<I', data, e + 24 + 96 + 8 * 2)[0]
        self.secs = []
        s = e + 24 + opt
        for _ in range(nsec):
            vs, va, rs, ro = struct.unpack_from('<IIII', data, s + 8)
            self.secs.append((va, vs, ro, rs))
            s += 40

    def offset(self, rva):
        for va, vs, ro, rs in self.secs:
            if va <= rva < va + max(vs, rs):
                return ro + rva - va
        raise ValueError('rva 0x%x' % rva)

    def cstr(self, va):
        o = self.offset(va - self.base)
        return self.d[o:self.d.index(b'\0', o)].decode('latin1')

    def rcdata(self):
        """RT_RCDATA resources by name: bytes."""
        root = self.offset(self.resource_rva)
        def entries(o):
            named, ids = struct.unpack_from('<HH', self.d, o + 12)
            for k in range(named + ids):
                name, target = struct.unpack_from('<II', self.d, o + 16 + 8 * k)
                yield name, target
        def name_of(n):
            if not n & 0x80000000:
                return n
            o = root + (n & 0x7fffffff)
            length = struct.unpack_from('<H', self.d, o)[0]
            return self.d[o + 2:o + 2 + 2 * length].decode('utf-16-le')
        out = {}
        for tid, tdir in entries(root):
            if name_of(tid) != 10:
                continue
            for nid, ndir in entries(root + (tdir & 0x7fffffff)):
                for _, leaf in entries(root + (ndir & 0x7fffffff)):
                    rva, size = struct.unpack_from('<II', self.d, root + leaf)
                    o = self.offset(rva)
                    out[name_of(nid)] = self.d[o:o + size]
        return out

# ---- fx_2_0 --------------------------------------------------------------------------------------------------------

RS = ['ZENABLE', 'FILLMODE', 'SHADEMODE', 'ZWRITEENABLE', 'ALPHATESTENABLE', 'LASTPIXEL', 'SRCBLEND', 'DESTBLEND',
      'CULLMODE', 'ZFUNC', 'ALPHAREF', 'ALPHAFUNC', 'DITHERENABLE', 'ALPHABLENDENABLE', 'FOGENABLE',
      'SPECULARENABLE', 'FOGCOLOR', 'FOGTABLEMODE', 'FOGSTART', 'FOGEND', 'FOGDENSITY', 'RANGEFOGENABLE',
      'STENCILENABLE', 'STENCILFAIL', 'STENCILZFAIL', 'STENCILPASS', 'STENCILFUNC', 'STENCILREF', 'STENCILMASK',
      'STENCILWRITEMASK', 'TEXTUREFACTOR'] + ['WRAP%d' % i for i in range(16)] + [
      'CLIPPING', 'LIGHTING', 'AMBIENT', 'FOGVERTEXMODE', 'COLORVERTEX', 'LOCALVIEWER', 'NORMALIZENORMALS',
      'DIFFUSEMATERIALSOURCE', 'SPECULARMATERIALSOURCE', 'AMBIENTMATERIALSOURCE', 'EMISSIVEMATERIALSOURCE',
      'VERTEXBLEND', 'CLIPPLANEENABLE', 'POINTSIZE', 'POINTSIZE_MIN', 'POINTSIZE_MAX', 'POINTSPRITEENABLE',
      'POINTSCALEENABLE', 'POINTSCALE_A', 'POINTSCALE_B', 'POINTSCALE_C', 'MULTISAMPLEANTIALIAS', 'MULTISAMPLEMASK',
      'PATCHEDGESTYLE', 'DEBUGMONITORTOKEN', 'INDEXEDVERTEXBLENDENABLE', 'COLORWRITEENABLE', 'TWEENFACTOR',
      'BLENDOP', 'POSITIONDEGREE', 'NORMALDEGREE', 'SCISSORTESTENABLE', 'SLOPESCALEDEPTHBIAS',
      'ANTIALIASEDLINEENABLE', 'MINTESSELLATIONLEVEL', 'MAXTESSELLATIONLEVEL', 'ADAPTIVETESS_X', 'ADAPTIVETESS_Y',
      'ADAPTIVETESS_Z', 'ADAPTIVETESS_W', 'ENABLEADAPTIVETESSELLATION', 'TWOSIDEDSTENCILMODE', 'CCW_STENCILFAIL',
      'CCW_STENCILZFAIL', 'CCW_STENCILPASS', 'CCW_STENCILFUNC', 'COLORWRITEENABLE1', 'COLORWRITEENABLE2',
      'COLORWRITEENABLE3', 'BLENDFACTOR', 'SRGBWRITEENABLE', 'DEPTHBIAS', 'SEPARATEALPHABLENDENABLE',
      'SRCBLENDALPHA', 'DESTBLENDALPHA', 'BLENDOPALPHA']
D3DRS = dict(ZENABLE=7, FILLMODE=8, SHADEMODE=9, ZWRITEENABLE=14, ALPHATESTENABLE=15, LASTPIXEL=16, SRCBLEND=19,
             DESTBLEND=20, CULLMODE=22, ZFUNC=23, ALPHAREF=24, ALPHAFUNC=25, DITHERENABLE=26, ALPHABLENDENABLE=27,
             FOGENABLE=28, SPECULARENABLE=29, FOGCOLOR=34, FOGTABLEMODE=35, FOGSTART=36, FOGEND=37, FOGDENSITY=38,
             RANGEFOGENABLE=48, TEXTUREFACTOR=60, CLIPPING=136, LIGHTING=137, AMBIENT=139, FOGVERTEXMODE=140,
             COLORVERTEX=141, LOCALVIEWER=142, NORMALIZENORMALS=143, DIFFUSEMATERIALSOURCE=145,
             SPECULARMATERIALSOURCE=146, AMBIENTMATERIALSOURCE=147, EMISSIVEMATERIALSOURCE=148, VERTEXBLEND=151,
             COLORWRITEENABLE=168, BLENDOP=171, BLENDFACTOR=193)
TSS = ['COLOROP', 'COLORARG0', 'COLORARG1', 'COLORARG2', 'ALPHAOP', 'ALPHAARG0', 'ALPHAARG1', 'ALPHAARG2',
       'RESULTARG', 'BUMPENVMAT00', 'BUMPENVMAT01', 'BUMPENVMAT10', 'BUMPENVMAT11', 'TEXCOORDINDEX',
       'BUMPENVLSCALE', 'BUMPENVLOFFSET', 'TEXTURETRANSFORMFLAGS', 'CONSTANT']
D3DTSS = dict(COLOROP=1, COLORARG1=2, COLORARG2=3, ALPHAOP=4, ALPHAARG1=5, ALPHAARG2=6, BUMPENVMAT00=7,
              BUMPENVMAT01=8, BUMPENVMAT10=9, BUMPENVMAT11=10, TEXCOORDINDEX=11, BUMPENVLSCALE=22, BUMPENVLOFFSET=23,
              TEXTURETRANSFORMFLAGS=24, COLORARG0=26, ALPHAARG0=27, RESULTARG=28, CONSTANT=32)
OTHER = ['NPatchMode', 'FVF', 'Transform.PROJECTION', 'Transform.VIEW', 'Transform.WORLD', 'Transform.TEXTURE',
         'MaterialDiffuse', 'MaterialAmbient', 'MaterialSpecular', 'MaterialEmissive', 'MaterialPower',
         'LightType', 'LightDiffuse', 'LightSpecular', 'LightAmbient', 'LightPosition', 'LightDirection',
         'LightRange', 'LightFalloff', 'LightAttenuation0', 'LightAttenuation1', 'LightAttenuation2', 'LightTheta',
         'LightPhi', 'LightEnable', 'VertexShader', 'PixelShader', 'VSConstF', 'VSConstB', 'VSConstI', 'VSConst',
         'VSConst1', 'VSConst2', 'VSConst3', 'VSConst4', 'PSConstF', 'PSConstB', 'PSConstI', 'PSConst', 'PSConst1',
         'PSConst2', 'PSConst3', 'PSConst4', 'Texture']
SAMP = ['ADDRESSU', 'ADDRESSV', 'ADDRESSW', 'BORDERCOLOR', 'MAGFILTER', 'MINFILTER', 'MIPFILTER', 'MIPMAPLODBIAS',
        'MAXMIPLEVEL', 'MAXANISOTROPY', 'SRGBTEXTURE', 'ELEMENTINDEX', 'DMAPOFFSET', 'Sampler']
D3DSAMP = dict(ADDRESSU=1, ADDRESSV=2, ADDRESSW=3, BORDERCOLOR=4, MAGFILTER=5, MINFILTER=6, MIPFILTER=7,
               MIPMAPLODBIAS=8, MAXMIPLEVEL=9, MAXANISOTROPY=10)

# The port's pass states (d3d9_internal.h PassState) and sampler fields (fx::SamplerField), in their order.
PASS_STATES = ['ZENABLE', 'ZWRITEENABLE', 'ALPHATESTENABLE', 'SRCBLEND', 'DESTBLEND', 'CULLMODE', 'ZFUNC', 'ALPHAREF',
               'ALPHABLENDENABLE', 'FOGENABLE', 'FOGCOLOR', 'TEXTUREFACTOR', 'LIGHTING', 'COLORWRITEENABLE']
FIELDS = ['ADDRESSU', 'ADDRESSV', 'MAGFILTER', 'MINFILTER', 'MIPFILTER']
# Sampler states the port does not model (render::Sampler): kept in End's restore list only.
SAMPLER_IGNORED = ['MAXANISOTROPY']

def state_kind(op):
    """('rs', name) | ('tss', name) | ('other', name) | ('samp', name)."""
    if op < len(RS):
        return 'rs', RS[op]
    op -= len(RS)
    if op < len(TSS):
        return 'tss', TSS[op]
    op -= len(TSS)
    if op < len(OTHER):
        return 'other', OTHER[op]
    op -= len(OTHER)
    if op < len(SAMP):
        return 'samp', SAMP[op]
    raise ValueError('state op %d' % op)

TYPES = ['void', 'bool', 'int', 'float', 'string', 'texture', 'texture1d', 'texture2d', 'texture3d', 'texturecube',
         'sampler', 'sampler1d', 'sampler2d', 'sampler3d', 'samplercube', 'pixelshader', 'vertexshader']

class R:
    def __init__(self, data, pos):
        self.d, self.p = data, pos

    def u(self):
        v = struct.unpack_from('<I', self.d, self.p)[0]
        self.p += 4
        return v

def name_at(base, off):
    if off == 0:
        return ''
    n = struct.unpack_from('<I', base, off)[0]
    return base[off + 4:off + 4 + n].split(b'\0')[0].decode('latin1')

def typedef(base, off):
    r = R(base, off)
    t, c = r.u(), r.u()
    name, sem = name_at(base, r.u()), name_at(base, r.u())
    elements = r.u()
    rows = cols = 1
    if c in (0, 2, 3):
        rows, cols = r.u(), r.u()
    elif c == 1:
        cols, rows = r.u(), r.u()
    elif c == 5:
        raise ValueError('struct parameter %s' % name)
    return dict(type=t, cls=c, name=name, semantic=sem, elements=elements, rows=rows, cols=cols)

def parse_state(base, r):
    op, idx, tdo, vo = r.u(), r.u(), r.u(), r.u()
    return dict(op=op, index=idx, td=typedef(base, tdo), value=struct.unpack_from('<I', base, vo)[0])

def parse(data):
    tag, off = struct.unpack_from('<II', data, 0)
    if tag != 0xFEFF0901:
        raise ValueError('not fx_2_0')
    base = data[8:]
    r = R(base, off)
    pcount, tcount, _, _ = r.u(), r.u(), r.u(), r.u()
    params = []
    for _ in range(pcount):
        tdo, vo, flags, ac = r.u(), r.u(), r.u(), r.u()
        td = typedef(base, tdo)
        for _ in range(ac):
            r.u(); r.u()
        samplers = []
        if 10 <= td['type'] <= 14:
            rr = R(base, vo)
            for _ in range(max(1, td['elements'])):
                samplers.append([parse_state(base, rr) for _ in range(rr.u())])
        elif td['type'] in (1, 2, 3) and td['cls'] < 4:
            n = td['rows'] * td['cols'] * max(1, td['elements'])
            if any(base[vo:vo + 4 * n]):
                raise ValueError('parameter %s has a non-zero default' % td['name'])
        params.append(dict(td=td, flags=flags, samplers=samplers))
    techs = []
    for _ in range(tcount):
        tname = name_at(base, r.u())
        ac, pc = r.u(), r.u()
        annos = []
        for _ in range(ac):
            atd, avo = typedef(base, r.u()), r.u()
            if atd['type'] != 2 or atd['cls'] != 0:
                raise ValueError('technique %s annotation %s is not an int' % (tname, atd['name']))
            annos.append((atd['name'], struct.unpack_from('<i', base, avo)[0]))
        passes = []
        for _ in range(pc):
            pname = name_at(base, r.u())
            ac2, sc = r.u(), r.u()
            if ac2:
                raise ValueError('pass annotations')
            passes.append((pname, [parse_state(base, r) for _ in range(sc)]))
        techs.append((tname, annos, passes))
    scount, rcount = r.u(), r.u()
    for _ in range(scount):
        r.u()
        size = r.u()
        r.p += (size + 3) & ~3
    resources = {}
    for _ in range(rcount):
        ti, pi, ei, si, usage, size = r.u(), r.u(), r.u(), r.u(), r.u(), r.u()
        resources[(ti, pi, ei, si)] = (usage, base[r.p:r.p + size])
        r.p += (size + 3) & ~3
    return params, techs, resources

# ---- Bytecode comments: CTAB and FXLC ------------------------------------------------------------------------------

def comments(blob):
    toks = struct.unpack_from('<%dI' % (len(blob) // 4), blob)
    i = 1
    while i < len(toks) and toks[i] != 0xffff:
        if (toks[i] & 0xffff) == 0xfffe:
            n = (toks[i] >> 16) & 0x7fff
            payload = blob[4 * (i + 1):4 * (i + 1 + n)]
            yield payload[:4], payload[4:]
            i += 1 + n
        else:
            i += 1

def ctab(payload):
    _, _, _, count, info, _, _ = struct.unpack_from('<7I', payload, 0)
    def cstr(o):
        return payload[o:payload.index(b'\0', o)].decode('latin1')
    out = []
    for k in range(count):
        no, rs, ri, rc, _, _, _ = struct.unpack_from('<IHHHHII', payload, info + 20 * k)
        out.append(dict(name=cstr(no), regset=rs, reg=ri, count=rc))
    return out

def sampler_registers(shader):
    """ps sampler register -> sampler parameter name."""
    for cc, payload in comments(shader):
        if cc == b'CTAB':
            return {c['reg'] + k: c['name'] for c in ctab(payload) if c['regset'] == 3 for k in range(c['count'])}
    return {}

def shader_version(blob):
    v = struct.unpack_from('<I', blob, 0)[0]
    return (v >> 16, (v >> 8) & 0xff, v & 0xff)

def expression(blob):
    """(name, element) of an FXLC mov from a parameter."""
    regs = {}
    code = None
    for cc, payload in comments(blob):
        if cc == b'CTAB':
            for c in ctab(payload):
                for k in range(c['count']):
                    regs[c['reg'] + k] = (c['name'], k)
        elif cc == b'FXLC':
            code = struct.unpack_from('<%dI' % (len(payload) // 4), payload)
        elif cc == b'CLIT':
            if struct.unpack_from('<I', payload, 0)[0]:
                raise ValueError('expression with literals')
    if code is None or code[0] != 1:
        raise ValueError('expression of %d instructions' % (code[0] if code else 0))
    op, nargs = (code[1] & 0x7ff00000) >> 20, code[2]
    if op != 0x100 or nargs != 1 or code[1] & 0xffff != 1:
        raise ValueError('expression op 0x%x' % op)
    p = 3
    args = []
    for _ in range(nargs + 1):
        if code[p]:
            raise ValueError('relative expression argument')
        args.append((code[p + 1], code[p + 2]))
        p += 3
    (table, off), (otable, ooff) = args
    if table != 2 or off % 4 or otable != 4 or ooff != 0:
        raise ValueError('expression arguments %s' % (args,))
    return regs[off // 4]

# ---- Tables --------------------------------------------------------------------------------------------------------

GAME_ORDER_TABLE = 0x7ff7d0     # 23 descriptors of 0x8c bytes; +0x84: the resource name
EFFECT_NAMES = {
    'WORLD': 'World', 'WORLDHEADLIGHT': 'WorldHeadlight', 'WORLDREFLECT': 'WorldReflect',
    'WORLDHEADLIGHTREFLECT': 'WorldHeadlightReflect', 'WORLDBONE': 'WorldBone', 'CAR': 'Car', 'WORLDMIN': 'WorldMin',
    'WORLDNOFOG': 'WorldNoFog', 'FE': 'Fe', 'DEPTH_WORLD': 'DepthWorld', 'DEPTH_WORLDHEADLIGHT': 'DepthWorldHeadlight',
    'DEPTH_WORLDREFLECT': 'DepthWorldReflect', 'DEPTH_WORLDHEADLIGHTREFLECT': 'DepthWorldHeadlightReflect',
    'WORLDNODEPTH': 'WorldNoDepth', 'WORLDHEADLIGHTNODEPTH': 'WorldHeadlightNoDepth', 'FILTER': 'Filter',
    'OVERBRIGHT': 'OverBright', 'SCREENFILTER': 'ScreenFilter', 'DEPTH_OF_FIELD': 'DepthOfField',
    'RAIN_DROP': 'RainDrop', 'SKYMORPH': 'SkyMorph', 'HORIZON_FOG': 'HorizonFog', 'RUNWAYLIGHT': 'RunwayLight',
}

def param_enum(name):
    return 'P' + ''.join(w[:1].upper() + w[1:] for w in name.split('_'))

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--exe', required=True)
    ap.add_argument('--out', required=True)
    a = ap.parse_args()
    exe = open(a.exe, 'rb').read()
    pe = PE(exe)
    res = pe.rcdata()
    order = [pe.cstr(struct.unpack_from('<I', exe, pe.offset(GAME_ORDER_TABLE + i * 0x8c + 0x84 - pe.base))[0])
             for i in range(23)]
    fx_names = sorted(k for k in res if isinstance(k, str) and k.endswith('_FX'))
    if len(fx_names) != 23 or sorted(order) != fx_names:
        raise ValueError('resources %s, game order %s' % (fx_names, order))

    params = []         # global: name -> info
    index = {}
    effects = []
    for resource in order:
        data = res[resource]
        ps, techs, resources = parse(data)
        local = []
        for i, p in enumerate(ps):
            td = p['td']
            numeric = td['type'] in (1, 2, 3) and td['cls'] < 4
            floats = td['rows'] * td['cols'] * max(1, td['elements']) if numeric else 0
            info = (td['name'], td['type'], td['cls'], td['rows'], td['cols'], td['elements'], floats)
            if td['name'] in index:
                if params[index[td['name']]] != info:
                    raise ValueError('%s: parameter %s shape %s differs from %s' % (resource, td['name'], info,
                                                                                    params[index[td['name']]]))
            else:
                index[td['name']] = len(params)
                params.append(info)
            local.append(dict(param=index[td['name']], name=td['name'], shared=bool(p['flags'] & 1),
                              semantic=td['semantic'] or None, samplers=p['samplers'], type=td['type']))
        effects.append(dict(resource=resource, data=data, params=local, techs=techs, resources=resources))

    enums = [param_enum(p[0]) for p in params]
    if len(set(enums)) != len(enums):
        raise ValueError('parameter enum collision')

    def ref_value(e, key, state, pname):
        """Value of a state: ('const', v) or ('param', name, element)."""
        if key in e['resources']:
            usage, blob = e['resources'][key]
            if usage == 1:
                return ('param', blob.split(b'\0')[0].decode('latin1'), 0)
            if usage == 0 and struct.unpack_from('<I', blob, 0)[0] >> 16 == 0x4658:
                return ('param',) + expression(blob)
            if usage == 0:
                return ('shader', blob)
            raise ValueError('%s %s: resource usage %d' % (e['resource'], pname, usage))
        return ('const', state['value'])

    out_effects = []
    for e in effects:
        lp = {p['name']: p for p in e['params']}
        techs = []
        for t, (tname, annos, passes) in enumerate(e['techs']):
            tech = dict(name=tname, annos=annos, pass_count=len(passes), passes=None)
            if t == 0:
                merged_rs, merged_tss, merged_samp = {}, {}, {}
                restore_rs, restore_tss, restore_samp = [], [], []
                shaders = {}
                out_passes = []
                for p, (pname, states) in enumerate(passes):
                    notes = []
                    for si, s in enumerate(states):
                        kind, name = state_kind(s['op'])
                        where = '%s t0 %s %s' % (e['resource'], pname, name)
                        v = ref_value(e, (0, p, 0xffffffff, si), s, where)
                        if kind == 'rs':
                            if name not in D3DRS:
                                raise ValueError(where + ': render state not in the table')
                            if D3DRS[name] not in restore_rs:
                                restore_rs.append(D3DRS[name])
                            if name in PASS_STATES:
                                merged_rs[name] = v
                            else:
                                notes.append('%s = %s (not modelled)' % (name, v[1] if v[0] == 'const' else v[1:]))
                        elif kind == 'tss':
                            key = (s['index'], D3DTSS[name])
                            if key not in restore_tss:
                                restore_tss.append(key)
                            merged_tss[key] = v
                        elif kind == 'samp':
                            key = (s['index'], D3DSAMP[name])
                            if key not in restore_samp:
                                restore_samp.append(key)
                            merged_samp.setdefault(s['index'], {})[name] = v
                        elif name == 'Texture':
                            if v[0] != 'param':
                                raise ValueError(where + ': texture is not a parameter')
                            merged_samp.setdefault(s['index'], {})['Texture'] = v
                        elif name in ('VertexShader', 'PixelShader'):
                            if v[0] == 'const':
                                if v[1]:
                                    raise ValueError(where + ': shader value %d' % v[1])
                                shaders[name] = None
                            else:
                                shaders[name] = v[1]
                            if name == 'PixelShader' and v[0] == 'shader':
                                for reg, sname in sampler_registers(v[1]).items():
                                    sp = lp[sname]
                                    if len(sp['samplers']) != 1:
                                        raise ValueError(where + ': sampler array %s' % sname)
                                    for k, ss in enumerate(sp['samplers'][0]):
                                        skind, sn = state_kind(ss['op'])
                                        sv = ref_value(e, (0xffffffff, index_of(e, sname), 0, k), ss,
                                                       where + ' ' + sname)
                                        if sn == 'Texture':
                                            merged_samp.setdefault(reg, {})['Texture'] = sv
                                        elif skind == 'samp':
                                            if (reg, D3DSAMP[sn]) not in restore_samp:
                                                restore_samp.append((reg, D3DSAMP[sn]))
                                            merged_samp.setdefault(reg, {})[sn] = sv
                                        else:
                                            raise ValueError(where + ': sampler state %s' % sn)
                        elif name == 'FVF':
                            notes.append('FVF = 0x%x (not modelled)' % v[1])
                        else:
                            raise ValueError(where + ': state not modelled')
                    vs, ps_ = shaders.get('VertexShader'), shaders.get('PixelShader')
                    out_passes.append(dict(name=pname, rs=dict(merged_rs), tss=dict(merged_tss),
                                           samp={k: dict(v) for k, v in merged_samp.items()},
                                           vs=shader_version(vs) if vs else None,
                                           ps=shader_version(ps_) if ps_ else None, notes=notes))
                tech['passes'] = out_passes
                tech['restore'] = (restore_rs, restore_tss, restore_samp)
            techs.append(tech)
        out_effects.append(dict(e, techs=techs))

    write(a.out, params, enums, out_effects, index)
    nsamp = sum(1 for p in params if 10 <= p[1] <= 14)
    print('%d effects, %d parameters (%d samplers), %d value floats' % (len(out_effects), len(params), nsamp,
                                                                        sum(p[6] for p in params)))

def index_of(e, name):
    for i, p in enumerate(e['params']):
        if p['name'] == name:
            return i
    raise KeyError(name)

def cvalue(v, enums, index):
    if v is None:
        return '{}'
    if v[0] == 'const':
        return '{ SourceConst, 0, 0, 0x%x }' % v[1]
    if v[0] == 'param':
        return '{ SourceParam, %s, %d, 0 }' % (enums[index[v[1]]], v[2])
    raise ValueError(v)

def shader_code(v):
    if not v:
        return '0'
    kind, major, minor = v
    return '0x%x%d%d' % (1 if kind == 0xfffe else 2, major, minor)

def write(path, params, enums, effects, index):
    L = []
    w = L.append
    w("// Effect tables of SPEED2.EXE's 23 compiled IDI_*_FX resources, generated by tools/fx/fxtable.py; do not edit.")
    w('// Technique 0 passes include the states earlier passes left set.')
    w('')
    w('#ifndef WINAPI_D3D9_FXTABLES_H_')
    w('#define WINAPI_D3D9_FXTABLES_H_')
    w('')
    w('#include <winapi/d3d9_internal.h>')
    w('')
    w('namespace win32 { namespace d3d9 { namespace fx')
    w('{')
    w('')
    w('enum : uint8_t { NoParam = 0xff };')
    w('enum Source : uint8_t { SourceNone, SourceConst, SourceParam };')
    w('enum SamplerField : uint8_t { FieldAddressU, FieldAddressV, FieldMagFilter, FieldMinFilter, FieldMipFilter,')
    w('                              FieldCount };')
    w('')
    w('struct ParamInfo')
    w('{')
    w('    const char* name;')
    w('    uint8_t     type, cls, rows, columns;   // D3DXPARAMETER_TYPE, D3DXPARAMETER_CLASS')
    w('    uint16_t    elements, floats;           // floats: storage of numeric parameters')
    w('};')
    w('struct EffectParam { uint8_t param; bool shared; const char* semantic; };')
    w('struct Value { uint8_t source, param, element; uint32_t value; };')
    w('struct Sampler { uint8_t stage, texture; Value fields[FieldCount]; };    // texture NoParam: the device\'s')
    w('struct Pass')
    w('{')
    w('    const char*     name;')
    w('    uint16_t        vertexShader, pixelShader;  // 0x111: vs_1_1, 0x214: ps_1_4; 0: fixed function')
    w('    Value           states[PassStateCount];')
    w('    uint8_t         ttfStages;                  // stages whose TEXTURETRANSFORMFLAGS the pass sets')
    w('    uint32_t        ttf0;')
    w('    const Sampler*  samplers;')
    w('    uint8_t         samplerCount;')
    w('};')
    w('struct Annotation { const char* name; int32_t value; };')
    w('struct Technique')
    w('{')
    w('    const char*         name;')
    w('    const Annotation*   annotations;')
    w('    uint8_t             annotationCount, passCount;')
    w('    const Pass*         passes;                 // technique 0 only')
    w('    const uint16_t*     restore;                // End: render, stage and sampler states (stage << 8 | type)')
    w('    uint8_t             restoreRender, restoreStage, restoreSampler;')
    w('};')
    w('struct EffectInfo')
    w('{')
    w('    const char*         resource;')
    w('    const char*         sha256;')
    w('    uint32_t            bytes;')
    w('    const EffectParam*  params;')
    w('    uint8_t             paramCount;')
    w('    const Technique*    techniques;')
    w('    uint8_t             techniqueCount;')
    w('};')
    w('')
    w('enum Param : uint8_t')
    w('{')
    line = '   '
    for n in enums + ['ParamCount']:
        if len(line) + len(n) + 2 > 118:
            w(line.rstrip())
            line = '   '
        line += ' %s,' % n
    w(line[:-1])
    w('};')
    w('')
    w('const ParamInfo s_params[ParamCount] = {')
    for p in params:
        w('    { "%s", %d, %d, %d, %d, %d, %d },' % p)
    w('};')
    w('')
    w('enum EffectKind : uint8_t      // the game\'s order (0x86eb28)')
    w('{')
    names = ['Effect' + EFFECT_NAMES[e['resource'][4:-3]] for e in effects]
    line = '   '
    for n in names + ['EffectCount']:
        if len(line) + len(n) + 2 > 118:
            w(line.rstrip())
            line = '   '
        line += ' %s,' % n
    w(line[:-1])
    w('};')
    w('')
    pass_index = {n: i for i, n in enumerate(PASS_STATES)}
    for e, ename in zip(effects, names):
        tag = ename[6:]
        w('// ---- %s ----' % e['resource'])
        w('const EffectParam s_%sParams[] = {' % tag)
        for p in e['params']:
            w('    { %s, %s, %s },' % (enums[p['param']], 'true' if p['shared'] else 'false',
                                       '"%s"' % p['semantic'] if p['semantic'] else 'nullptr'))
        w('};')
        for t, tech in enumerate(e['techs']):
            if tech['annos']:
                w('const Annotation s_%sT%dAnnotations[] = { %s };' % (tag, t, ', '.join(
                    '{ "%s", %d }' % an for an in tech['annos'])))
            if not tech['passes']:
                continue
            for p, ps in enumerate(tech['passes']):
                stages = sorted(ps['samp'])
                if stages:
                    w('const Sampler s_%sT%dP%dSamplers[] = {' % (tag, t, p))
                    for st in stages:
                        s = ps['samp'][st]
                        tex = s.get('Texture')
                        if tex and tex[0] != 'param':
                            raise ValueError('texture')
                        unknown = set(s) - set(FIELDS) - set(SAMPLER_IGNORED) - {'Texture'}
                        if unknown:
                            raise ValueError('%s sampler states %s' % (e['resource'], unknown))
                        w('    { %d, %s, {' % (st, enums[index[tex[1]]] if tex else 'NoParam'))
                        for f in FIELDS:
                            w('        %s,  // %s' % (cvalue(s.get(f), enums, index), f))
                        w('    } },')
                    w('};')
            w('const Pass s_%sT%dPasses[] = {' % (tag, t))
            for p, ps in enumerate(tech['passes']):
                for n in ps['notes']:
                    w('    // %s: %s' % (ps['name'], n))
                ttf = 0
                for (st, ty) in ps['tss']:
                    if ty == D3DTSS['TEXTURETRANSFORMFLAGS']:
                        ttf |= 1 << st
                ttf0 = ps['tss'].get((0, D3DTSS['TEXTURETRANSFORMFLAGS']))
                if ttf0 is not None and ttf0[0] != 'const':
                    raise ValueError('TTF0 from a parameter')
                stages = sorted(ps['samp'])
                w('    { "%s", %s, %s, {' % (ps['name'], shader_code(ps['vs']), shader_code(ps['ps'])))
                for n in PASS_STATES:
                    w('        %s,  // %s' % (cvalue(ps['rs'].get(n), enums, index), n))
                w('      }, 0x%x, 0x%x, %s, %d },' % (ttf, ttf0[1] if ttf0 else 0,
                                                    's_%sT%dP%dSamplers' % (tag, t, p) if stages else 'nullptr',
                                                    len(stages)))
            w('};')
            rrs, rtss, rsamp = tech['restore']
            w('const uint16_t s_%sT%dRestore[] = {' % (tag, t))
            groups = [[str(x) for x in rrs], ['0x%x' % (st << 8 | ty) for st, ty in rtss],
                      ['0x%x' % (st << 8 | ty) for st, ty in rsamp]]
            for g in groups:
                for k in range(0, len(g), 16):
                    w('    %s,' % ', '.join(g[k:k + 16]))
            w('};')
        w('const Technique s_%sTechniques[] = {' % tag)
        for t, tech in enumerate(e['techs']):
            an = 's_%sT%dAnnotations' % (tag, t) if tech['annos'] else 'nullptr'
            if tech['passes']:
                rrs, rtss, rsamp = tech['restore']
                w('    { "%s", %s, %d, %d,' % (tech['name'], an, len(tech['annos']), tech['pass_count']))
                w('      s_%sT%dPasses, s_%sT%dRestore, %d, %d, %d },' % (tag, t, tag, t, len(rrs), len(rtss),
                                                                       len(rsamp)))
            else:
                w('    { "%s", %s, %d, %d, nullptr, nullptr, 0, 0, 0 },' % (tech['name'], an, len(tech['annos']),
                                                                         tech['pass_count']))
        w('};')
        w('')
    w('const EffectInfo s_effects[EffectCount] = {')
    for e, ename in zip(effects, names):
        tag = ename[6:]
        w('    { "%s", "%s", %d,' % (e['resource'], hashlib.sha256(e['data']).hexdigest(), len(e['data'])))
        w('      s_%sParams, %d, s_%sTechniques, %d },' % (tag, len(e['params']), tag, len(e['techs'])))
    w('};')
    w('')
    w('}}}')
    w('')
    w('#endif')
    open(path, 'w').write('\n'.join(L) + '\n')

if __name__ == '__main__':
    main()
