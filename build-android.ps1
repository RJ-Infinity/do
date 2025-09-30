$ErrorActionPreference="Stop"
$PSNativeCommandUseErrorActionPreference = $true

$ENV:ANDROID_SYSROOT = "$ENV:ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/windows-x86_64/sysroot"
$PACKAGE = "com.rjinfinity.todo"


.\sokol-shdc.exe --input .\src\text.glsl --output .\build\android\include\shader\text.h --slang glsl300es

&$ENV:ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/windows-x86_64/bin/clang `
--target=aarch64-linux-android33 `
--sysroot=$ENV:ANDROID_SYSROOT `
-g `
-DANDROID `
-D__ANDROID__ `
-fdata-sections `
-ffunction-sections `
-funwind-tables `
-fstack-protector-strong `
-no-canonical-prefixes `
-D__BIONIC_NO_PAGE_SIZE_MACRO `
-D_FORTIFY_SOURCE=2 `
-Wformat `
-Werror=format-security `
-fno-limit-debug-info `
-fPIC `
-o ./build/android/obj/android_native_app_glue.c.o `
-c $ENV:ANDROID_NDK_ROOT/sources/android/native_app_glue/android_native_app_glue.c

$freetype_files =
"base/ftsystem.c",
"base/ftinit.c",
"base/ftdebug.c",
"base/ftbase.c",
"base/ftbbox.c",
"base/ftglyph.c",
"base/ftbdf.c",
"base/ftbitmap.c",
"base/ftcid.c",
"base/ftfstype.c",
"base/ftgasp.c",
"base/ftgxval.c",
"base/ftmm.c",
"base/ftotval.c",
"base/ftpatent.c",
"base/ftpfr.c",
"base/ftstroke.c",
"base/ftsynth.c",
"base/fttype1.c",
"base/ftwinfnt.c",
"bdf/bdf.c",
"cff/cff.c",
"cid/type1cid.c",
"pcf/pcf.c",
"pfr/pfr.c",
"sfnt/sfnt.c",
"truetype/truetype.c",
"type1/type1.c",
"type42/type42.c",
"winfonts/winfnt.c",
"smooth/smooth.c",
"raster/raster.c",
"sdf/sdf.c",
"autofit/autofit.c",
"cache/ftcache.c",
"gzip/ftgzip.c",
"lzw/ftlzw.c",
"bzip2/ftbzip2.c",
"gxvalid/gxvalid.c",
"otvalid/otvalid.c",
"psaux/psaux.c",
"pshinter/pshinter.c",
"psnames/psnames.c",
"svg/ftsvg.c"

$files = "./src/main.c", "./src/layout.c", "./deps/glad.c"
$files += $freetype_files | ForEach-Object {"./deps/freetype/src/"+$_}

foreach ($file in $files){
	$out_file = "./build/android/obj/$file.o"
	$file_time = (Get-Item $file).LastWriteTime
	try{
		$out_file_time = (Get-Item $out_file).LastWriteTime
	}catch{
		$out_file_time = 0;
	}

	if ($file_time -eq $out_file_time){continue}

	echo compiling $file

	md -Force $(Split-Path ./build/android/obj/$file) > $null
	$cargs = @()
	$cargs += "--target=aarch64-linux-android33"
	$cargs += "--sysroot=$ENV:ANDROID_SYSROOT"
	$cargs += "-Dnative_activity_EXPORTS"
	$cargs += "-I$ENV:ANDROID_NDK_ROOT/sources/android/native_app_glue"
	$cargs += "-I./deps/"
	$cargs += "-I./build/android/include/"
	$cargs += "-I./deps/freetype/include"
	$cargs += "-g"
	$cargs += "-DANDROID"
	$cargs += "-D__ANDROID__"
	if ($file.StartsWith("./deps/freetype/src/")) {
		$cargs += "-DFT2_BUILD_LIBRARY"
	}
	$cargs += "-fdata-sections"
	$cargs += "-ffunction-sections"
	$cargs += "-funwind-tables"
	$cargs += "-fstack-protector-strong"
	$cargs += "-no-canonical-prefixes"
	$cargs += "-D__BIONIC_NO_PAGE_SIZE_MACRO"
	$cargs += "-D_FORTIFY_SOURCE=2"
	$cargs += "-Wformat"
	$cargs += "-Werror=format-security"
	$cargs += "-Wall"
	$cargs += "-fno-limit-debug-info"
	$cargs += "-fPIC"
	$cargs += "-o", $out_file
	$cargs += "-c", $file
	&$ENV:ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/windows-x86_64/bin/clang $cargs

	(Get-Item $out_file).LastWriteTime = $file_time
}

&$ENV:ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-ar qc ./build/android/obj/libandroid_native_app_glue.a ./build/android/obj/android_native_app_glue.c.o

&$ENV:ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-ranlib ./build/android/obj/libandroid_native_app_glue.a


$linker_args = @()
$linker_args += "--sysroot=$ENV:ANDROID_SYSROOT"
$linker_args += "-z", "now"
$linker_args += "-z", "relro"
$linker_args += "--hash-style=gnu"
$linker_args += "--eh-frame-hdr"
$linker_args += "-m", "aarch64linux"
$linker_args += "-shared"
$linker_args += "-o", "./build/android/pkg/lib/arm64-v8a/libapp.so"
$linker_args += "$ENV:ANDROID_SYSROOT/usr/lib/aarch64-linux-android/33/crtbegin_so.o"
$linker_args += "-u", "ANativeActivity_onCreate"
$linker_args += "-L$ENV:ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/windows-x86_64/lib/clang/19/lib/linux/aarch64"
$linker_args += "-L$ENV:ANDROID_SYSROOT/usr/lib/aarch64-linux-android/33"
$linker_args += "-L$ENV:ANDROID_SYSROOT/usr/lib/aarch64-linux-android"
$linker_args += "-L$ENV:ANDROID_SYSROOT/usr/lib"
$linker_args += "-z", "max-page-size=16384"
$linker_args += "--build-id=sha1"
$linker_args += "--no-rosegment"
$linker_args += "--no-undefined-version"
$linker_args += "--fatal-warnings"
$linker_args += "--no-undefined"
$linker_args += "-soname", "libapp.so"
foreach ($file in $files){
	$linker_args += "./build/android/obj/$file.o"
}
$linker_args += "-landroid"
$linker_args += "./build/android/obj/libandroid_native_app_glue.a"
$linker_args += "-lEGL"
$linker_args += "-lGLESv1_CM"
$linker_args += "-llog"
$linker_args += "-latomic"
$linker_args += "-lm"
$linker_args += "-Bstatic"
$linker_args += "-lc++"
$linker_args += "-Bdynamic"
$linker_args += "-lm"
$linker_args += "$ENV:ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/windows-x86_64/lib/clang/19/lib/linux/libclang_rt.builtins-aarch64-android.a"
$linker_args += "-l:libunwind.a"
$linker_args += "-ldl"
$linker_args += "-lc"
$linker_args += "$ENV:ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/windows-x86_64/lib/clang/19/lib/linux/libclang_rt.builtins-aarch64-android.a"
$linker_args += "-l:libunwind.a"
$linker_args += "-ldl"
$linker_args += "$ENV:ANDROID_SYSROOT/usr/lib/aarch64-linux-android/33/crtend_so.o"

&$ENV:ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/windows-x86_64/bin/ld.lld $linker_args

&$ENV:ANDROID_SDK_ROOT/build-tools/36.0.0/aapt package -f --debug-mode -M ./android/AndroidManifest.xml -S ./android/res -I $ENV:ANDROID_SDK_ROOT/platforms/android-34/android.jar -F ./build/android/apk/$PACKAGE.unsigned.apk ./build/android/pkg/

&$ENV:ANDROID_SDK_ROOT/build-tools/36.0.0/zipalign -f -p 4 ./build/android/apk/$PACKAGE.unsigned.apk ./build/android/apk/$PACKAGE.aligned.apk

&$ENV:ANDROID_SDK_ROOT/build-tools/36.0.0/apksigner.bat sign --ks ./keystore.jks --ks-key-alias androidkey --ks-pass pass:android --key-pass pass:android --out ./build/android/apk/$PACKAGE.apk ./build/android/apk/$PACKAGE.aligned.apk

adb install -r ./build/android/apk/$PACKAGE.apk

adb shell am start -n $PACKAGE/android.app.NativeActivity

