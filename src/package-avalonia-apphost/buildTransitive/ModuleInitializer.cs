using System;
using System.IO;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Threading;

namespace HostForge.Avalonia.AppHost;

// The build defines HOSTFORGE_<ASSEMBLY> for each assembly registered below that the project references.
// The static type references load no assembly the application lacks, and trimming keeps the assemblies they name.
internal static class ModuleInitializer
{
    private static readonly IntPtr MainProgram = NativeLibrary.GetMainProgramHandle();

    [ModuleInitializer]
    public static void Init()
    {
        // The application assembly also runs under other hosts, such as dotnet, a test host or the previewer.
        // Their main program exports none of the libraries, which then load from their files as usual.
        if (Exports("sk_version_get_milestone"))
            RegisterSkiaSharp();
        if (Exports("hb_version_string"))
            RegisterHarfBuzzSharp();
        if (Exports("EGL_GetError"))
            RegisterAngle();
        if (Exports("CreateAvaloniaNative"))
            RegisterAvaloniaNative();
    }

    // The registrations are separate methods so that the JIT loads their assemblies only when the
    // executable exports the library.
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static void RegisterSkiaSharp()
    {
#if HOSTFORGE_SKIASHARP
        SetResolver(typeof(global::SkiaSharp.SkiaSharpVersion).Assembly, "libSkiaSharp", requiresFontconfig: true);
#endif
        // Skottie, SceneGraph and Resources import libSkiaSharp themselves; the templates export their functions too.
#if HOSTFORGE_SKIASHARP_SKOTTIE
        SetResolver(typeof(global::SkiaSharp.Skottie.Animation).Assembly, "libSkiaSharp", requiresFontconfig: true);
#endif
#if HOSTFORGE_SKIASHARP_SCENEGRAPH
        SetResolver(typeof(global::SkiaSharp.SceneGraph.InvalidationController).Assembly, "libSkiaSharp", requiresFontconfig: true);
#endif
#if HOSTFORGE_SKIASHARP_RESOURCES
        SetResolver(typeof(global::SkiaSharp.Resources.ResourceProvider).Assembly, "libSkiaSharp", requiresFontconfig: true);
#endif
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static void RegisterHarfBuzzSharp()
    {
#if HOSTFORGE_HARFBUZZSHARP
        SetResolver(typeof(global::HarfBuzzSharp.Blob).Assembly, "libHarfBuzzSharp");
#endif
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static void RegisterAngle()
    {
#if HOSTFORGE_AVALONIA_WIN32
        SetResolver(typeof(global::Avalonia.Win32PlatformOptions).Assembly, "av_libglesv2");
#endif
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static void RegisterAvaloniaNative()
    {
#if HOSTFORGE_AVALONIA_NATIVE
        SetResolver(typeof(global::Avalonia.AvaloniaNativePlatformOptions).Assembly, "libAvaloniaNative");
#endif
    }

    private static bool Exports(string symbol) => NativeLibrary.TryGetExport(MainProgram, symbol, out _);

    // The Linux templates bind fontconfig lazily, so the executable starts without it. Skia needs it once it
    // runs, so without fontconfig SkiaSharp fails to load, as the dynamic libSkiaSharp.so did, instead of
    // terminating the process at its first font call. The result of the first check is kept.
    private static int s_fontconfigState;

    private static void RequireFontconfig()
    {
        if (!OperatingSystem.IsLinux())
            return;
        var state = Volatile.Read(ref s_fontconfigState);
        if (state == 0)
        {
            state = NativeLibrary.TryLoad("libfontconfig.so.1", out _) ? 1 : 2;
            Volatile.Write(ref s_fontconfigState, state);
        }
        if (state == 2)
            throw new DllNotFoundException(
                "Unable to load libSkiaSharp from the executable: it requires libfontconfig.so.1, which could not be loaded. Install fontconfig.");
    }

    private static void SetResolver(Assembly assembly, string library, bool requiresFontconfig = false)
    {
        try
        {
            NativeLibrary.SetDllImportResolver(assembly, (libraryName, _, _) =>
            {
                if (!string.Equals(Path.GetFileNameWithoutExtension(libraryName), library, StringComparison.OrdinalIgnoreCase))
                    return IntPtr.Zero;
                if (requiresFontconfig)
                    RequireFontconfig();
                return MainProgram;
            });
        }
        catch (InvalidOperationException)
        {
            // Respect a resolver already installed by the application.
        }
    }
}
