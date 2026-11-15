// Manifest lines: "<name> <sce_vp_psp2|sce_fp_psp2> <source.cg> [NAME=VALUE ...]"; "option <key> <value>" lines set
// compile options for the lines after them (keys: O, fx, fastmath, fastprecision, fastint, perfwarn). '#' starts a
// comment. The log ends with "done <ok> ok <failed> failed".

#include "shadercomp.h"
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/shacccg.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

namespace shadercomp
{

namespace
{

const char* const CompilerPaths[] = { "ur0:data/libshacccg.suprx", "ur0:data/external/libshacccg.suprx" };

FILE* s_log = nullptr;
std::string s_sourceFolder;

void logLine(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void logLine(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(s_log, fmt, args);
    va_end(args);
    fputc('\n', s_log);
    fflush(s_log);
}

bool readFile(const std::string& path, std::string& text)
{
    FILE* f = fopen(path.c_str(), "rb");
    if (!f)
        return false;
    fseek(f, 0, SEEK_END);
    const long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    text.resize(size > 0 ? size_t(size) : 0);
    const bool ok = text.empty() || fread(&text[0], 1, text.size(), f) == text.size();
    fclose(f);
    return ok;
}

bool writeFile(const std::string& path, const void* data, size_t size)
{
    FILE* f = fopen(path.c_str(), "wb");
    if (!f)
        return false;
    const bool ok = fwrite(data, 1, size, f) == size;
    return fclose(f) == 0 && ok;
}

// Sources are opened by their name relative to the source folder, as psp2cgc saw them (the name is hashed into
// the program header).
SceShaccCgSourceFile* openFile(const char* fileName, const SceShaccCgSourceLocation*, const SceShaccCgCompileOptions*,
                               const char** errorString)
{
    std::string text;
    if (!readFile(s_sourceFolder + "/" + fileName, text))
    {
        *errorString = "cannot open source file";
        return nullptr;
    }
    SceShaccCgSourceFile* file = static_cast<SceShaccCgSourceFile*>(malloc(sizeof(SceShaccCgSourceFile)));
    char* name = strdup(fileName);
    char* data = static_cast<char*>(malloc(text.size() + 1));
    memcpy(data, text.data(), text.size());
    data[text.size()] = 0;
    file->fileName = name;
    file->text = data;
    file->size = SceUInt32(text.size());
    return file;
}

void releaseFile(const SceShaccCgSourceFile* file, const SceShaccCgCompileOptions*)
{
    free(const_cast<char*>(file->fileName));
    free(const_cast<char*>(file->text));
    free(const_cast<SceShaccCgSourceFile*>(file));
}

void* allocate(unsigned int size)
{
    return malloc(size);
}

struct Options
{
    int optimization = 3;
    int fx = 0;
    int fastmath = 0;
    int fastprecision = 0;
    int fastint = 0;
    int perfwarn = 1;
};

bool setOption(Options& o, const std::string& key, int value)
{
    if (key == "O")
        o.optimization = value;
    else if (key == "fx")
        o.fx = value;
    else if (key == "fastmath")
        o.fastmath = value;
    else if (key == "fastprecision")
        o.fastprecision = value;
    else if (key == "fastint")
        o.fastint = value;
    else if (key == "perfwarn")
        o.perfwarn = value;
    else
        return false;
    return true;
}

const char* levelName(SceShaccCgDiagnosticLevel level)
{
    return level == SCE_SHACCCG_DIAGNOSTIC_LEVEL_ERROR ? "error"
         : level == SCE_SHACCCG_DIAGNOSTIC_LEVEL_WARNING ? "warning" : "info";
}

bool compile(const std::string& outFolder, const SceShaccCgCallbackList& callbacks, const Options& o,
             const std::string& name, const std::string& profile, const std::string& source,
             const std::vector<std::string>& defines)
{
    SceShaccCgCompileOptions options;
    sceShaccCgInitializeCompileOptions(&options);
    options.mainSourceFile = source.c_str();
    options.targetProfile = profile == "sce_fp_psp2" ? SCE_SHACCCG_PROFILE_FP : SCE_SHACCCG_PROFILE_VP;
    options.entryFunctionName = "main";
    std::vector<const char*> macros;
    for (const std::string& d : defines)
        macros.push_back(d.c_str());
    options.macroDefinitionCount = SceUInt32(macros.size());
    options.macroDefinitions = macros.empty() ? nullptr : macros.data();
    options.optimizationLevel = o.optimization;
    options.useFx = o.fx;
    options.useFastmath = o.fastmath;
    options.useFastprecision = o.fastprecision;
    options.useFastint = o.fastint;
    options.performanceWarnings = o.perfwarn;
    const SceShaccCgCompileOutput* output = sceShaccCgCompileProgram(&options, &callbacks, 0);
    if (!output)
    {
        logLine("failed %s: the compiler returned no output", name.c_str());
        return false;
    }
    for (SceInt32 i = 0; i < output->diagnosticCount; ++i)
    {
        const SceShaccCgDiagnosticMessage& d = output->diagnostics[i];
        const unsigned line = d.location ? d.location->lineNumber : 0;
        logLine("  %s %s:%u: %s", levelName(d.level), source.c_str(), line, d.message ? d.message : "");
    }
    bool ok = output->programData && output->programSize;
    if (ok)
    {
        ok = writeFile(outFolder + "/" + name + ".gxp", output->programData, output->programSize);
        logLine(ok ? "ok %s %u" : "failed %s: writing %u bytes", name.c_str(), unsigned(output->programSize));
    }
    else
        logLine("failed %s", name.c_str());
    sceShaccCgDestroyCompileOutput(output);
    return ok;
}

}

bool runIfRequested(const char* folder)
{
    const std::string base(folder);
    std::string manifest;
    if (!readFile(base + "/compile.txt", manifest))
        return false;
    s_log = fopen((base + "/compile.log").c_str(), "w");
    if (!s_log)
        return true;
    s_sourceFolder = base + "/src";
    const std::string outFolder = base + "/out";
    sceIoMkdir(outFolder.c_str(), 0777);

    SceUID module = -1;
    for (const char* path : CompilerPaths)
    {
        module = sceKernelLoadStartModule(path, 0, nullptr, 0, nullptr, nullptr);
        if (module >= 0)
        {
            logLine("compiler: %s", path);
            break;
        }
    }
    int ok = 0, failed = 0;
    if (module < 0)
        logLine("error: libshacccg.suprx not found in ur0:data/ (0x%08x); install it with ShaRKBR33D", unsigned(module));
    else
    {
        logLine("version: %s", sceShaccCgGetVersionString());
        sceShaccCgSetDefaultAllocator(allocate, free);
        SceShaccCgCallbackList callbacks;
        sceShaccCgInitializeCallbackList(&callbacks, SCE_SHACCCG_TRIVIAL);
        callbacks.openFile = openFile;
        callbacks.releaseFile = releaseFile;
        Options options;
        std::istringstream lines(manifest);
        std::string line;
        while (std::getline(lines, line))
        {
            const size_t hash = line.find('#');
            if (hash != std::string::npos)
                line.resize(hash);
            std::istringstream words(line);
            std::string first, second, third;
            if (!(words >> first))
                continue;
            if (first == "option")
            {
                int value = 0;
                if (!(words >> second >> value) || !setOption(options, second, value))
                    logLine("error: bad option line: %s", line.c_str());
                continue;
            }
            if (!(words >> second >> third))
            {
                logLine("error: bad line: %s", line.c_str());
                ++failed;
                continue;
            }
            std::vector<std::string> defines;
            for (std::string d; words >> d;)
                defines.push_back(d);
            if (compile(outFolder, callbacks, options, first, second, third, defines))
                ++ok;
            else
                ++failed;
        }
        sceShaccCgReleaseCompiler();
    }
    logLine("done %d ok %d failed", ok, failed);
    fclose(s_log);
    s_log = nullptr;
    sceIoRemove((base + "/compile.done").c_str());
    sceIoRename((base + "/compile.txt").c_str(), (base + "/compile.done").c_str());
    return true;
}

}
