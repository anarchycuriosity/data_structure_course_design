$ErrorActionPreference = "Stop"

$project_root = Split-Path -Parent $MyInvocation.MyCommand.Path
$test_program = Join-Path $project_root "bin\reference_tree_tests.exe"

& (Join-Path $project_root "build_reference_tests.ps1")

if (-not (Test-Path $test_program))
{
    throw "没有生成测试程序：$test_program"
}

& $test_program

if ($LASTEXITCODE -ne 0)
{
    throw "参考项目测试未全部通过。"
}
