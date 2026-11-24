/*
 * Lazy fontconfig bindings for the Linux Avalonia templates.
 *
 * Skia's font manager calls fontconfig. Linking -lfontconfig would add libfontconfig.so.1 to DT_NEEDED,
 * so a host without fontconfig could not start at all. These definitions resolve each function from
 * libfontconfig.so.1 on first use instead. The module initializer checks that the library loads before it
 * binds SkiaSharp to the executable, so without fontconfig only SkiaSharp fails, with DllNotFoundException,
 * as it did with the dynamic libSkiaSharp.so.
 *
 * The version scripts keep these definitions local, so they never interpose on the real library.
 * A function Skia references but this file lacks fails the link as an undefined symbol.
 */
#include <dlfcn.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <fontconfig/fontconfig.h>

#define HOSTFORGE_FONTCONFIG_SONAME "libfontconfig.so.1"

static void *hostforge_fontconfig_handle(void)
{
    static void *handle;
    void *current = __atomic_load_n(&handle, __ATOMIC_ACQUIRE);
    if (current == NULL)
    {
        current = dlopen(HOSTFORGE_FONTCONFIG_SONAME, RTLD_NOW | RTLD_LOCAL);
        if (current == NULL)
        {
            const char *error = dlerror();
            fprintf(stderr, "HostForge: cannot load %s: %s\n", HOSTFORGE_FONTCONFIG_SONAME,
                    error != NULL ? error : "unknown error");
            abort();
        }
        void *expected = NULL;
        if (!__atomic_compare_exchange_n(&handle, &expected, current, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
        {
            /* Another thread stored its handle first; dlopen counts references, so release ours. */
            dlclose(current);
            current = expected;
        }
    }
    return current;
}

static void *hostforge_fontconfig_symbol(void **slot, const char *name)
{
    void *function = __atomic_load_n(slot, __ATOMIC_ACQUIRE);
    if (function == NULL)
    {
        function = dlsym(hostforge_fontconfig_handle(), name);
        if (function == NULL)
        {
            const char *error = dlerror();
            fprintf(stderr, "HostForge: %s does not export %s: %s\n", HOSTFORGE_FONTCONFIG_SONAME, name,
                    error != NULL ? error : "unknown error");
            abort();
        }
        __atomic_store_n(slot, function, __ATOMIC_RELEASE);
    }
    return function;
}

/* The declarations in fontconfig.h check every signature below. */
#define HOSTFORGE_FC(ret, name, params, args)                                                    \
    ret name params                                                                              \
    {                                                                                            \
        static void *slot;                                                                       \
        return ((ret(*) params)hostforge_fontconfig_symbol(&slot, #name)) args;                  \
    }

#define HOSTFORGE_FC_VOID(name, params, args)                                                    \
    void name params                                                                             \
    {                                                                                            \
        static void *slot;                                                                       \
        ((void(*) params)hostforge_fontconfig_symbol(&slot, #name)) args;                        \
    }

HOSTFORGE_FC(FcBool, FcCharSetAddChar, (FcCharSet *fcs, FcChar32 ucs4), (fcs, ucs4))
HOSTFORGE_FC(FcCharSet *, FcCharSetCreate, (void), ())
HOSTFORGE_FC_VOID(FcCharSetDestroy, (FcCharSet *fcs), (fcs))
HOSTFORGE_FC(FcBool, FcCharSetHasChar, (const FcCharSet *fcs, FcChar32 ucs4), (fcs, ucs4))
HOSTFORGE_FC(FcConfig *, FcConfigCreate, (void), ())
HOSTFORGE_FC_VOID(FcConfigDestroy, (FcConfig *config), (config))
HOSTFORGE_FC(FcFontSet *, FcConfigGetFonts, (FcConfig *config, FcSetName set), (config, set))
HOSTFORGE_FC(const FcChar8 *, FcConfigGetSysRoot, (const FcConfig *config), (config))
HOSTFORGE_FC(FcConfig *, FcConfigReference, (FcConfig *config), (config))
HOSTFORGE_FC(FcBool, FcConfigSubstitute, (FcConfig *config, FcPattern *p, FcMatchKind kind), (config, p, kind))
HOSTFORGE_FC_VOID(FcDefaultSubstitute, (FcPattern *pattern), (pattern))
HOSTFORGE_FC(FcPattern *, FcFontMatch, (FcConfig *config, FcPattern *p, FcResult *result), (config, p, result))
HOSTFORGE_FC(FcPattern *, FcFontRenderPrepare, (FcConfig *config, FcPattern *pat, FcPattern *font),
             (config, pat, font))
HOSTFORGE_FC(FcBool, FcFontSetAdd, (FcFontSet *s, FcPattern *font), (s, font))
HOSTFORGE_FC(FcFontSet *, FcFontSetCreate, (void), ())
HOSTFORGE_FC_VOID(FcFontSetDestroy, (FcFontSet *s), (s))
HOSTFORGE_FC(FcPattern *, FcFontSetMatch,
             (FcConfig *config, FcFontSet **sets, int nsets, FcPattern *p, FcResult *result),
             (config, sets, nsets, p, result))
HOSTFORGE_FC(FcFontSet *, FcFontSort, (FcConfig *config, FcPattern *p, FcBool trim, FcCharSet **csp, FcResult *result),
             (config, p, trim, csp, result))
HOSTFORGE_FC(int, FcGetVersion, (void), ())
HOSTFORGE_FC(FcConfig *, FcInitLoadConfigAndFonts, (void), ())
HOSTFORGE_FC(FcBool, FcLangSetAdd, (FcLangSet *ls, const FcChar8 *lang), (ls, lang))
HOSTFORGE_FC(FcLangSet *, FcLangSetCreate, (void), ())
HOSTFORGE_FC_VOID(FcLangSetDestroy, (FcLangSet *ls), (ls))
HOSTFORGE_FC(FcLangResult, FcLangSetHasLang, (const FcLangSet *ls, const FcChar8 *lang), (ls, lang))
HOSTFORGE_FC(FcObjectSet *, FcObjectSetVaBuild, (const char *first, va_list va), (first, va))
HOSTFORGE_FC_VOID(FcObjectSetDestroy, (FcObjectSet *os), (os))
HOSTFORGE_FC(FcBool, FcPatternAddBool, (FcPattern *p, const char *object, FcBool b), (p, object, b))
HOSTFORGE_FC(FcBool, FcPatternAddCharSet, (FcPattern *p, const char *object, const FcCharSet *c), (p, object, c))
HOSTFORGE_FC(FcBool, FcPatternAddInteger, (FcPattern *p, const char *object, int i), (p, object, i))
HOSTFORGE_FC(FcBool, FcPatternAddLangSet, (FcPattern *p, const char *object, const FcLangSet *ls), (p, object, ls))
HOSTFORGE_FC(FcBool, FcPatternAddString, (FcPattern *p, const char *object, const FcChar8 *s), (p, object, s))
HOSTFORGE_FC(FcBool, FcPatternAddWeak, (FcPattern *p, const char *object, FcValue value, FcBool append),
             (p, object, value, append))
HOSTFORGE_FC(FcPattern *, FcPatternCreate, (void), ())
HOSTFORGE_FC_VOID(FcPatternDestroy, (FcPattern *p), (p))
HOSTFORGE_FC(FcPattern *, FcPatternDuplicate, (const FcPattern *p), (p))
HOSTFORGE_FC(FcBool, FcPatternEqual, (const FcPattern *pa, const FcPattern *pb), (pa, pb))
HOSTFORGE_FC(FcPattern *, FcPatternFilter, (FcPattern *p, const FcObjectSet *os), (p, os))
HOSTFORGE_FC(FcResult, FcPatternGet, (const FcPattern *p, const char *object, int id, FcValue *v), (p, object, id, v))
HOSTFORGE_FC(FcResult, FcPatternGetBool, (const FcPattern *p, const char *object, int n, FcBool *b), (p, object, n, b))
HOSTFORGE_FC(FcResult, FcPatternGetCharSet, (const FcPattern *p, const char *object, int n, FcCharSet **c),
             (p, object, n, c))
HOSTFORGE_FC(FcResult, FcPatternGetInteger, (const FcPattern *p, const char *object, int n, int *i), (p, object, n, i))
HOSTFORGE_FC(FcResult, FcPatternGetLangSet, (const FcPattern *p, const char *object, int n, FcLangSet **ls),
             (p, object, n, ls))
HOSTFORGE_FC(FcResult, FcPatternGetMatrix, (const FcPattern *p, const char *object, int n, FcMatrix **s),
             (p, object, n, s))
HOSTFORGE_FC(FcResult, FcPatternGetString, (const FcPattern *p, const char *object, int n, FcChar8 **s),
             (p, object, n, s))
HOSTFORGE_FC(FcResult, FcPatternGetWithBinding,
             (const FcPattern *p, const char *object, int id, FcValue *v, FcValueBinding *b), (p, object, id, v, b))
HOSTFORGE_FC_VOID(FcPatternReference, (FcPattern *p), (p))
HOSTFORGE_FC(FcBool, FcPatternRemove, (FcPattern *p, const char *object, int id), (p, object, id))
HOSTFORGE_FC(int, FcStrCmpIgnoreCase, (const FcChar8 *s1, const FcChar8 *s2), (s1, s2))

/* A variadic function cannot forward its arguments, so build the set through the va_list entry point. */
FcObjectSet *FcObjectSetBuild(const char *first, ...)
{
    va_list va;
    va_start(va, first);
    FcObjectSet *set = FcObjectSetVaBuild(first, va);
    va_end(va);
    return set;
}
