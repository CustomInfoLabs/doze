param([Parameter(Mandatory=$true)][string]$Output, [string]$Work, [string]$Toolchain, [switch]$Tests, [switch]$Installer)
$ErrorActionPreference='Stop'
$sourceRoot=Split-Path $PSScriptRoot -Parent
if(!$Work){$Work=Join-Path (Split-Path $Output -Parent) 'native-work'}
if(!$Toolchain){$Toolchain=Join-Path (Split-Path (Split-Path $sourceRoot -Parent) -Parent) '.toolchains'}
$Toolchain=[IO.Path]::GetFullPath($Toolchain)
$env:ZIG_GLOBAL_CACHE_DIR=Join-Path $Toolchain 'zig-cache'
$env:ZIG_LOCAL_CACHE_DIR=Join-Path $Work 'zig-cache'
$zig=Join-Path $Toolchain 'zig-x86_64-windows-0.15.2/zig.exe'
foreach($required in @($zig,(Join-Path $Toolchain 'json.hpp'))){if(!(Test-Path -LiteralPath $required)){throw "Toolchain mancante: $required. Esegui packaging/prepare-toolchain.ps1."}}
New-Item -ItemType Directory -Path $Output,$Work -Force | Out-Null
$Output=[IO.Path]::GetFullPath($Output);$Work=[IO.Path]::GetFullPath($Work)
function Invoke-Compiler([string[]]$Arguments){ & $zig @Arguments; if($LASTEXITCODE){throw "Compilazione nativa fallita: $LASTEXITCODE"} }
$common=@('-std=c++20','-target','x86_64-windows-gnu','-fms-extensions','-O2','-static','-I',$Toolchain,'-I',(Join-Path $sourceRoot 'src'),'-I',$Work)
$metadata=Get-Content (Join-Path $sourceRoot 'project.json') -Raw | ConvertFrom-Json
$version=[string]$metadata.version
if($version -notmatch '^\d+\.\d+\.\d+$'){throw 'Versione nativa non valida: usare major.minor.patch'}
[IO.File]::WriteAllText((Join-Path $Work 'version.hpp'),('inline constexpr const char* dozeVersion="'+$version+'";'),[Text.UTF8Encoding]::new($false))
if($Tests){
 Write-Output 'Test del motore nativo (nessuna azione energetica reale).'
 Invoke-Compiler (@('c++')+$common+@((Join-Path $sourceRoot 'tests/power_tests.cpp'),'-o',(Join-Path $Work 'power-tests.exe'),'-luser32','-ladvapi32','-lpowrprof'))
 & (Join-Path $Work 'power-tests.exe');if($LASTEXITCODE){throw 'Test nativi non superati'}
}
$resources=Get-ChildItem -LiteralPath (Join-Path $sourceRoot 'assets/fonts') -File -Recurse | Sort-Object FullName
$rc=@(('1 ICON "'+(Join-Path $sourceRoot 'assets/doze-windows.ico').Replace('\','/')+'"'),('2 ICON "'+(Join-Path $sourceRoot 'assets/doze.ico').Replace('\','/')+'"'),('1 24 "'+(Join-Path $sourceRoot 'src/app.manifest').Replace('\','/')+'"'))
$header=@('#pragma once','struct EmbeddedResource { const char* name; int id; };','inline constexpr EmbeddedResource embeddedResources[] = {')
$id=100
foreach($file in $resources){$id++;$name=$file.FullName.Substring((Join-Path $sourceRoot 'assets/fonts').Length+1).Replace('\','/');$rc+= "$id RCDATA `"$($file.FullName.Replace('\','/'))`"";$header+= "{`"$name`",$id},"}
$header+='};'
$rc+=@('1 VERSIONINFO','FILEVERSION 0,1,0,0','PRODUCTVERSION 0,1,0,0','FILEFLAGSMASK 0x3fL','FILEFLAGS 0','FILEOS 0x40004L','FILETYPE 0x1L','BEGIN','BLOCK "StringFileInfo"','BEGIN','BLOCK "040904B0"','BEGIN','VALUE "CompanyName", "CustomInfo\0"','VALUE "FileDescription", "Doze - Windows power timer\0"','VALUE "FileVersion", "0.1.0\0"','VALUE "ProductName", "Doze\0"','VALUE "ProductVersion", "0.1.0\0"','VALUE "OriginalFilename", "Doze.exe\0"','END','END','BLOCK "VarFileInfo"','BEGIN','VALUE "Translation", 0x0409, 1200','END','END')
$rc=$rc | ForEach-Object { $_.Replace('0,1,0,0',($version.Replace('.',',')+',0')).Replace('0.1.0\0',($version+'\0')) }
[IO.File]::WriteAllLines((Join-Path $Work 'embedded.hpp'),$header,[Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllLines((Join-Path $Work 'app.rc'),$rc,[Text.UTF8Encoding]::new($false))
Invoke-Compiler @('rc','/c65001',('/fo'+(Join-Path $Work 'app.res')),(Join-Path $Work 'app.rc'))
Write-Output 'Compilo Doze.exe: C++ nativo, UI incorporata, nessun Python nel pacchetto.'
Invoke-Compiler (@('c++')+$common+@((Join-Path $sourceRoot 'src/main.cpp'),(Join-Path $Work 'app.res'),'-o',(Join-Path $Output 'Doze.exe'),'-municode','-Wl,--subsystem,windows','-luser32','-lgdi32','-ladvapi32','-lshell32','-lole32','-lgdiplus','-loleacc','-loleaut32','-lpowrprof','-luuid'))
if(Test-Path -LiteralPath (Join-Path $Output 'WebView2Loader.dll')){Remove-Item -LiteralPath (Join-Path $Output 'WebView2Loader.dll')}
if(Test-Path -LiteralPath (Join-Path $Output 'Doze.pdb')){Move-Item -LiteralPath (Join-Path $Output 'Doze.pdb') -Destination (Join-Path $Work 'Doze.pdb') -Force}
$notice=Get-ChildItem (Join-Path $sourceRoot 'assets/licenses') -File | Where-Object {$_.Name -notlike 'WebView2-*'} | Sort-Object Name | ForEach-Object { $_.Name+"`r`n"+('-'*60)+"`r`n"+(Get-Content -LiteralPath $_.FullName -Raw) }

if($Tests){
 Write-Output 'Verifico la UI del programma nativo con azioni simulate e preferenze isolate.'
 $testData=Join-Path $Work 'ui-test-data'
 $testReport=Join-Path $Work 'ui-test-result.json'
 if(Test-Path -LiteralPath $testReport){Remove-Item -LiteralPath $testReport}
 $process=Start-Process -FilePath (Join-Path $Output 'Doze.exe') -ArgumentList @('--test-mode',('--data-dir="'+$testData+'"'),('--smoke-report="'+$testReport+'"')) -WindowStyle Hidden -PassThru
 if(!$process.WaitForExit(45000)){Stop-Process -Id $process.Id;throw 'Timeout test UI nativa'}
 if(!(Test-Path -LiteralPath $testReport)){throw 'Report test UI non disponibile'}
 $result=Get-Content -LiteralPath $testReport -Raw | ConvertFrom-Json
 if(!$result.ok){throw ('Test UI non superato: '+($result | ConvertTo-Json -Compress))}
 Write-Output 'UI nativa verificata: lingua, tema, About, timer, annullamento e azioni simulate.'
}
if($Installer){
 Copy-Item -LiteralPath (Join-Path $sourceRoot 'packaging/README.txt') -Destination (Join-Path $Output 'README.txt') -Force
 Copy-Item -LiteralPath (Join-Path $sourceRoot 'LICENSE.txt') -Destination (Join-Path $Output 'LICENSE.txt') -Force
 $iscc=Join-Path $Toolchain 'inno/ISCC.exe'
 if(!(Test-Path -LiteralPath $iscc)){throw 'Inno Setup non disponibile nella toolchain'}
 $installerOutput=Join-Path (Split-Path $Output -Parent) 'installer'
 & $iscc ('/DPackageDir='+$Output) ('/DOutputDir='+$installerOutput) ('/DAppVersion='+$version) (Join-Path $PSScriptRoot 'doze.iss')
 if($LASTEXITCODE){throw 'Creazione installer non riuscita'}
}
Write-Output ('EXE nativo: '+(Join-Path $Output 'Doze.exe'))
