$ErrorActionPreference = "Stop"

$project_root = Split-Path -Parent $MyInvocation.MyCommand.Path
$release_program = Join-Path $project_root "release\easyx_red_black_tree.exe"

if (-not (Test-Path $release_program))
{
    throw "缺少预编译程序：$release_program。请确认仓库文件已完整克隆。"
}

Start-Process -FilePath $release_program -WorkingDirectory $project_root
