param([Parameter(Mandatory=$true)][string]$Destination)
$ErrorActionPreference='Stop'
$Destination=[IO.Path]::GetFullPath($Destination)
New-Item -ItemType Directory -Path $Destination -Force | Out-Null
if(!(Test-Path (Join-Path $Destination 'zig-x86_64-windows-0.15.2/zig.exe'))){
 $index=Invoke-RestMethod 'https://ziglang.org/download/index.json'
 $zig=$index.'0.15.2'.'x86_64-windows'
 $archive=Join-Path $Destination 'zig.zip';Invoke-WebRequest $zig.tarball -OutFile $archive -UseBasicParsing
 if((Get-FileHash $archive -Algorithm SHA256).Hash.ToLower() -ne $zig.shasum){throw 'Checksum Zig non valido'}
 Expand-Archive -LiteralPath $archive -DestinationPath $Destination -Force
}
if(!(Test-Path (Join-Path $Destination 'json.hpp'))){Invoke-WebRequest 'https://raw.githubusercontent.com/nlohmann/json/v3.12.0/single_include/nlohmann/json.hpp' -OutFile (Join-Path $Destination 'json.hpp') -UseBasicParsing}
if(!(Test-Path (Join-Path $Destination 'inno/ISCC.exe'))){
 $setup=Join-Path $Destination 'innosetup-6.7.3.exe'
 Invoke-WebRequest 'https://github.com/jrsoftware/issrc/releases/download/is-6_7_3/innosetup-6.7.3.exe' -OutFile $setup -UseBasicParsing
 if((Get-AuthenticodeSignature $setup).Status -ne 'Valid'){throw 'Firma Inno Setup non valida'}
 $process=Start-Process -FilePath $setup -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/CURRENTUSER','/NOICONS',('/DIR="'+(Join-Path $Destination 'inno')+'"')) -WindowStyle Hidden -Wait -PassThru
 if($process.ExitCode){throw 'Preparazione Inno Setup non riuscita'}
}
Write-Output 'Toolchain di sviluppo disponibile. Nessun Python richiesto dal prodotto compilato.'
