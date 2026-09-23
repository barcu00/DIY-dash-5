param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
)

$ErrorActionPreference = "Stop"
$module = Join-Path $ProjectRoot "hardware/can-io-module"
$output = Join-Path $module "manufacturing/generated"
$resolvedModule = (Resolve-Path -LiteralPath $module).Path

if (-not $output.StartsWith($resolvedModule, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to clean output outside the CAN I/O module directory"
}

if (Test-Path -LiteralPath $output) {
    Remove-Item -LiteralPath $output -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $output, (Join-Path $output "gerbers"), (Join-Path $output "drill"), (Join-Path $output "position") | Out-Null

$schematic = Join-Path $module "can-io-module.kicad_sch"
$pcb = Join-Path $module "can-io-module.kicad_pcb"

& kicad-cli sch erc --exit-code-violations --output (Join-Path $output "erc-report.rpt") $schematic
if ($LASTEXITCODE) { throw "sch erc failed: $LASTEXITCODE" }
& kicad-cli pcb drc --all-track-errors --schematic-parity --output (Join-Path $output "drc-report.rpt") $pcb
if ($LASTEXITCODE) { throw "pcb drc report failed: $LASTEXITCODE" }
& kicad-cli pcb drc --severity-error --exit-code-violations --all-track-errors --output (Join-Path $output "drc-errors.rpt") $pcb
if ($LASTEXITCODE) { throw "pcb drc error gate failed: $LASTEXITCODE" }
& kicad-cli sch export pdf --output (Join-Path $output "can-io-module-schematic.pdf") $schematic
if ($LASTEXITCODE) { throw "sch export pdf failed: $LASTEXITCODE" }
& kicad-cli pcb export svg --layers "F.Cu,B.Cu,F.Silkscreen,B.Silkscreen,Edge.Cuts" --output (Join-Path $output "can-io-module-pcb.svg") $pcb
if ($LASTEXITCODE) { throw "pcb export svg failed: $LASTEXITCODE" }
& kicad-cli pcb export gerbers --output (Join-Path $output "gerbers") $pcb
if ($LASTEXITCODE) { throw "pcb export gerbers failed: $LASTEXITCODE" }
& kicad-cli pcb export drill --output (Join-Path $output "drill") $pcb
if ($LASTEXITCODE) { throw "pcb export drill failed: $LASTEXITCODE" }
& kicad-cli pcb export pos --format csv --units mm --output (Join-Path $output "position/can-io-module-top-pos.csv") $pcb
if ($LASTEXITCODE) { throw "pcb export pos failed: $LASTEXITCODE" }

Copy-Item -LiteralPath (Join-Path $module "bom/can-io-module-bom.csv") -Destination $output
Write-Output "Prototype package written to $output"
