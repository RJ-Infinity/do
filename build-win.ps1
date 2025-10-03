$ErrorActionPreference="Stop"
$PSNativeCommandUseErrorActionPreference = $true

./sokol-shdc.exe --input ./src/text.glsl --output ./build/win/include/shader/text.h --slang glsl430

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

$files = "./src/main.c", "./src/layout.c", "./deps/glad.c", "./src/thing.c"
$files += $freetype_files | ForEach-Object {"./deps/freetype/src/"+$_}

foreach ($file in $files){
	$out_file = "./build/win/obj/$file.o"
	$file_time = (Get-Item $file).LastWriteTime
	try{
		$out_file_time = (Get-Item $out_file).LastWriteTime
	}catch{
		$out_file_time = 0;
	}

	# if ($file_time -eq $out_file_time){continue}
	if (($out_file_time -gt 0) -and ($file.StartsWith("./deps"))){continue}

	echo compiling $file

	md -Force $(Split-Path ./build/win/obj/$file) > $null
	$cargs = @()
	$cargs += "-I./deps/"
	$cargs += "-I./build/win/include/"
	$cargs += "-I./deps/freetype/include"
	$cargs += "-g"
	if ($file.StartsWith("./deps/freetype/src/")) {
		$cargs += "-DFT2_BUILD_LIBRARY"
	}
	$cargs += "-DUNICODE"
	$cargs += "-Wformat"
	$cargs += "-Werror=format-security"
	# $cargs += "-Wno-deprecated-declarations"
	$cargs += "-Wall"
	$cargs += "-fno-limit-debug-info"
	$cargs += "-o", $out_file
	$cargs += "-c", $file
	clang $cargs

	(Get-Item $out_file).LastWriteTime = $file_time
}

$clang_path = (Get-Command clang).Source + "/../../"

$linker_args = @()
$linker_args += "-o", "./build/win/do.exe"
$linker_args += "--fatal-warnings"
$linker_args += "--no-undefined"
foreach ($file in $files){
	$linker_args += "./build/win/obj/$file.o"
}
$linker_args += "$clang_path/x86_64-w64-mingw32/lib/crt2.o"
# $linker_args += "$clang_path/lib/gcc/x86_64-w64-mingw32/13.2.0/crtbegin.o"
$linker_args += "-L$clang_path/lib/gcc/x86_64-w64-mingw32/13.2.0"
$linker_args += "-L$clang_path/x86_64-w64-mingw32/lib"
$linker_args += "-L$clang_path/x86_64-w64-mingw32/mingw/lib"
$linker_args += "-L$clang_path/lib"

$linker_args += "-lmingw32"
$linker_args += "-lgcc"
$linker_args += "-lgcc_eh"
$linker_args += "-lmoldname"
$linker_args += "-lmingwex"
$linker_args += "-lmsvcrt"
$linker_args += "-lgdi32"
$linker_args += "-ladvapi32"
$linker_args += "-lshell32"
$linker_args += "-lOpengl32"
$linker_args += "-luser32"
$linker_args += "-lkernel32"

ld.lld $linker_args

./build/win/do.exe
