$ErrorActionPreference = "Stop"

$project_root = Split-Path -Parent $MyInvocation.MyCommand.Path
$test_program = Join-Path $project_root "bin\red_black_tree_data_tests.exe"

& (Join-Path $project_root "build_data_tests.ps1")

if (-not (Test-Path $test_program))
{
    throw "没有生成测试程序：$test_program"
}

& $test_program

if ($LASTEXITCODE -ne 0)
{
    throw "红黑树数据测试未全部通过。"
}
