$ErrorActionPreference="Stop"
$PSNativeCommandUseErrorActionPreference = $true

$file = "./test/test.c"


$out_file = "./build/test/win/obj/$file.dll"

$cargs = @()
$cargs += "-I./deps/"
$cargs += "-I./build/win/include/"
$cargs += "-I./deps/freetype/include"
$cargs += "-I./src"
$cargs += "-g"
$cargs += "-DUNICODE"
$cargs += "-D_WINDLL"
$cargs += "-Wformat"
$cargs += "-Werror=format-security"
# $cargs += "-Wno-deprecated-declarations"
$cargs += "-Wall"
$cargs += "-fno-limit-debug-info"
$cargs += "-o", $out_file
$cargs += "--shared"
$cargs += $file

clang $cargs
