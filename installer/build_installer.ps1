# Arma el instalador de Windows de Inventario de punta a punta:
# compila en Release, empaqueta las dependencias de Qt, arma una carpeta
# de distribucion limpia y genera el .exe instalador con Inno Setup.
#
# Uso: powershell -File installer\build_installer.ps1

param(
    [string]$QtDir = "C:\Qt\6.10.3\msvc2022_64",
    [string]$InnoSetupCompiler = "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe",
    [string]$Version = "0.1.0"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot "build-release"
$distDir = Join-Path $repoRoot "dist"

Write-Host "== Compilando Inventario (Release) ==" -ForegroundColor Cyan
cmake -S $repoRoot -B $buildDir -G Ninja -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { throw "Fallo la configuracion de CMake." }
cmake --build $buildDir --target Inventario
if ($LASTEXITCODE -ne 0) { throw "Fallo la compilacion." }

Write-Host "== Empaquetando dependencias de Qt (windeployqt) ==" -ForegroundColor Cyan
& "$QtDir\bin\windeployqt.exe" --release --no-translations --no-opengl-sw "$buildDir\Inventario.exe"
if ($LASTEXITCODE -ne 0) { throw "Fallo windeployqt." }

# Solo usamos SQLite: quitar los demas drivers de base de datos que
# windeployqt agrega por si acaso.
$sqlDrivers = Join-Path $buildDir "sqldrivers"
if (Test-Path $sqlDrivers) {
    Get-ChildItem $sqlDrivers -Filter "*.dll" | Where-Object { $_.Name -ne "qsqlite.dll" } | Remove-Item -Force
}

Write-Host "== Armando carpeta de distribucion limpia ==" -ForegroundColor Cyan
if (Test-Path $distDir) { Remove-Item $distDir -Recurse -Force }
New-Item -ItemType Directory -Path $distDir | Out-Null

Copy-Item (Join-Path $buildDir "Inventario.exe") $distDir
Get-ChildItem $buildDir -Filter "*.dll" | ForEach-Object { Copy-Item $_.FullName $distDir }

foreach ($sub in @("platforms", "sqldrivers", "styles", "imageformats", "iconengines", "generic", "networkinformation", "tls")) {
    $src = Join-Path $buildDir $sub
    if (Test-Path $src) {
        Copy-Item $src (Join-Path $distDir $sub) -Recurse
    }
}

Write-Host "== Generando el instalador con Inno Setup ==" -ForegroundColor Cyan
if (-not (Test-Path $InnoSetupCompiler)) {
    throw "No se encontro el compilador de Inno Setup en: $InnoSetupCompiler"
}
& $InnoSetupCompiler "/DMyAppVersion=$Version" (Join-Path $repoRoot "installer\setup.iss")
if ($LASTEXITCODE -ne 0) { throw "Fallo Inno Setup." }

Write-Host "== Listo: instalador generado en installer\output ==" -ForegroundColor Green
