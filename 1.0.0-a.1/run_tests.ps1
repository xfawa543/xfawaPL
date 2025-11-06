# Run regression tests for xfawac0
# Usage: run from repository root (PowerShell)

$root = Split-Path -Parent $MyInvocation.MyCommand.Definition
Push-Location $root

$xf = Join-Path $root "xfawac0.c"
$exe = Join-Path $root "xfawac0.exe"

Write-Host "Compiling xfawac0.c..."
gcc -std=c11 -O2 -Wall $xf -o $exe
if ($LASTEXITCODE -ne 0) { Write-Error "Failed to compile xfawac0.c"; exit 1 }

$tests = @(
    @{src='hello.xf'; out='hello.exe'; expect=@('你好，世界！','114514','HelloWorld')},
    @{src='if_test.xf'; out='if_test.exe'; expect=@('you 18')},
    @{src='duoblock.xf'; out='duoblock.exe'; expect=@('你好，世界！','114514','HelloWorld','18')}
)

$allOk = $true
foreach ($t in $tests) {
    $src = Join-Path $root $t.src
    $out = Join-Path $root $t.out
    Write-Host "\nTesting $($t.src) -> $($t.out)"
    & $exe $src -o $out
    if ($LASTEXITCODE -ne 0) { Write-Error "Translator failed for $($t.src) (exit $LASTEXITCODE)"; $allOk = $false; continue }
    Write-Host "Running generated $out..."
    $outText = & $out 2>&1
    foreach ($exp in $t.expect) {
        if ($outText -notmatch [regex]::Escape($exp)) { Write-Error "Expected '$exp' not found in output of $($t.out)"; $allOk = $false }
    }
}

if ($allOk) { Write-Host "All tests passed."; exit 0 } else { Write-Error "Some tests failed."; exit 2 }
