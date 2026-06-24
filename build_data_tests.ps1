param(
    [string]$CompilerPath = $env:EASYX_GXX
)

$ErrorActionPreference = "Stop"

$project_root = Split-Path -Parent $MyInvocation.MyCommand.Path
$source_file = Join-Path $project_root "src\main.cpp"
$output_directory = Join-Path $project_root "bin"
$output_file = Join-Path $output_directory "red_black_tree_data_tests.exe"

function Find-GnuCompiler
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
        "C:\Program Files\mingw64\bin\g++.exe"
    )

    $path_compiler = Get-Command g++.exe -ErrorAction SilentlyContinue

    if ($path_compiler)
    {
        $candidates += $path_compiler.Source
    }

    foreach ($candidate in ($candidates | Select-Object -Unique))
    {
        if (Test-Path $candidate)
        {
            return $candidate
        }
    }

    throw "没有找到 g++.exe，无法构建红黑树数据测试。"
}

$compiler = Find-GnuCompiler -RequestedCompiler $CompilerPath

if (-not (Test-Path $output_directory))
{
    New-Item -ItemType Directory -Path $output_directory | Out-Null
}

Write-Host "使用编译器：$compiler"

& $compiler `
    -std=gnu++14 `
    -DDEBUG `
    -Wall `
    -Wextra `
    $source_file `
    "-Wl,--stack,33554432" `
    -o $output_file

if ($LASTEXITCODE -ne 0)
{
    throw "红黑树数据测试构建失败。"
}

Write-Host "红黑树数据测试构建成功：$output_file"
