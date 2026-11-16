"""Finds VitaSDK and LLVM on this computer, or downloads pinned versions once into a per-user folder.

python3 -I tools/deps.py vitasdk|llvm   prints the folder used
Tools already installed are preferred (VITASDK, NFSU_LLVM, PATH, Homebrew). Downloads are checked against SHA-256.
"""
import hashlib
import os
import platform
import re
import shutil
import subprocess
import sys
import tarfile
import urllib.request

VITASDK_URL = 'https://github.com/vitasdk/autobuilds/releases/download/sdk-snapshot-20261015.850.1/'
LLVM_URL = 'https://github.com/llvm/llvm-project/releases/download/llvmorg-22.1.8/'
LLVM_VERSION = '22.1.8'
LLVM_MINIMUM = 20  # older clang is untried with this code and its profile

# (system, machine): archive name, SHA-256
VITASDK = {
    ('windows', 'x86_64'): ('vitasdk-x86_64-w64-mingw32-2026-10-15_05-17-40.tar.bz2',
                            'bc7af73fa8c873ecc9f6ab89239e449bf94c2912231f62a6151ba2a13ff45326'),
    ('darwin', 'arm64'): ('vitasdk-arm64-apple-darwin-2026-10-15_05-00-31.tar.bz2',
                          '99f2e1cfb89f0995d80f50f55c927d2c6032c3938f2d79d7b18bba4c9e4ab4ce'),
    ('darwin', 'x86_64'): ('vitasdk-x86_64-apple-darwin-2026-10-15_05-17-51.tar.bz2',
                           'b801d0db100e8be48134ce1b1d1f3ab86ac27bbcb60ff23f533b8bde069df6d5'),
    ('linux', 'x86_64'): ('vitasdk-x86_64-linux-gnu-2026-10-15_05-01-16.tar.bz2',
                          '801f483928523fe63547faf58a7dfdaca7482b26c33af0064acea96a906f7662'),
    ('linux', 'arm64'): ('vitasdk-aarch64-linux-gnu-2026-10-15_05-00-35.tar.bz2',
                         'f312261397ba4400baa6a0b15a9670d1fcb7463b55b0cd54879fbd0126c0b17d'),
}
LLVM = {
    ('windows', 'x86_64'): ('clang+llvm-22.1.8-x86_64-pc-windows-msvc.tar.xz',
                            'd96c2cc1736f4eb7fa43cb9bbdf56d93551a9ae0a9aadb9c99c3c3b2b712a234'),
    ('darwin', 'arm64'): ('LLVM-22.1.8-macOS-ARM64.tar.xz',
                          'f260f4f7c0d430828a81ae8a3826a1d63fc0963ec2459489308cc23b1f7eab4f'),
    ('linux', 'x86_64'): ('LLVM-22.1.8-Linux-X64.tar.xz',
                          'df0e1ecf16caf3489a272a5eea4eec9b0d82878f6477fa309504f918a0006384'),
    ('linux', 'arm64'): ('LLVM-22.1.8-Linux-ARM64.tar.xz',
                         '805efad2bb91cb4967fa569e0881d10c0f69c04461cf671cccbae19f547acc34'),
}

EXE = '.exe' if os.name == 'nt' else ''
LLVM_TOOLS = ('clang', 'clang++', 'llvm-ar', 'llvm-ranlib', 'llvm-profdata')
LLVM_KEEP = re.compile(r'^(bin/(clang(\+\+|-\d+)?|lld|ld\.lld|llvm-(ar|ranlib|profdata|nm|objcopy|readelf|readobj))'
                       r'(\.exe)?|lib/clang/.*|lib/lib(LLVM|clang-cpp)[^/]*\.(dylib|so)[^/]*)$')


def host():
    """(system, machine) in the spelling the tables use."""
    machine = platform.machine().lower()
    machine = {'amd64': 'x86_64', 'aarch64': 'arm64'}.get(machine, machine)
    return platform.system().lower(), machine


def tools_dir():
    """Per-user folder that outlives any one copy of the source."""
    if os.environ.get('NFSU_TOOLS_DIR'):
        return os.environ['NFSU_TOOLS_DIR']
    home = os.path.expanduser('~')
    if os.name == 'nt':
        return os.path.join(os.environ.get('LOCALAPPDATA') or os.path.join(home, 'AppData', 'Local'),
                            'NFSU2Vita', 'tools')
    if sys.platform == 'darwin':
        return os.path.join(home, 'Library', 'Application Support', 'NFSU2Vita', 'tools')
    return os.path.join(os.environ.get('XDG_CACHE_HOME') or os.path.join(home, '.cache'), 'nfsu2vita', 'tools')


def note(text):
    print(text, file=sys.stderr, flush=True)


def file_hash(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as f:
        for block in iter(lambda: f.read(1 << 20), b''):
            digest.update(block)
    return digest.hexdigest()


def download(url, sha256, path):
    """Fetches url to path unless a good copy is there."""
    if os.path.exists(path) and file_hash(path) == sha256:
        return
    os.makedirs(os.path.dirname(path), exist_ok=True)
    part = path + '.part'
    note('Downloading ' + url)
    digest = hashlib.sha256()
    with urllib.request.urlopen(url) as response, open(part, 'wb') as out:
        total = int(response.headers.get('Content-Length') or 0)
        done, shown = 0, -1
        while True:
            block = response.read(1 << 20)
            if not block:
                break
            out.write(block)
            digest.update(block)
            done += len(block)
            if total and done * 10 // total != shown:
                shown = done * 10 // total
                note('  %d %% of %d MB' % (shown * 10, total >> 20))
    if digest.hexdigest() != sha256:
        os.remove(part)
        raise RuntimeError('SHA-256 mismatch for ' + url)
    os.replace(part, path)


def strip_top(name):
    parts = name.replace('\\', '/').split('/', 1)
    return parts[1] if len(parts) == 2 else ''


def unpack_vitasdk(archive, target):
    with tarfile.open(archive, 'r:bz2') as tar:
        members = []
        for member in tar.getmembers():
            member.name = strip_top(member.name)
            if member.name:
                members.append(member)
        if hasattr(tarfile, 'tar_filter'):
            tar.extractall(target, members=members, filter='tar')
        else:
            tar.extractall(target, members=members)


def link(source, path):
    """A symbolic link, or a copy where links need special rights."""
    if os.name == 'nt':
        shutil.copy2(os.path.normpath(os.path.join(os.path.dirname(path), source)), path)
    else:
        os.symlink(source, path)


def unpack_llvm(archive, target):
    """Only the tools the build runs, clang's headers and shared libraries."""
    links = {}
    with tarfile.open(archive, 'r|xz') as tar:
        for member in tar:
            name = strip_top(member.name)
            if not LLVM_KEEP.match(name):
                continue
            path = os.path.join(target, *name.split('/'))
            if member.issym():
                links[path] = member.linkname
            elif member.isfile():
                os.makedirs(os.path.dirname(path), exist_ok=True)
                with tar.extractfile(member) as src, open(path, 'wb') as out:
                    shutil.copyfileobj(src, out)
                os.chmod(path, member.mode | 0o200)
    for _ in range(4):
        for path, source in list(links.items()):
            if os.path.exists(os.path.normpath(os.path.join(os.path.dirname(path), source))):
                link(source, path)
                del links[path]
    binaries = os.path.join(target, 'bin')
    for wanted, fallback in (('clang++', 'clang'), ('ld.lld', 'lld')):
        if not os.path.exists(os.path.join(binaries, wanted + EXE)):
            link(fallback + EXE, os.path.join(binaries, wanted + EXE))


def install(name, table, base_url, folder, unpack, variable):
    """Downloads and unpacks one pinned package; returns its folder."""
    if host() not in table:
        sys.exit('No %s download is known for %s %s. Install it yourself and set %s to its folder.'
                 % (name, host()[0], host()[1], variable))
    archive_name, sha256 = table[host()]
    target = os.path.join(tools_dir(), folder)
    marker = os.path.join(target, '.installed')
    if os.path.exists(marker) and open(marker).read().strip() == sha256:
        return target
    archive = os.path.join(tools_dir(), 'downloads', archive_name)
    download(base_url + archive_name, sha256, archive)
    note('Unpacking %s (this can take several minutes)' % archive_name)
    if os.path.exists(target):
        shutil.rmtree(target)
    os.makedirs(target)
    unpack(archive, target)
    with open(marker, 'w') as f:
        f.write(sha256 + '\n')
    os.remove(archive)
    return target


def is_vitasdk(folder):
    return bool(folder) and os.path.isfile(os.path.join(folder, 'share', 'vita.toolchain.cmake'))


def vitasdk():
    """An installed VitaSDK, else the pinned download."""
    for folder in (os.environ.get('VITASDK'), '/usr/local/vitasdk'):
        if is_vitasdk(folder):
            return os.path.abspath(folder)
    folder = install('VitaSDK', VITASDK, VITASDK_URL, 'vitasdk-20261015', unpack_vitasdk, 'VITASDK')
    if not is_vitasdk(folder):
        sys.exit('The VitaSDK in %s is incomplete. Delete that folder and run this again.' % folder)
    return folder


def clang_version(folder):
    """Major version of the clang in folder/bin, or 0."""
    binaries = os.path.join(folder, 'bin')
    if not all(os.path.isfile(os.path.join(binaries, tool + EXE)) for tool in LLVM_TOOLS):
        return 0
    if not os.path.isfile(os.path.join(binaries, 'ld.lld' + EXE)) and not shutil.which('ld.lld'):
        return 0
    try:
        out = subprocess.run([os.path.join(binaries, 'clang' + EXE), '--version'], capture_output=True, text=True,
                             timeout=60).stdout
    except (OSError, subprocess.SubprocessError):
        return 0
    found = re.search(r'clang version (\d+)', out)
    return int(found.group(1)) if found and 'Apple' not in out else 0


def llvm():
    """An installed LLVM that is new enough, else the pinned download."""
    candidates = [os.environ.get('NFSU_LLVM')]
    on_path = shutil.which('clang')
    if on_path:
        candidates.append(os.path.dirname(os.path.dirname(os.path.realpath(on_path))))
    candidates += ['/opt/homebrew/opt/llvm', '/usr/local/opt/llvm']
    for folder in candidates:
        if folder and clang_version(folder) >= LLVM_MINIMUM:
            return os.path.abspath(folder)
    folder = install('LLVM', LLVM, LLVM_URL, 'llvm-' + LLVM_VERSION, unpack_llvm, 'NFSU_LLVM')
    if clang_version(folder) < LLVM_MINIMUM:
        sys.exit('The LLVM in %s does not run. Delete that folder and run this again.' % folder)
    return folder


if __name__ == '__main__':
    if len(sys.argv) != 2 or sys.argv[1] not in ('vitasdk', 'llvm'):
        sys.exit(__doc__)
    print(vitasdk() if sys.argv[1] == 'vitasdk' else llvm())
