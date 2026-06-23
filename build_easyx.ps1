$ErrorActionPreference = "Stop"

$project_root = Split-Path -Parent $MyInvocation.MyCommand.Path
$compiler = "C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe"
$backend_source_file = Join-Path $project_root "src\tree_visualization_model.cpp"
$frontend_source_file = Join-Path $project_root "src\easyx_frontend.cpp"
$easyx_include_directory = Join-Path $project_root "third_party\easyx\include"
$easyx_library_directory = Join-Path $project_root "third_party\easyx\lib64"
$output_directory = Join-Path $project_root "bin"
$output_file = Join-Path $output_directory "easyx_red_black_tree.exe"

if (-not (Test-Path $compiler))
{
    throw "没有找到 MinGW 编译器：$compiler"
}

if (-not (Test-Path (Join-Path $easyx_include_directory "graphics.h")))
{
    throw "项目内缺少 EasyX 头文件。"
}

if (-not (Test-Path (Join-Path $easyx_library_directory "libeasyxw.a")))
{
    throw "项目内缺少 EasyX 64 位 Unicode 静态库。"
}

if (-not (Test-Path $output_directory))
{
    New-Item -ItemType Directory -Path $output_directory | Out-Null
}

& $compiler `
    -std=c++14 `
    -Wall `
    -Wextra `
    -g `
    -DUNICODE `
    -D_UNICODE `
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
