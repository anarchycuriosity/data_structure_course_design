$ErrorActionPreference = "Stop"

$project_root = Split-Path -Parent $MyInvocation.MyCommand.Path
$backend_source_file = Join-Path $project_root "src\tree_visualization_model.cpp"
$frontend_source_file = Join-Path $project_root "src\easyx_frontend.cpp"
$output_directory = Join-Path $project_root "bin"
$output_file = Join-Path $output_directory "easyx_red_black_tree.exe"

if (-not (Test-Path $output_directory))
{
    New-Item -ItemType Directory -Path $output_directory | Out-Null
}

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"

if (-not (Test-Path $vswhere))
{
    throw '没有找到 vswhere.exe。请先安装 Visual Studio 的 C++ 桌面开发组件。'
}

$visual_studio_path = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath

if (-not $visual_studio_path)
{
    throw '没有找到可用的 Visual Studio C++ 编译工具。'
}

$developer_shell = Join-Path $visual_studio_path "Common7\Tools\VsDevCmd.bat"
$batch_file = Join-Path $output_directory "build_easyx_temp.cmd"
$batch_content = @"
@call "$developer_shell" -arch=x64 -no_logo
@if errorlevel 1 exit /b 1
@cl /nologo /std:c++17 /EHsc /utf-8 /DUNICODE /D_UNICODE "$backend_source_file" "$frontend_source_file" /Fe:"$output_file"
"@

[System.IO.File]::WriteAllText($batch_file, $batch_content, [System.Text.Encoding]::ASCII)

# 某些软件会把带有特殊字符的目录塞进 PATH，进而让 VsDevCmd.bat 解析失败。
# 构建期间临时移除 QQGameTempest 路径，结束后立即恢复用户原有环境。
$original_path = $env:Path
$env:Path = (($env:Path -split ";") | Where-Object { $_ -notmatch "QQGameTempest" }) -join ";"

try
{
    cmd.exe /d /c "`"$batch_file`""
    $compile_exit_code = $LASTEXITCODE
}
finally
{
    $env:Path = $original_path
    Remove-Item -LiteralPath $batch_file -Force
}

if ($compile_exit_code -ne 0)
{
    throw '编译失败。若错误为找不到 graphics.h，请先从 EasyX 官网安装 EasyX。'
}

Write-Host "编译成功：$output_file"
