param(
    [string]$CompilerPath = $env:EASYX_GXX
)

$ErrorActionPreference = "Stop"

$project_root = Split-Path -Parent $MyInvocation.MyCommand.Path
$backend_source_file = Join-Path $project_root "src\tree_visualization_model.cpp"
$frontend_source_file = Join-Path $project_root "src\easyx_frontend.cpp"
$easyx_include_directory = Join-Path $project_root "third_party\easyx\include"
$easyx_library_directory = Join-Path $project_root "third_party\easyx\lib64"
$output_directory = Join-Path $project_root "bin"
$output_file = Join-Path $output_directory "easyx_red_black_tree.exe"

function Find-CompatibleCompiler
{
    param(
        [string]$RequestedCompiler
    )

    $candidates = @()

    if ($RequestedCompiler)
    {
        $candidates += $RequestedCompiler
    }

    $candidates += @(
        "C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe",
        "C:\Program Files\Dev-Cpp\MinGW64\bin\g++.exe",
        "C:\Dev-Cpp\MinGW64\bin\g++.exe",
        "C:\TDM-GCC-64\bin\g++.exe"
    )

    $path_compiler = Get-Command g++.exe -ErrorAction SilentlyContinue

    if ($path_compiler)
    {
        $candidates += $path_compiler.Source
    }

    foreach ($candidate in ($candidates | Select-Object -Unique))
    {
        if (-not (Test-Path $candidate))
        {
            continue
        }

        $previous_error_action = $ErrorActionPreference
        $ErrorActionPreference = "Continue"
        $version_information = (& $candidate -v 2>&1 | Out-String)
        $ErrorActionPreference = $previous_error_action

        # EasyX 官方 MinGW 库使用 MSVCRT，不能与 UCRT 工具链链接。
        if ($version_information -match "ucrt")
        {
            Write-Host "跳过 UCRT 编译器：$candidate"
            continue
        }

        return $candidate
    }

    throw @"
没有找到与 EasyX 兼容的 64 位 MSVCRT MinGW。

解决方法：
1. 安装带 TDM-GCC 4.9.2 的 Dev-C++ 5.11；或
2. 设置环境变量 EASYX_GXX，指向兼容编译器的 g++.exe。

当前官网 EasyX MinGW 库不支持 UCRT MinGW。
"@
}

$compiler = Find-CompatibleCompiler -RequestedCompiler $CompilerPath

if (-not (Test-Path (Join-Path $easyx_include_directory "graphics.h")))
{
    throw "项目内缺少 EasyX 头文件。请确认 Git 已完整克隆 third_party/easyx。"
}

if (-not (Test-Path (Join-Path $easyx_library_directory "libeasyxw.a")))
{
    throw "项目内缺少 EasyX 64 位 Unicode 静态库。"
}

if (-not (Test-Path $output_directory))
{
    New-Item -ItemType Directory -Path $output_directory | Out-Null
}

Write-Host "使用编译器：$compiler"

& $compiler `
    -std=c++14 `
    -Wall `
    -Wextra `
    -g `
    -DUNICODE `
    -D_UNICODE `
    -static-libgcc `
    -static-libstdc++ `
    $backend_source_file `
    $frontend_source_file `
    "-I$easyx_include_directory" `
    "-L$easyx_library_directory" `
    -leasyxw `
    -lgdi32 `
    -lole32 `
    -luuid `
    -o $output_file

if ($LASTEXITCODE -ne 0)
{
    throw "MinGW 编译或链接失败。"
}

Write-Host "编译成功：$output_file"
