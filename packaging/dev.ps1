param([switch]$CheckOnly)
$ErrorActionPreference='Stop'
$sourceRoot=Split-Path $PSScriptRoot -Parent
$projectRoot=Split-Path $sourceRoot -Parent
$output=Join-Path $projectRoot 'build/dev'
$hashInput=((Get-ChildItem (Join-Path $sourceRoot 'src'),(Join-Path $sourceRoot 'assets'),$PSScriptRoot -File -Recurse | Sort-Object FullName | ForEach-Object { (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }) -join '')
$hash=[Convert]::ToBase64String([Security.Cryptography.SHA256]::Create().ComputeHash([Text.Encoding]::UTF8.GetBytes($hashInput)))
$stamp=Join-Path $output 'source.stamp'
if(!(Test-Path (Join-Path $output 'Doze.exe')) -or !(Test-Path $stamp) -or (Get-Content $stamp -Raw).Trim() -ne $hash){
 & (Join-Path $PSScriptRoot 'build.ps1') -Output $output -Work (Join-Path $projectRoot 'build/native-work') -Tests
 if(!$?){exit 1}
 [IO.File]::WriteAllText($stamp,$hash)
}
if(!$CheckOnly){Start-Process -FilePath (Join-Path $output 'Doze.exe')}
